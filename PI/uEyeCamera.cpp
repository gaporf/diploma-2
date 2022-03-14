#include <iostream>
#include <fstream>
#include <mutex>

#include "uEyeCamera.h"

uEyeCamera::uEyeCamera() : m_hCam(static_cast<HIDS>(0)), m_nBuffersNew(850), lib("opencv_world455.dll")
{
    wait_live_picture.store(0);
    wait_picture.store(0);

    INT nRet = is_InitCamera(&m_hCam, nullptr);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Failed to initiliaze camera");
    }

    nRet = is_ParameterSet(m_hCam, IS_PARAMETERSET_CMD_LOAD_EEPROM, NULL, NULL);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't load default parameters");
    }

    nRet = is_GetSensorInfo(m_hCam, &m_sInfo);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't get sensor info");
    }

    CAMINFO CamInfo;
    nRet = is_GetCameraInfo(m_hCam, &CamInfo);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't get camera info");
    }

    m_nSizeX = m_sInfo.nMaxWidth;
    m_nSizeY = m_sInfo.nMaxHeight;
    push_log("m_nSizeX = " + std::to_string(m_nSizeX) + ", m_nSizeY = " + std::to_string(m_nSizeY));

    IS_SIZE_2D imageSize;
    imageSize.s32Width = m_nSizeX;
    imageSize.s32Height = m_nSizeY;

    if (m_sInfo.nColorMode == IS_COLORMODE_MONOCHROME)
    {
        m_nBitsPerPixel = 8;
        nRet = is_SetColorMode(m_hCam, IS_CM_MONO8);
        if (nRet != 0)
        {
            throw std::runtime_error("Can't set color mode");
        }
    }
    else
    {
        throw std::runtime_error("Unknown color mode");
    }

    m_dwSingleBufferSize = m_nSizeX * m_nSizeY * m_nBitsPerPixel / 8;
    push_log("m_dwSingleBufferSize = " + std::to_string(m_dwSingleBufferSize));

    nRet = is_SetDisplayMode(m_hCam, IS_SET_DM_DIB);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't set display mode");
    }

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    m_dwSysMemTotal = stat.dwTotalPhys;

    SeqBuilt();
    EvInitAll();

    nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_GET, (void*)&m_nCamPclk, sizeof(m_nCamPclk));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't get Pixel Clock");
    }

    double newFps;
    nRet = is_SetFrameRate(m_hCam, IS_GET_FRAMERATE, &newFps);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't get frame rate");
    }
    push_log("fps = " + std::to_string(newFps));

    int nDllVer = is_GetDLLVersion();
    char strDllVer[32];
    sprintf(strDllVer, "%d.%02d.%04d", ((nDllVer >> 24) & 0xff), ((nDllVer >> 16) & 0xff), (nDllVer & 0xff));
    push_log("DLL version = " + std::string(strDllVer));
}

void uEyeCamera::load_config()
{
    if (is_ParameterSet(m_hCam, IS_PARAMETERSET_CMD_LOAD_FILE, NULL, NULL) == IS_SUCCESS)
    {
        IS_SIZE_2D imageSize;
        INT nRet = is_AOI(m_hCam, IS_AOI_IMAGE_GET_SIZE, reinterpret_cast<void *>(&imageSize), sizeof(imageSize));
        if (nRet != 0)
        {
            push_log("is_AOI returned non-zero value");
            throw std::runtime_error("Can't call is_AOI");
        }

        m_nSizeX = imageSize.s32Width;
        m_nSizeY = imageSize.s32Height;
        push_log("m_nSizeX = " + std::to_string(m_nSizeX) + ", m_nSizeY = " + std::to_string(m_nSizeY));

        INT m_nColorMode = is_SetColorMode(m_hCam, IS_GET_COLOR_MODE);
        switch (m_nColorMode)
        {
            case IS_CM_BGRA8_PACKED:
                m_nBitsPerPixel = 32;
                break;
            case IS_CM_BGR8_PACKED:
                m_nBitsPerPixel = 24;
                break;
            case IS_CM_BGR565_PACKED:
                m_nBitsPerPixel = 16;
                break;
            case IS_CM_BGR5_PACKED:
                m_nBitsPerPixel = 15;
                break;
            case IS_CM_MONO8:
                m_nBitsPerPixel = 8;
                break;
            case IS_CM_MONO12:
                m_nBitsPerPixel = 12;
                break;
            case IS_CM_UYVY_PACKED:
                m_nBitsPerPixel = 16;
                break;
            case IS_CM_SENSOR_RAW10:
                m_nBitsPerPixel = 10;
                break;
            default:
                push_log("Invalid color mode");
                throw std::runtime_error("Unknown color mode");
        }

        ReallocteBuffers();
        InitSliderExposure();

        nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_GET, (void*)&m_nCamPclk, sizeof(m_nCamPclk));
        if (nRet != IS_SUCCESS)
        {
            throw std::runtime_error("Can't get Pixel Clock");
        }
        push_log("Pixel clock = " + std::to_string(m_nCamPclk));

        double newFps;
        nRet = is_SetFrameRate(m_hCam, IS_GET_FRAMERATE, &newFps);
        if (nRet != IS_SUCCESS)
        {
            throw std::runtime_error("Can't get Fps settings");
        }
        push_log("FPS rate = " + std::to_string(newFps));

        nRet = is_ParameterSet(m_hCam, IS_PARAMETERSET_CMD_SAVE_EEPROM, NULL, NULL);
        if (nRet != IS_SUCCESS)
        {
            throw std::runtime_error("Can't set parameters");
        }
    }
}

