#include "findcamera.h"
#include "ui_findcamera.h"

#include <QFileDialog>

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
    });

    connect(ui->closeButton, &QPushButton::clicked, this, [this]
    {
        this->close();
    });
}

FindCamera::~FindCamera()
{
    delete ui;
}
