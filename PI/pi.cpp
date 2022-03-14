#include "pi.h"
#include "findaxes.h"
#include "ui_pi.h"

#include <iostream>
#include <thread>
#include <set>

#include <QFileDialog>

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
                add_enabled(0);
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
                double z0_size = ui->z0LineEdit->text().toDouble();
                double cur_x = x0_size + (x0_size < xN_size ? 1 : -1) * xs_size / 2;
                double cur_y = y0_size + (y0_size < yN_size ? 1 : -1) * ys_size / 2;
                std::thread th1([this, cur_x]
                {
                    (*x_controller).set_velocity(5);
                    (*x_controller).move(cur_x);
                });
                std::thread th2([this, cur_y]
                {
                    (*y_controller).set_velocity(5);
                    (*y_controller).move(cur_y);
                });
                std::thread th3([this, z0_size]
                {
                    (*z_controller).set_velocity(0.5);
                    (*z_controller).move(z0_size);
                });
                th1.join();
                th2.join();
                th3.join();

                size_t x = 1;
                size_t y = 1;
                std::set<std::pair<double, double>> is_visited;
                char direction = (x0_size < xN_size ? 'F' : 'B');
                ui->cancelScanningButton->setEnabled(true);
                while (true)
                {
                    if (direction == 'F')
                    {
                        while (true)
                        {
                            if (is_cancelled.load())
                            {
                                break;
                            }
                            if (is_visited.find({cur_x, cur_y}) == is_visited.end())
                            {
                                capture_and_save(x, y);
                                is_visited.insert({cur_x, cur_y});
                            }
                            if (cur_x + xs_size / 2 < xN_size)
                            {
                                x++;
                                cur_x += xs_size;
                                (*x_controller).set_velocity(0.5);
                                (*x_controller).move(min(cur_x, x_controller->get_max_position()));
                            }
                            else
                            {
                                break;
                            }
                        }
                        direction = 'B';
                    }
                    else if (direction == 'B')
                    {
                        while (true)
                        {
                            if (is_cancelled.load())
                            {
                                break;
                            }
                            if (is_visited.find({cur_x, cur_y}) == is_visited.end()) {
                                capture_and_save(x, y);
                                is_visited.insert({cur_x, cur_y});
                            }
                            if (cur_x - xs_size / 2 > x0_size)
                            {
                                x--;
                                cur_x -= xs_size;
                                (*x_controller).set_velocity(10);
                                (*x_controller).move(max(cur_x, x_controller->get_min_position()));
                            }
                            else
                            {
                                break;
                            }
                        }
                        direction = 'F';
                    }
                    if (is_cancelled.load())
                    {
                        break;
                    }
                    if (cur_y + ys_size / 2 < yN_size)
                    {
                        y++;
                        cur_y = cur_y + ys_size;
                        (*y_controller).set_velocity(10);
                        (*y_controller).move(max(y_controller->get_min_position(), min(y_controller->get_max_position(), cur_y)));
                    }
                    else
                    {
                        break;
                    }
                }
                scanning_clicked = false;
                ui->cancelScanningButton->setEnabled(false);
                ui->moveToStartPositionButton->setEnabled(true);
                ui->startScanningButton->setEnabled(true);
                lg.unlock();
            }
        })
    , x_moving_left_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                x_moving_left_var.wait(lg, [this]
                {
                    return x_moving_left_clicked;
                });
                double xs = ui->xsLineEdit->text().toDouble();
                x_controller->set_velocity(0.5);
                x_controller->move(max(x_controller->get_current_position() - xs, x_controller->get_min_position()));
                x_moving_left_clicked = false;
                enable_all();
                lg.unlock();
            }
        })
    , x_moving_right_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                x_moving_right_var.wait(lg, [this]
                {
                    return x_moving_right_clicked;
                });
                double xs = ui->xsLineEdit->text().toDouble();
                x_controller->set_velocity(0.5);
                x_controller->move(min(x_controller->get_current_position() + xs, x_controller->get_max_position()));
                x_moving_right_clicked = false;
                enable_all();
                lg.unlock();
            }
        })
    , y_moving_left_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                y_moving_left_var.wait(lg, [this]
                {
                    return y_moving_left_clicked;
                });
                double ys = ui->ysLineEdit->text().toDouble();
                y_controller->set_velocity(0.5);
                y_controller->move(max(y_controller->get_current_position() - ys, y_controller->get_min_position()));
                y_moving_left_clicked = false;
                enable_all();
                lg.unlock();
            }
        })
    , y_moving_right_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                y_moving_right_var.wait(lg, [this]
                {
                    return y_moving_right_clicked;
                });
                double ys = ui->ysLineEdit->text().toDouble();
                y_controller->set_velocity(0.5);
                y_controller->move(min(y_controller->get_current_position() + ys, y_controller->get_max_position()));
                y_moving_right_clicked = false;
                enable_all();
                lg.unlock();
            }
        })
    , z_moving_down_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                z_moving_down_var.wait(lg, [this]
                {
                    return z_moving_down_clicked;
                });
                double zs = ui->zsLineEdit->text().toDouble();
                z_controller->set_velocity(0.5);
                z_controller->move(max(z_controller->get_current_position() - zs, z_controller->get_min_position()));
                z_moving_down_clicked = false;
                enable_all();
                lg.unlock();
            }
        })
    , z_moving_up_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                z_moving_up_var.wait(lg, [this]
                {
                    return z_moving_up_clicked;
                });
                double zs = ui->zsLineEdit->text().toDouble();
                z_controller->set_velocity(0.5);
                z_controller->move(min(z_controller->get_current_position() + zs, z_controller->get_max_position()));
                z_moving_up_clicked = false;
                enable_all();
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
            add_disabled(X0_MASK);
        }
        else
        {
            ui->x0LineEdit->setStyleSheet("color: green");
            ui->x0Label->setText("");
            add_enabled(X0_MASK);
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
            add_disabled(Y0_MASK);
        }
        else
        {
            ui->y0LineEdit->setStyleSheet("color: green");
            ui->y0Label->setText("");
            add_enabled(Y0_MASK);
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
            add_disabled(XN_MASK);
        }
        else
        {
            ui->xnLineEdit->setStyleSheet("color: green");
            ui->xnLabel->setText("");
            add_enabled(XN_MASK);
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
            add_disabled(YN_MASK);
        }
        else
        {
            ui->ynLineEdit->setStyleSheet("color: green");
            ui->ynLabel->setText("");
            add_enabled(YN_MASK);
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
            add_disabled(XS_MASK);
        }
        else
        {
            ui->xsLineEdit->setStyleSheet("color: green");
            ui->xsLabel->setText("");
            add_enabled(XS_MASK);
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
            add_disabled(YS_MASK);
        }
        else
        {
            ui->ysLineEdit->setStyleSheet("color: green");
            ui->ysLabel->setText("");
            add_enabled(YS_MASK);
        }
    });

    connect(ui->z0LineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString z0_size_str = ui->z0LineEdit->text();
        double z0_size = z0_size_str.toDouble(&is_double);
        if (!is_double || z0_size < z_controller->get_min_position() || z0_size > z_controller->get_max_position())
        {
            ui->z0LineEdit->setStyleSheet("color: red");
            ui->z0Label->setStyleSheet("color: red");
            std::string info = "position should be valid double from " + std::to_string(z_controller->get_min_position()) + " to " + std::to_string(z_controller->get_max_position());
            ui->z0Label->setText(QString::fromStdString(info));
            add_disabled(Z0_MASK);
        }
        else
        {
            ui->z0LineEdit->setStyleSheet("color: green");
            ui->z0Label->setText("");
            add_enabled(Z0_MASK);
        }
    });

    connect(ui->znLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString zn_size_str = ui->znLineEdit->text();
        double zn_size = zn_size_str.toDouble(&is_double);
        if (!is_double || zn_size < z_controller->get_min_position() || zn_size > z_controller->get_max_position())
        {
            ui->znLineEdit->setStyleSheet("color: red");
            ui->znLabel->setStyleSheet("color: red");
            std::string info = "position should be valid double from " + std::to_string(z_controller->get_min_position()) + " to " + std::to_string(z_controller->get_max_position());
            ui->znLabel->setText(QString::fromStdString(info));
            add_disabled(ZN_MASK);
        }
        else
        {
            ui->znLineEdit->setStyleSheet("color: green");
            ui->znLabel->setText("");
            add_enabled(ZN_MASK);
        }
    });

    connect(ui->zsLineEdit, &QLineEdit::textEdited, this, [this]
    {
        bool is_double;
        QString zs_size_str = ui->zsLineEdit->text();
        double zs_size = zs_size_str.toDouble(&is_double);
        if (!is_double || zs_size < 0 || zs_size > z_controller->get_max_position() - z_controller->get_min_position())
        {
            ui->zsLineEdit->setStyleSheet("color: red");
            ui->zsLabel->setStyleSheet("color: red");
            std::string info = "size should be valid positive double not more " + std::to_string(z_controller->get_max_position() - z_controller->get_min_position());
            ui->zsLabel->setText(QString::fromStdString(info));
            add_disabled(ZS_MASK);
        }
        else
        {
            ui->zsLineEdit->setStyleSheet("color: green");
            ui->zsLabel->setText("");
            add_enabled(ZS_MASK);
        }
    });

    connect(ui->setCurrentPosition0Button, &QPushButton::clicked, this, [this]
    {
        ui->x0LineEdit->setText(ui->xCur->text());
        ui->y0LineEdit->setText(ui->yCur->text());
        ui->x0LineEdit->setStyleSheet("color: green");
        ui->x0Label->setText("");
        add_enabled(X0_MASK);
        ui->y0LineEdit->setStyleSheet("color: green");
        ui->y0Label->setText("");
        add_enabled(Y0_MASK);

    });

    connect(ui->setCurrentPositionNButton, &QPushButton::clicked, this, [this]
    {
        ui->xnLineEdit->setText(ui->xCur->text());
        ui->ynLineEdit->setText(ui->yCur->text());
        ui->xnLineEdit->setStyleSheet("color: green");
        ui->xnLabel->setText("");
        add_enabled(XN_MASK);
        ui->ynLineEdit->setStyleSheet("color: green");
        ui->ynLabel->setText("");
        add_enabled(YN_MASK);
    });

    connect(ui->setCurrentPositionZ0Button, &QPushButton::clicked, this, [this]
    {
        ui->z0LineEdit->setText(ui->zCur->text());
        ui->z0LineEdit->setStyleSheet("color: green");
        ui->z0Label->setText("");
        add_enabled(Z0_MASK);
    });

    connect(ui->setCurrentPositionZNButton, &QPushButton::clicked, this, [this]
    {
        ui->znLineEdit->setText(ui->zCur->text());
        ui->znLineEdit->setStyleSheet("color: green");
        ui->znLabel->setText("");
        add_enabled(ZN_MASK);
    });


    connect(ui->xMoveLeftButton, &QPushButton::clicked, this, [this]
    {
        disable_all();
        x_moving_left_clicked = true;
        x_moving_left_var.notify_one();
    });

    connect(ui->xMoveRightButton, &QPushButton::clicked, this, [this]
    {
        disable_all();
        x_moving_right_clicked = true;
        x_moving_right_var.notify_one();
    });

    connect(ui->yMoveLeftButton, &QPushButton::clicked, this, [this]
    {
        disable_all();
        y_moving_left_clicked = true;
        y_moving_left_var.notify_one();
    });

    connect(ui->yMoveRightButton, &QPushButton::clicked, this, [this]
    {
        disable_all();
        y_moving_right_clicked = true;
        y_moving_right_var.notify_one();
    });

    connect(ui->zMoveDownButton, &QPushButton::clicked, this, [this]
    {
        disable_all();
        z_moving_down_clicked = true;
        z_moving_down_var.notify_one();
    });

    connect(ui->zMoveUpButton, &QPushButton::clicked, this, [this]
    {
        disable_all();
        z_moving_up_clicked = true;
        z_moving_up_var.notify_one();
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
            ui->xCur->setText(QString::fromStdString(std::to_string(x_controller->get_current_position())));
        }
        if (y_controller != nullptr)
        {
            QString logs = QString().fromStdString(y_controller->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:green\">" + logs + "</span>";
                ui->logs->append(logs);
            }
            ui->yCur->setText(QString::fromStdString(std::to_string(y_controller->get_current_position())));
        }
        if (z_controller != nullptr)
        {
            QString logs = QString().fromStdString(z_controller->get_logs());
            if (logs != "")
            {
                logs = "<span style=\"color:blue\">" + logs + "</span>";
                ui->logs->append(logs);
            }
            ui->zCur->setText(QString::fromStdString(std::to_string(z_controller->get_current_position())));
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
            x_controller = new pi_controller("x", "M-511.DD");
            int x_port = x_controller->connect_vid_usb(0);
            y_controller = new pi_controller("y", "M-511.DD");
            int y_port = y_controller->connect_vid_usb(x_port + 1);
            z_controller = new pi_controller("z", "M-501.1DG");
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
            ui->z0LineEdit->setEnabled(true);
            ui->znLineEdit->setEnabled(true);
            ui->zsLineEdit->setEnabled(true);
            ui->setCurrentPosition0Button->setEnabled(true);
            ui->setCurrentPositionNButton->setEnabled(true);
            ui->setCurrentPositionZ0Button->setEnabled(true);
            ui->setCurrentPositionZNButton->setEnabled(true);
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
            add_enabled(CM_MASK);
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
        double x0_size = ui->x0LineEdit->text().toDouble();
        double y0_size = ui->y0LineEdit->text().toDouble();
        double xN_size = ui->xnLineEdit->text().toDouble();
        double yN_size = ui->ynLineEdit->text().toDouble();
        if (x0_size > xN_size)
        {
            ui->x0LineEdit->setText(QString::fromStdString(std::to_string(xN_size)));
            ui->xnLineEdit->setText(QString::fromStdString(std::to_string(x0_size)));
        }
        if (y0_size > yN_size)
        {
            ui->y0LineEdit->setText(QString::fromStdString(std::to_string(yN_size)));
            ui->ynLineEdit->setText(QString::fromStdString(std::to_string(y0_size)));
        }
        ui->moveToStartPositionButton->setEnabled(false);
        ui->startScanningButton->setEnabled(false);
        scanning_clicked = true;
        scanning_var.notify_one();
    });

    connect(ui->cancelScanningButton, &QPushButton::clicked, this, [this]
    {
        is_cancelled.store(true);
    });

    connect(ui->browseButton, &QPushButton::clicked, this, [this]
    {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open directory"), "C://", QFileDialog::ShowDirsOnly);
        ui->pathLine->setText(dir);
        add_enabled(BR_MASK);
    });
}