void uEyeCamera::push_log(std::string log)
{
    if (log[log.length() - 1] == '\n')
    {
        log = log.substr(0, log.size() - 1);
    }
    logs.push("camera: " + log + "<br>");
}

void uEyeCamera::capture(std::string path, pi_controller *z_controller, double z0, double zn, double zs)
{
    INT nRet;
    start_capture();
    std::atomic_int working;
    while (!pictures.empty()) {
        pictures.pop();
    }
    z_controller->set_velocity(zs);
    std::ofstream out_file(path + "/info.txt");
    std::thread th([z_controller, z0, zn, zs, &working, &out_file]
    {
        z_controller->set_velocity(zs);
        if (std::abs(z_controller->get_current_position() - z0) < std::abs(z_controller->get_current_position() - zn))
        {
            out_file << "normal" << std::endl;
            z_controller->move(zn);
        }
        else
        {
            out_file << "reverse" << std::endl;
            z_controller->move(z0);
        }
        working.store(0);
    });
    working.store(1);
    wait_picture.store(1);
    std::atomic<int> cnt;
    cnt.store(0);
    std::thread th1([&cnt, &working, &path, this]
    {
        while (working == 1) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
        wait_picture.store(0);
        while (!pictures.empty()) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
    });
    std::thread th2([&cnt, &working, &path, this]
    {
        while (working == 1) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
        wait_picture.store(0);
        while (!pictures.empty()) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
    });
    std::thread th3([&cnt, &working, &path, this]
    {
        while (working == 1) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
        wait_picture.store(0);
        while (!pictures.empty()) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
    });
    std::thread th4([&cnt, &working, &path, this]
    {
        while (working == 1) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
        wait_picture.store(0);
        while (!pictures.empty()) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
    });
    std::thread th5([&cnt, &working, &path, this]
    {
        while (working == 1) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
        wait_picture.store(0);
        while (!pictures.empty()) {
            char *picture;
            {
                std::unique_lock<std::mutex> lg(m);
                if (pictures.empty()) {
                    continue;
                }
                picture = pictures.front();
                pictures.pop();
            }
            cv::Mat image(cv::Size(m_nSizeX, m_nSizeY), CV_8UC1, picture, m_nSizeX);
            int cur_cnt;
            while (true) {
                cur_cnt = cnt;
                if (cnt.compare_exchange_strong(cur_cnt, cur_cnt + 1))
                {
                    break;
                }
            }
            imwrite(path + "/" + fit(std::to_string(cur_cnt)) + ".png", image);
        }
    });
    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();
    stop_capture();
    th.join();
}

void uEyeCamera::start_capture()
{
    ReallocteBuffers();
    int cur;
    while (true)
    {
        cur = current_captures;
        if (current_captures.compare_exchange_strong(cur, cur + 1))
        {
            break;
        }
    }
    if (cur == 0)
    {
        is_CaptureVideo(m_hCam, IS_DONT_WAIT);
    }
    else
    {
        _sleep(1000);
    }
}

