#ifndef FINDCAMERA_H
#define FINDCAMERA_H

#include <QDialog>

#include "uEyeCamera.h"

namespace Ui {
class FindCamera;
}

class FindCamera : public QDialog
{
    Q_OBJECT

public:
    FindCamera(QWidget *parent = nullptr, uEyeCamera *camera = nullptr);
    ~FindCamera();

private:
    Ui::FindCamera *ui;
    uEyeCamera *camera = nullptr;
};

#endif // FINDCAMERA_H
