#include "pi.h"
#include "findaxes.h"
#include "ui_pi.h"

#include <iostream>

#include <QThread>

PI::PI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PI)
{
    ui->setupUi(this);

    connect(timer, &QTimer::timeout, this, [this]
    {
        if (x_controller != nullptr)
        {
            QString logs = QString().fromStdString(x_controller->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:blue\">" + logs + "</span>";
                ui->logs->append(logs);
            }
        }
        if (y_controller != nullptr)
        {
            QString logs = QString().fromStdString(y_controller->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:orange\">" + logs + "</span>";
                ui->logs->append(logs);
            }
        }
    });
    timer->start(1000);

    connect(ui->findAxesButton, &QPushButton::clicked, this, [this]
    {
        x_controller = new pi_controller("x");
        int x_port = x_controller->connect_vid_usb(0);
        y_controller = new pi_controller("y");
        y_controller->connect_vid_usb(x_port + 1);

        x_controller->get_controller_info();
        y_controller->get_controller_info();

        x_controller->get_axes_info();
        y_controller->get_axes_info();

        x_controller->init_stage();
        y_controller->init_stage();

        x_controller->reference();
        y_controller->reference();

        x_controller->get_limits();
        y_controller->get_limits();

        FindAxes *findAxes = new FindAxes(nullptr, &x_controller, &y_controller);
        findAxes->show();

        ui->x0LineEdit->setEnabled(true);
        ui->xnLineEdit->setEnabled(true);
        ui->y0LineEdit->setEnabled(true);
        ui->ynLineEdit->setEnabled(true);
    });
}

PI::~PI()
{
    delete ui;
}