void uEyeCamera::stop_capture()
{
    int cur;
    while (true)
    {
        cur = current_captures;
        if (current_captures.compare_exchange_strong(cur, cur + 1))
        {
            break;
        }
    }
    if (cur == 0)
    {
        is_StopLiveVideo(m_hCam, IS_FORCE_VIDEO_STOP);
    }
    else
    {
        _sleep(1000);
    }
}

char *uEyeCamera::get_picture()
{
    start_capture();
    wait_live_picture.store(1);
    while (true)
    {
        if (wait_live_picture != 2)
        {
            _sleep(50);
            continue;
        }
        wait_live_picture.store(0);
        break;
    }
    stop_capture();
    return live_picture;
}

std::string uEyeCamera::fit(std::string number) {
    while (number.length() < 5) {
        number = "0" + number;
    }
    return number;
}

std::string uEyeCamera::get_logs()
{
    std::string res;
    while (!logs.empty())
    {
        res += logs.front();
        logs.pop();
    }
    return res;
}

void uEyeCamera::SeqBuilt()
{
    unsigned int imax = m_nBuffersNew;
    IS_SIZE_2D imageSize;
    INT nRet = is_AOI(m_hCam, IS_AOI_IMAGE_GET_SIZE, (void*)&imageSize, sizeof(imageSize));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("is_AOI returned non-zero value");
    }

    m_nSizeX = imageSize.s32Width;
    m_nSizeY = imageSize.s32Height;
    push_log("m_nSizeX = " + std::to_string(m_nSizeX) + ", m_nSizeY = " + std::to_string(m_nSizeY));

    INT nAllocSizeX = 0;
    INT nAllocSizeY = 0;
    UINT nAbsPosX = 0;
    UINT nAbsPosY = 0;

    nRet = is_AOI(m_hCam, IS_AOI_IMAGE_GET_POS_X_ABS, (void*)&nAbsPosX, sizeof(nAbsPosX));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("is_AOI returned non-zero value");
    }
    nRet = is_AOI(m_hCam, IS_AOI_IMAGE_GET_POS_Y_ABS, (void*)&nAbsPosY, sizeof(nAbsPosY));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("is_AOI returned non-zero value");
    }

    if (nAbsPosX)
    {
        nAllocSizeX = m_sInfo.nMaxWidth;
    }
    if (nAbsPosY)
    {
        nAllocSizeY = m_sInfo.nMaxHeight;
    }

    IS_RECT rectAOI, rectAOI2;
    INT NEW_x, NEW_y, NEW_width, NEW_height;
    nRet = is_AOI(m_hCam, IS_AOI_IMAGE_GET_AOI, (void*)&rectAOI, sizeof(rectAOI));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("is_AOI returned non-zero value");
    }

    NEW_x = rectAOI.s32X;
    NEW_y = rectAOI.s32Y;
    NEW_width = rectAOI.s32Width;
    NEW_height = rectAOI.s32Height;
    //rectAOI2.s32X = NEW_x | IS_AOI_IMAGE_POS_ABSOLUTE;
    //rectAOI2.s32Y = NEW_y | IS_AOI_IMAGE_POS_ABSOLUTE;
    rectAOI2.s32X = NEW_x;
    rectAOI2.s32Y = NEW_y;
    rectAOI2.s32Width = NEW_width;
    rectAOI2.s32Height = NEW_height;
    nRet = is_AOI(m_hCam, IS_AOI_IMAGE_SET_AOI, (void*)&rectAOI2, sizeof(rectAOI2));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("is_AOI returned non-zero value");
    }

    m_dwSingleBufferSize = NEW_width * NEW_height * m_nBitsPerPixel / 8;
    push_log("m_dwSingleBufferSize = " + std::to_string(m_dwSingleBufferSize));

    unsigned int i;
    for (i = 0; i < imax; i++)
    {
        nRet = is_AllocImageMem(m_hCam, NEW_width, NEW_height, m_nBitsPerPixel, &m_pcSeqImgMem[i], &m_lSeqMemId[i]);
        if (nRet != IS_SUCCESS)
        {
            throw std::runtime_error("Can't allocate memory");
        }

        nRet = is_AddToSequence(m_hCam, m_pcSeqImgMem[i], m_lSeqMemId[i]);
        if (nRet != IS_SUCCESS)
        {
            is_FreeImageMem(m_hCam, m_pcSeqImgMem[i], m_lSeqMemId[i]);
            throw std::runtime_error("AddToSequence returned non-zero value");
        }
        m_nSeqNumId[i] = i + 1;
    }

    m_nBuffersInUse = i;
    m_nBufferStart = 0;
    m_nBufferStop = m_nBuffersInUse - 1;
}

