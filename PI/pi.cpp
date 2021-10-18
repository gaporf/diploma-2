#include "pi.h"
#include "findaxes.h"
#include "ui_pi.h"

#include <iostream>
#include <thread>

PI::PI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PI)
{
    ui->setupUi(this);

    connect(ui->x0LineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString x0_position_str = ui->x0LineEdit->text();
        double x0_position = x0_position_str.toDouble(&is_double);
        if (!is_double || x0_position < x_controller->get_min_position() || x0_position > x_controller->get_max_position())
        {
            ui->x0LineEdit->setStyleSheet("color: red");
            ui->x0Label->setStyleSheet("color: red");
            std::string info = "position should be valid double from " + std::to_string(x_controller->get_min_position()) + " to " + std::to_string(x_controller->get_max_position());
            ui->x0Label->setText(QString::fromStdString(info));

            button_mask &= ~1;
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->x0LineEdit->setStyleSheet("color: green");
            ui->x0Label->setText("");

            button_mask |= 1;
            if (button_mask == 15)
            {
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->y0LineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString y0_position_str = ui->y0LineEdit->text();
        double y0_position = y0_position_str.toDouble(&is_double);
        if (!is_double || y0_position < y_controller->get_min_position() || y0_position > y_controller->get_max_position())
        {
            ui->y0LineEdit->setStyleSheet("color: red");
            ui->y0Label->setStyleSheet("color: red");
            std::string info = "position should be valid double from " + std::to_string(y_controller->get_min_position()) + " to " + std::to_string(y_controller->get_max_position());
            ui->y0Label->setText(QString::fromStdString(info));

            button_mask &= ~2;
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->y0LineEdit->setStyleSheet("color: green");
            ui->y0Label->setText("");

            button_mask |= 2;
            if (button_mask == 15)
            {
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->xnLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString xn_position_str = ui->xnLineEdit->text();
        double xn_position = xn_position_str.toDouble(&is_double);
        if (!is_double || xn_position < x_controller->get_min_position() || xn_position > x_controller->get_max_position())
        {
            ui->xnLineEdit->setStyleSheet("color: red");
            ui->xnLabel->setStyleSheet("color: red");
            std::string info = "position should be valid double from " + std::to_string(x_controller->get_min_position()) + " to " + std::to_string(x_controller->get_max_position());
            ui->xnLabel->setText(QString::fromStdString(info));

            button_mask &= ~4;
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->xnLineEdit->setStyleSheet("color: green");
            ui->xnLabel->setText("");

            button_mask |= 4;
            if (button_mask == 15)
            {
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->ynLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString yn_position_str = ui->ynLineEdit->text();
        double yn_position = yn_position_str.toDouble(&is_double);
        if (!is_double || yn_position < y_controller->get_min_position() || yn_position > y_controller->get_max_position())
        {
            ui->ynLineEdit->setStyleSheet("color: red");
            ui->ynLabel->setStyleSheet("color: red");
            std::string info = "position should be valid double from " + std::to_string(y_controller->get_min_position()) + " to " + std::to_string(y_controller->get_max_position());
            ui->ynLabel->setText(QString::fromStdString(info));

            button_mask &= ~8;
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->ynLineEdit->setStyleSheet("color: green");
            ui->ynLabel->setText("");

            button_mask |= 8;
            if (button_mask == 15)
            {
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

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
        try
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
        }
        catch (std::exception &e)
        {
            ui->logs->append("<span style=\"color:red\">Can't find axes</span>");
        }
    });

    connect(ui->startScanningButton, &QPushButton::clicked, this, [this]
    {
        double x0 = ui->x0LineEdit->text().toDouble();
        double xn = ui->xnLineEdit->text().toDouble();
        double y0 = ui->y0LineEdit->text().toDouble();
        double yn = ui->ynLineEdit->text().toDouble();
        x_controller->set_velocity(20);
        x_controller->move(x0);
        y_controller->set_velocity(20);
        y_controller->move(y0);

    });
}

PI::~PI()
{
    delete ui;
}

