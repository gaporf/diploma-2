#include "findcamera.h"
#include "ui_findcamera.h"

#include <iostream>

#include <QFileDialog>
#include <QPixmap>
#include <QBitmap>

FindCamera::FindCamera(QWidget *parent, uEyeCamera *camera) :
    QDialog(parent),
    ui(new Ui::FindCamera),
    camera(camera)
{
    ui->setupUi(this);

    connect(ui->loadConfigButton, &QPushButton::clicked, this, [this]
    {
        ui->errorLabel->setText("");
        this->camera->load_config();
        ui->errorLabel->setStyleSheet("color: green");
        ui->errorLabel->setText("Config was loaded successfully");

        ui->startLiveButton->setEnabled(true);
     });

    connect(ui->closeButton, &QPushButton::clicked, this, [this]
    {
        this->close();
    });

    connect(ui->startLiveButton, &QPushButton::clicked, this, [this]
    {
        is_started = true;
        this->camera->start_capture();
        timer->start(100);
        ui->startLiveButton->setEnabled(false);
        ui->stopButtonLive->setEnabled(true);
    });

    connect(ui->stopButtonLive, &QPushButton::clicked, this, [this]
    {
        is_started = false;
        timer->stop();
        this->camera->stop_capture();
        ui->startLiveButton->setEnabled(true);
        ui->stopButtonLive->setEnabled(false);
    });

    connect(timer, &QTimer::timeout, this, [this]
    {
        char *picture = this->camera->get_picture();
        QImage image(reinterpret_cast<uchar *>(picture), this->camera->get_width(), this->camera->get_height(), this->camera->get_width(), QImage::Format_Indexed8);
        QPixmap pixmap = QPixmap::fromImage(image);
        ui->imageLabel->setPixmap(pixmap);
        ui->imageLabel->setMask(pixmap.mask());
        ui->imageLabel->show();
    });
}

FindCamera::~FindCamera()
{
    if (is_started)
    {
        is_started = false;
        this->camera->stop_capture();
    }
    delete ui;
}
