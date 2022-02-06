#ifndef UEYECAMERA_H
#define UEYECAMERA_H

#include <queue>

#include <QLibrary>

#include "uEye.h"

struct uEyeCamera
{
    uEyeCamera();

    std::string get_logs();

    void load_config(std::string path);

    void capture();
private:
    HIDS m_hCam;
    INT m_nSizeX, m_nSizeY;
    INT m_nBitsPerPixel;
    UINT64 m_dwSingleBufferSize;
    int m_nCamPclk;
    double newFps;

    std::queue<std::string> logs;

    void push_log(std::string log);
};

#endif // UEYECAMERA_H
