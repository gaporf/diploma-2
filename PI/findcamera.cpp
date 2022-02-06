#include "findcamera.h"
#include "ui_findcamera.h"

#include <QFileDialog>

FindCamera::FindCamera(QWidget *parent, uEyeCamera *camera) :
    QDialog(parent),
    ui(new Ui::FindCamera),
    camera(camera)
{
    ui->setupUi(this);

    connect(ui->browseButton, &QPushButton::clicked, this, [this]
    {
        QString path = QFileDialog::getOpenFileName(this, "Open a file", "", "");
        ui->pathLine->setText(path);
    });

    connect(ui->loadConfigButton, &QPushButton::clicked, this, [this]
    {
        std::string path = ui->pathLine->text().toStdString();
        if (path == "")
        {
            ui->errorLabel->setStyleSheet("color: red");
            ui->errorLabel->setText("Path shouldn't be empty");
        }
        else
        {
            ui->errorLabel->setText("");
            this->camera->load_config(path);
            ui->errorLabel->setStyleSheet("color: green");
            ui->errorLabel->setText("Config was loaded successfully");
        }
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
