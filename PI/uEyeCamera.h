#ifndef UEYECAMERA_H
#define UEYECAMERA_H

#include <queue>
#include <thread>

#include <QLibrary>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include "uEye.h"

struct uEyeCamera
{
    uEyeCamera();

    std::string get_logs();

    void load_config();

    void capture(std::string path);
private:
    QLibrary lib;

#define MIN_SEQ_BUFFERS 3
#define MAX_SEQ_BUFFERS 32767

    HIDS m_hCam;
    SENSORINFO m_sInfo;
    INT m_nSizeX, m_nSizeY;
    INT m_nBitsPerPixel;
    UINT64 m_dwSingleBufferSize;
    HWND m_hWndDisplay;
    UINT64 m_dwSysMemTotal;
    UINT m_nBuffersNew;

    INT		m_lSeqMemId[MAX_SEQ_BUFFERS];
    char*	m_pcSeqImgMem[MAX_SEQ_BUFFERS];
    int		m_nSeqNumId[MAX_SEQ_BUFFERS];

    UINT	m_nBuffersInUse;
    int		m_nBufferStart;
    int		m_nBufferStop;

#define MAX_EV      32

    HANDLE  m_hEv[MAX_EV];			// event handle array
    INT     m_nEvUI[MAX_EV];		// type of uEye event
    int     m_nEvCount[MAX_EV];		// event counter
    int     m_EvMax;				// maximum number of events we use

    int m_nCamPclk;
    double newFps;

    std::thread *event_thread;
    INT nNum;
    char *pcMem, *pcMemLast;
    char *picture;
    std::atomic<int> wait_picture;

    std::queue<std::string> logs;

    void push_log(std::string log);

    void SeqBuilt();

    void SeqKill();

    void EvInitAll();

    void EvEnumerate();

    void EvInit(INT nEvent);

    void EvGetIndex(INT nEvent, int *pnIndex);

    void EvEnable(INT nEvent, bool bEnable);

    void ReallocteBuffers();

    void InitSliderExposure();
};

#endif // UEYECAMERA_H