PI::~PI()
{
    delete ui;
}

void PI::capture_and_save(size_t x_pos, size_t y_pos)
{
    std::string dir_to_save = ui->pathLine->text().toStdString() + "/" + std::to_string(x_pos) + "_" + std::to_string(y_pos);
    QDir().mkdir(QString::fromStdString(dir_to_save));
    camera->capture(dir_to_save, z_controller, ui->z0LineEdit->text().toDouble(), ui->znLineEdit->text().toDouble(), ui->zsLineEdit->text().toDouble());
}

void PI::add_enabled(int mask)
{
    button_mask |= mask;
    if ((button_mask & (X0_MASK | Y0_MASK)) == (X0_MASK | Y0_MASK))
    {
        ui->moveToStartPositionButton->setEnabled(true);
    }
    if ((button_mask & XS_MASK) == XS_MASK)
    {
        ui->xMoveLeftButton->setEnabled(true);
        ui->xMoveRightButton->setEnabled(true);
    }
    if ((button_mask & YS_MASK) == YS_MASK)
    {
        ui->yMoveLeftButton->setEnabled(true);
        ui->yMoveRightButton->setEnabled(true);
    }
    if ((button_mask & ZS_MASK) == ZS_MASK)
    {
        ui->zMoveUpButton->setEnabled(true);
        ui->zMoveDownButton->setEnabled(true);
    }
    if (button_mask == (X0_MASK | XN_MASK | Y0_MASK | YN_MASK | Z0_MASK | ZN_MASK | XS_MASK | YS_MASK | ZS_MASK | BR_MASK | CM_MASK))
    {
        ui->startScanningButton->setEnabled(true);
    }
}


