#include "pi.h"
#include "findaxes.h"
#include "ui_pi.h"

#include <iostream>
#include <thread>


PI::PI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PI)
    , move_to_start_position_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                move_to_start_position_var.wait(lg, [this]
                {
                    return move_to_start_position_clicked;
                });
                std::thread th1([this]
                {
                    (*x_controller).set_velocity(15);
                    (*x_controller).move(ui->x0LineEdit->text().toDouble());
                });
                std::thread th2([this]
                {
                    (*y_controller).set_velocity(15);
                    (*y_controller).move(ui->y0LineEdit->text().toDouble());
                });
                th1.join();
                th2.join();
                move_to_start_position_clicked = false;
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
                lg.unlock();
            }
        })
    , scanning_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                scanning_var.wait(lg, [this]
                {
                    return scanning_clicked;
                });
                is_cancelled.store(false);
                double x0_size = ui->x0LineEdit->text().toDouble();
                double y0_size = ui->y0LineEdit->text().toDouble();
                double xN_size = ui->xnLineEdit->text().toDouble();
                double yN_size = ui->ynLineEdit->text().toDouble();
                double xs_size = ui->xsLineEdit->text().toDouble();
                double ys_size = ui->ysLineEdit->text().toDouble();
                double xo_size = ui->xoLineEdit->text().toDouble();
                double yo_size = ui->yoLineEdit->text().toDouble();
                if (xo_size >= xs_size)
                {
                    ui->xoLabel->setStyleSheet("color: red");
                    std::string info = "Overlapped size should be less than frame size";
                    ui->xoLabel->setText(QString::fromStdString(info));
                }
                else if (yo_size >= ys_size)
                {
                    ui->yoLabel->setStyleSheet("color: red");
                    std::string info = "Overlapped size should be less than frame size";
                    ui->yoLabel->setText(QString::fromStdString(info));
                }
                else if (std::abs(x0_size - xN_size) < xs_size)
                {
                    ui->xsLabel->setStyleSheet("color: red");
                    std::string info = "Frame size should be not greater than scanning size";
                    ui->xsLabel->setText(QString::fromStdString(info));
                }
                else if (std::abs(y0_size - yN_size) < ys_size)
                {
                    ui->ysLabel->setStyleSheet("color: red");
                    std::string info = "Frame size should be not greater than scanning size";
                    ui->ysLabel->setText(QString::fromStdString(info));
                }
                else
                {
                    std::thread th1([this, x0_size, xN_size, xs_size]
                    {
                        (*x_controller).set_velocity(15);
                        (*x_controller).move(x0_size + (x0_size < xN_size ? 1 : -1) * xs_size / 2);
                    });
                    std::thread th2([this, y0_size, yN_size, ys_size]
                    {
                        (*y_controller).set_velocity(15);
                        (*y_controller).move(y0_size + (y0_size < yN_size ? 1 : -1) * ys_size / 2);
                    });
                    th1.join();
                    th2.join();
                    double cur_x = (*x_controller).get_current_position();
                    double cur_y = (*y_controller).get_current_position();
                    char direction = (x0_size < xN_size ? 'F' : 'B');
                    ui->cancelScanningButton->setEnabled(true);
                    while ((y0_size < yN_size && cur_y - (ys_size - yo_size) / 2 <= yN_size) || (y0_size > yN_size && cur_y + (ys_size - yo_size) / 2 >= yN_size))
                    {
                        if (direction == 'F')
                        {
                            while (cur_x - (xs_size - xo_size) / 2 <= max(x0_size, xN_size))
                            {
                                if (is_cancelled.load())
                                {
                                    break;
                                }
                                capture_and_save(x_controller->get_current_position(), y_controller->get_current_position());
                                cur_x = cur_x + xs_size - xo_size;
                                if (cur_x - (xs_size - xo_size) / 2 <= max(x0_size, xN_size))
                                {
                                    (*x_controller).set_velocity(10);
                                    (*x_controller).move(min(cur_x, x_controller->get_max_position()));
                                }
                            }
                            direction = 'B';
                        }
                        else if (direction == 'B')
                        {
                            while (cur_x + (xs_size - xo_size) / 2 >= min(x0_size, xN_size))
                            {

                                if (is_cancelled.load())
                                {
                                    break;
                                }
                                capture_and_save(x_controller->get_current_position(), y_controller->get_current_position());
                                cur_x = cur_x - xs_size + xo_size;
                                if (cur_x + (xs_size - xo_size) / 2 >= min(x0_size, xN_size))
                                {
                                    (*x_controller).set_velocity(10);
                                    (*x_controller).move(max(cur_x, x_controller->get_min_position()));
                                }
                            }
                            direction = 'F';
                        }
                        if (is_cancelled.load())
                        {
                            break;
                        }
                        cur_y = cur_y + (yN_size > y0_size ? 1 : -1) * (ys_size - yo_size);
                        cur_y = min(y_controller->get_max_position(), max(y_controller->get_min_position(), cur_y));
                        if (cur_y + (ys_size - yo_size) / 2 >= min(y0_size, yN_size) && cur_y - (ys_size - yo_size) / 2 <= max(y0_size, yN_size))
                        {
                            (*y_controller).set_velocity(10);
                            (*y_controller).move(max(y_controller->get_min_position(), min(y_controller->get_max_position(), cur_y)));
                        }
                    }
                }
                scanning_clicked = false;
                ui->cancelScanningButton->setEnabled(false);
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
                lg.unlock();
            }
        })
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
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->x0LineEdit->setStyleSheet("color: green");
            ui->x0Label->setText("");

            button_mask |= 1;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
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
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->y0LineEdit->setStyleSheet("color: green");
            ui->y0Label->setText("");

            button_mask |= 2;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
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
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->xnLineEdit->setStyleSheet("color: green");
            ui->xnLabel->setText("");

            button_mask |= 4;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
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
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->ynLineEdit->setStyleSheet("color: green");
            ui->ynLabel->setText("");

            button_mask |= 8;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->xsLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString xs_size_str = ui->xsLineEdit->text();
        double xs_size = xs_size_str.toDouble(&is_double);
        if (!is_double || xs_size < 0 || xs_size > x_controller->get_max_position() - x_controller->get_min_position())
        {
            ui->xsLineEdit->setStyleSheet("color: red");
            ui->xsLabel->setStyleSheet("color: red");
            std::string info = "size should be valid positive double not more " + std::to_string(x_controller->get_max_position() - x_controller->get_min_position());
            ui->xsLabel->setText(QString::fromStdString(info));

            button_mask &= ~16;
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->xsLineEdit->setStyleSheet("color: green");
            ui->xsLabel->setText("");
            button_mask |= 16;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->ysLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString ys_size_str = ui->ysLineEdit->text();
        double ys_size = ys_size_str.toDouble(&is_double);
        if (!is_double || ys_size < 0 || ys_size > y_controller->get_max_position() - y_controller->get_min_position())
        {
            ui->ysLineEdit->setStyleSheet("color: red");
            ui->ysLabel->setStyleSheet("color: red");
            std::string info = "size should be valid positive double not more " + std::to_string(y_controller->get_max_position() - y_controller->get_min_position());
            ui->ysLabel->setText(QString::fromStdString(info));

            button_mask &= ~32;
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->ysLineEdit->setStyleSheet("color: green");
            ui->ysLabel->setText("");
            button_mask |= 32;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->xoLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString xo_size_str = ui->xoLineEdit->text();
        double xo_size = xo_size_str.toDouble(&is_double);
        if (!is_double || xo_size < 0 || xo_size > x_controller->get_max_position() - x_controller->get_min_position())
        {
            ui->xoLineEdit->setStyleSheet("color: red");
            ui->xoLabel->setStyleSheet("color: red");
            std::string info = "size should be valid positive double not more " + std::to_string(x_controller->get_max_position() - x_controller->get_min_position());
            ui->xoLabel->setText(QString::fromStdString(info));

            button_mask &= ~64;
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->xoLineEdit->setStyleSheet("color: green");
            ui->xoLabel->setText("");

            button_mask |= 64;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
            }
        }
    });

    connect(ui->yoLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString yo_size_str = ui->yoLineEdit->text();
        double yo_size = yo_size_str.toDouble(&is_double);
        if (!is_double || yo_size < 0 || yo_size > y_controller->get_max_position() - y_controller->get_min_position())
        {
            ui->yoLineEdit->setStyleSheet("color: red");
            ui->yoLabel->setStyleSheet("color: red");
            std::string info = "size should be valid positive double not more " + std::to_string(y_controller->get_max_position() - y_controller->get_min_position());
            ui->yoLabel->setText(QString::fromStdString(info));

            button_mask &= ~32;
            ui->moveToStartPositionButton->setEnabled(false);
            ui->startScanningButton->setEnabled(false);
        }
        else
        {
            ui->yoLineEdit->setStyleSheet("color: green");
            ui->yoLabel->setText("");

            button_mask |= 128;
            if (button_mask == 255)
            {
                ui->moveToStartPositionButton->setEnabled(true);
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
                logs = "<span style=\"color:orange\">" + logs + "</span>";
                ui->logs->append(logs);
            }
        }
        if (y_controller != nullptr)
        {
            QString logs = QString().fromStdString(y_controller->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:green\">" + logs + "</span>";
                ui->logs->append(logs);
            }
        }
        if (z_controller != nullptr)
        {
            QString logs = QString().fromStdString(z_controller->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:blue\">" + logs + "</span>";
                ui->logs->append(logs);
            }
        }
        if (camera != nullptr)
        {
            QString logs = QString().fromStdString(camera->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:grey\">" + logs + "</span>";
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
            int y_port = y_controller->connect_vid_usb(x_port + 1);
            z_controller = new pi_controller("z");
            z_controller->connect_vid_usb(y_port + 1);

            x_controller->get_controller_info();
            y_controller->get_controller_info();
            z_controller->get_controller_info();

            x_controller->get_axes_info();
            y_controller->get_axes_info();
            z_controller->get_axes_info();

            x_controller->init_stage();
            y_controller->init_stage();
            z_controller->init_stage();

            x_controller->reference();
            y_controller->reference();
            z_controller->reference();

            x_controller->get_limits();
            y_controller->get_limits();
            z_controller->get_limits();

            FindAxes *findAxes = new FindAxes(nullptr, &x_controller, &y_controller, &z_controller);
            findAxes->show();

            ui->x0LineEdit->setEnabled(true);
            ui->xnLineEdit->setEnabled(true);
            ui->y0LineEdit->setEnabled(true);
            ui->ynLineEdit->setEnabled(true);
            ui->xsLineEdit->setEnabled(true);
            ui->ysLineEdit->setEnabled(true);
            ui->xoLineEdit->setEnabled(true);
            ui->yoLineEdit->setEnabled(true);

            ui->findAxesButton->setEnabled(false);
        }
        catch (std::exception &)
        {
            ui->logs->append("<span style=\"color:red\">Can't find axes</span>");
        }
    });

    connect(ui->findCameraButton, &QPushButton::clicked, this, [this]
    {
        try
        {
            camera = new uEyeCamera();
            FindCamera *findCamera = new FindCamera(nullptr, camera);
            findCamera->show();

            ui->findCameraButton->setEnabled(false);
        }
        catch (std::exception &)
        {
            ui->logs->append("<span style=\"color:red\">Can't find uEyeCamera</span>");
        }
    });

    connect(ui->moveToStartPositionButton, &QPushButton::clicked, this, [this]
    {
        ui->moveToStartPositionButton->setEnabled(false);
        ui->startScanningButton->setEnabled(false);
        move_to_start_position_clicked = true;
        move_to_start_position_var.notify_one();
    });

    connect(ui->startScanningButton, &QPushButton::clicked, this, [this]
    {
        ui->moveToStartPositionButton->setEnabled(false);
        ui->startScanningButton->setEnabled(false);
        scanning_clicked = true;
        scanning_var.notify_one();
    });

    connect(ui->cancelScanningButton, &QPushButton::clicked, this, [this]
    {
        is_cancelled.store(true);
    });
}

PI::~PI()
{
    delete ui;
}

void PI::capture_and_save(double x_pos, double y_pos)
{
    camera->capture("C:\\Users\\gapor\\Desktop\\uEyeSequence\\test\\t.png");
}