void uEyeCamera::SeqKill()
{
    INT nRet = is_ClearSequence(m_hCam);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't clear sequence");
    }

    int i;
    for (i = (m_nBuffersInUse - 1); i >= 0; i--)
    {
        nRet = is_FreeImageMem(m_hCam, m_pcSeqImgMem[i], m_lSeqMemId[i]);
        if (nRet != IS_SUCCESS)
        {
            throw std::runtime_error("Can't free image");
        }
    }
    m_nBuffersInUse = i;
}

void uEyeCamera::EvInitAll()
{
    EvEnumerate();

    for (int i = 0; i < m_EvMax; i++)
    {
        m_hEv[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hEv[i] == NULL)
        {
            throw std::runtime_error("Can't create event");
        }
        EvInit(m_nEvUI[i]);
        EvEnable(m_nEvUI[i], true);
        m_nEvCount[i] = 0;
    }

    wait_picture.store(0);
    event_thread = new std::thread([this]
    {
        for(;;)
        {
            DWORD lReturn = WaitForMultipleObjects(m_EvMax, m_hEv, FALSE, INFINITE);
            int nEvIndex = lReturn - WAIT_OBJECT_0;
            if (m_nEvUI[nEvIndex] == IS_SET_EVENT_FRAME)
            {
                INT nRet = is_GetActSeqBuf(m_hCam, &nNum, &pcMem, &pcMemLast);
                if (nRet != IS_SUCCESS)
                {
                    throw std::runtime_error("Can't take picture");
                }
                if (wait_picture == 1)
                {
                    std::unique_lock<std::mutex> lg(m);
                    pictures.push(pcMemLast);
                }
                if (wait_live_picture == 1)
                {
                    live_picture = pcMemLast;
                    wait_live_picture.store(2);
                }
            }
        }
    });
}

void uEyeCamera::EvEnumerate()
{
    m_nEvUI[0] = IS_SET_EVENT_FRAME;
    m_nEvUI[1] = IS_SET_EVENT_SEQ;
    m_nEvUI[2] = IS_SET_EVENT_CAPTURE_STATUS;
    m_nEvUI[3] = IS_SET_EVENT_DEVICE_RECONNECTED;
    m_nEvUI[4] = IS_SET_EVENT_REMOVE;

    m_EvMax = 5;
}

void uEyeCamera::EvInit(INT nEvent)
{
    int nIndex;
    EvGetIndex(nEvent, &nIndex);
    INT nRet = is_InitEvent(m_hCam, m_hEv[nIndex], nEvent);
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't initialize event");
    }
}

void uEyeCamera::EvGetIndex(INT nEvent, int *pnIndex)
{
    for (int i = 0; i < m_EvMax; i++)
    {
        if (m_nEvUI[i] == nEvent)
        {
            *pnIndex = i;
            return;
        }
    }
    throw std::runtime_error("Can't find index");
}

void uEyeCamera::EvEnable(INT nEvent, bool bEnable)
{
    if (bEnable)
    {
        INT nRet = is_EnableEvent(m_hCam, nEvent);
        if (nRet != IS_SUCCESS)
        {
            throw std::runtime_error("Can't enable event");
        }
    }
    else
    {
        is_DisableEvent(m_hCam, nEvent);
    }
}

void uEyeCamera::ReallocteBuffers()
{
    SeqKill();
    SeqBuilt();
}

void uEyeCamera::InitSliderExposure()
{
    double dExposure = 0.0;
    double dbrange[3];

    INT nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&dExposure, sizeof(dExposure));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't set exposure");
    }

    nRet = is_Exposure(m_hCam, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, (void*)&dbrange, sizeof(dbrange));
    if (nRet != IS_SUCCESS)
    {
        throw std::runtime_error("Can't get exposure range");
    }
    push_log("Exposure range from " + std::to_string(dbrange[0]) + " to " + std::to_string(dbrange[1]));
}