void PI::add_disabled(int mask)
{
    button_mask &= ~mask;
    ui->startScanningButton->setEnabled(false);
    if ((button_mask & (X0_MASK | Y0_MASK)) != (X0_MASK | Y0_MASK))
    {
        ui->moveToStartPositionButton->setEnabled(false);
    }
    if ((button_mask & XS_MASK) != XS_MASK)
    {
        ui->xMoveLeftButton->setEnabled(false);
        ui->xMoveRightButton->setEnabled(false);
    }
    if ((button_mask & YS_MASK) != YS_MASK)
    {
        ui->yMoveLeftButton->setEnabled(false);
        ui->yMoveRightButton->setEnabled(false);
    }
    if ((button_mask & ZS_MASK) != ZS_MASK)
    {
        ui->zMoveUpButton->setEnabled(false);
        ui->zMoveDownButton->setEnabled(false);
    }
}

void PI::disable_all()
{
    ui->xMoveLeftButton->setEnabled(false);
    ui->xMoveRightButton->setEnabled(false);
    ui->yMoveLeftButton->setEnabled(false);
    ui->yMoveRightButton->setEnabled(false);
    ui->zMoveDownButton->setEnabled(false);
    ui->zMoveUpButton->setEnabled(false);
}

void PI::enable_all()
{
    if ((button_mask & XS_MASK) == XS_MASK)
    {
        ui->xMoveLeftButton->setEnabled(true);
        ui->xMoveRightButton->setEnabled(true);
    }
    if ((button_mask & YS_MASK) == YS_MASK)
    {
        ui->yMoveLeftButton->setEnabled(true);
        ui->yMoveRightButton->setEnabled(true);
    }
    if ((button_mask & ZS_MASK) == ZS_MASK)
    {
        ui->zMoveDownButton->setEnabled(true);
        ui->zMoveUpButton->setEnabled(true);
    }
}
