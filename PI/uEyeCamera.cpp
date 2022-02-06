#include <iostream>

#include "uEyeCamera.h"

uEyeCamera::uEyeCamera() : m_hCam(static_cast<HIDS>(0))
{
    INT nRet = is_InitCamera(&m_hCam, nullptr);
    if (nRet == IS_SUCCESS)
    {
        nRet = is_ParameterSet(m_hCam, IS_PARAMETERSET_CMD_LOAD_EEPROM, NULL, NULL);
        if (nRet != 0)
        {
            push_log("Can't set parameters");
            return;
        }
        SENSORINFO SensorInfo;
        nRet = is_GetSensorInfo(m_hCam, &SensorInfo);
        if (nRet != 0)
        {
            push_log("Can't get sensor info");
            return;
        }

        CAMINFO CamInfo;
        nRet = is_GetCameraInfo(m_hCam, &CamInfo);
        if (nRet != 0)
        {
            push_log("Can't get camera info");
            return;
        }

        m_nSizeX = SensorInfo.nMaxWidth;
        m_nSizeY = SensorInfo.nMaxHeight;

        IS_SIZE_2D imageSize;
        imageSize.s32Width = m_nSizeX;
        imageSize.s32Height = m_nSizeY;

        if (SensorInfo.nColorMode == IS_COLORMODE_MONOCHROME)
        {
            m_nBitsPerPixel = 8;
            nRet = is_SetColorMode(m_hCam, IS_CM_MONO8);
            if (nRet != 0)
            {
                push_log("Can't set color mode");
                return;
            }
        }
        else
        {
            push_log("Unknown color mode");
            return;
        }
        m_dwSingleBufferSize = m_nSizeX * m_nSizeY * m_nBitsPerPixel / 8;
        push_log("Camera type " + std::string(SensorInfo.strSensorName));

        nRet = is_PixelClock(m_hCam, IS_PIXELCLOCK_CMD_GET, reinterpret_cast<void *>(&m_nCamPclk), sizeof(m_nCamPclk));
        if (nRet != 0)
        {
            push_log("Can't get pixel clock");
            return;
        }
        push_log("Pixel clock = " + std::to_string(m_nCamPclk));

        nRet = is_SetFrameRate(m_hCam, IS_GET_FRAMERATE, &newFps);
        if (nRet != 0)
        {
            push_log("Can't get fram rate");
        }
        push_log("Frame rate = " + std::to_string(newFps));
    }
    else
    {
        push_log("Camera failed");
    }
}

void uEyeCamera::load_config(std::string path)
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
