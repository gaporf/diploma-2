#include "findaxes.h"
#include "ui_findaxes.h"

FindAxes::FindAxes(QWidget *parent, pi_controller **x, pi_controller **y, pi_controller **z) :
    QDialog(parent),
    ui(new Ui::FindAxes),
    x_controller(x),
    y_controller(y),
    z_controller(z),
    x0_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                x0_var.wait(lg, [this]
                {
                    return x0_clicked;
                });
                (*x_controller)->set_velocity(15);
                (*x_controller)->move((*x_controller)->get_min_position());
                x0_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    xN_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                xN_var.wait(lg, [this]
                {
                    return xN_clicked;
                });
                (*x_controller)->set_velocity(15);
                (*x_controller)->move((*x_controller)->get_max_position());
                xN_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    y0_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                y0_var.wait(lg, [this]
                {
                    return y0_clicked;
                });
                (*y_controller)->set_velocity(15);
                (*y_controller)->move((*y_controller)->get_min_position());
                y0_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    yN_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                yN_var.wait(lg, [this]
                {
                    return yN_clicked;
                });
                (*y_controller)->set_velocity(15);
                (*y_controller)->move((*y_controller)->get_max_position());
                yN_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    z0_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                z0_var.wait(lg, [this]
                {
                    return z0_clicked;
                });
                (*z_controller)->set_velocity(15);
                (*z_controller)->move((*z_controller)->get_min_position());
                z0_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    zN_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                zN_var.wait(lg, [this]
                {
                    return zN_clicked;
                });
                (*z_controller)->set_velocity(15);
                (*z_controller)->move((*z_controller)->get_max_position());
                zN_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    xs_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                xs_var.wait(lg, [this]
                {
                    return xs_clicked;
                });
                double cur_position = (*x_controller)->get_current_position();
                (*x_controller)->set_velocity(5);
                (*x_controller)->move(std::max((*x_controller)->get_min_position(), cur_position - 5));
                (*x_controller)->move(std::min((*x_controller)->get_max_position(), cur_position + 5));
                (*x_controller)->move(cur_position);
                xs_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    ys_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                ys_var.wait(lg, [this]
                {
                    return ys_clicked;
                });
                double cur_position = (*y_controller)->get_current_position();
                (*y_controller)->set_velocity(5);
                (*y_controller)->move(std::max((*y_controller)->get_min_position(), cur_position - 5));
                (*y_controller)->move(std::min((*y_controller)->get_max_position(), cur_position + 5));
                (*y_controller)->move(cur_position);
                ys_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    zs_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                zs_var.wait(lg, [this]
                {
                    return zs_clicked;
                });
                double cur_position = (*z_controller)->get_current_position();
                (*z_controller)->set_velocity(5);
                (*z_controller)->move(std::max((*z_controller)->get_min_position(), cur_position - 5));
                (*z_controller)->move(std::min((*z_controller)->get_max_position(), cur_position + 5));
                (*z_controller)->move(cur_position);
                zs_clicked =false;
                enable_all();
                lg.unlock();
            }
        }),
    xy_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                xy_var.wait(lg, [this]
                {
                    return xy_clicked;
                });
                std::string x_name = (*x_controller)->get_axis_name();
                std::string y_name = (*y_controller)->get_axis_name();
                std::swap(*x_controller, *y_controller);
                (*x_controller)->set_axis_name(x_name);
                (*y_controller)->set_axis_name(y_name);
                xy_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    xz_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                xz_var.wait(lg, [this]
                {
                    return xz_clicked;
                });
                std::string x_name = (*x_controller)->get_axis_name();
                std::string z_name = (*z_controller)->get_axis_name();
                std::swap(*x_controller, *z_controller);
                (*x_controller)->set_axis_name(x_name);
                (*z_controller)->set_axis_name(z_name);
                xz_clicked = false;
                enable_all();
                lg.unlock();
            }
        }),
    yz_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                yz_var.wait(lg, [this]
                {
                    return yz_clicked;
                });
                std::string y_name = (*y_controller)->get_axis_name();
                std::string z_name = (*z_controller)->get_axis_name();
                std::swap(*y_controller, *z_controller);
                (*y_controller)->set_axis_name(y_name);
                (*z_controller)->set_axis_name(z_name);
                yz_clicked = false;
                enable_all();
                lg.unlock();
            }
        })
{
    ui->setupUi(this);

    connect(ui->moveXTo0, &QPushButton::clicked, this, [this]
    {
        disable_all();
        x0_clicked = true;
        x0_var.notify_one();
    });

    connect(ui->moveXToN, &QPushButton::clicked, this, [this]
    {
        disable_all();
        xN_clicked = true;
        xN_var.notify_one();
    });

    connect(ui->showX, &QPushButton::clicked, this, [this]
    {
        disable_all();
        xs_clicked = true;
        xs_var.notify_one();
    });

    connect(ui->moveYTo0, &QPushButton::clicked, this, [this]
    {
        disable_all();
        y0_clicked = true;
        y0_var.notify_one();
    });

    connect(ui->moveYToN, &QPushButton::clicked, this, [this]
    {
        disable_all();
        yN_clicked = true;
        yN_var.notify_one();
    });

    connect(ui->showY, &QPushButton::clicked, this, [this]
    {
        disable_all();
        ys_clicked = true;
        ys_var.notify_one();
    });

    connect(ui->swapXY, &QPushButton::clicked, this, [this]
    {
        disable_all();
        xy_clicked = true;
        xy_var.notify_one();
    });

    connect(ui->swapXZ, &QPushButton::clicked, this, [this]
    {
        disable_all();
        xz_clicked = true;
        xz_var.notify_one();
    });

    connect(ui->swapYZ, &QPushButton::clicked, this, [this]
    {
        disable_all();
        yz_clicked = true;
        yz_var.notify_one();
    });
}

FindAxes::~FindAxes()
{
    delete ui;
}

void FindAxes::enable_all()
{
    ui->moveXTo0->setEnabled(true);
    ui->moveXToN->setEnabled(true);
    ui->showX->setEnabled(true);
    ui->moveYTo0->setEnabled(true);
    ui->moveYToN->setEnabled(true);
    ui->showY->setEnabled(true);
    ui->moveZTo0->setEnabled(true);
    ui->moveZToN->setEnabled(true);
    ui->showZ->setEnabled(true);
    ui->swapXY->setEnabled(true);
    ui->swapXZ->setEnabled(true);
    ui->swapYZ->setEnabled(true);
}

void FindAxes::disable_all()
{
    ui->moveXTo0->setEnabled(false);
    ui->moveXToN->setEnabled(false);
    ui->showX->setEnabled(false);
    ui->moveYTo0->setEnabled(false);
    ui->moveYToN->setEnabled(false);
    ui->showY->setEnabled(false);
    ui->moveZTo0->setEnabled(false);
    ui->moveZToN->setEnabled(false);
    ui->showZ->setEnabled(false);
    ui->swapXY->setEnabled(false);
    ui->swapXZ->setEnabled(false);
    ui->swapYZ->setEnabled(false);
}
