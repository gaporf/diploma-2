#include "findaxes.h"
#include "ui_findaxes.h"

FindAxes::FindAxes(QWidget *parent, pi_controller **x, pi_controller **y) :
    QDialog(parent),
    ui(new Ui::FindAxes),
    x_controller(x),
    y_controller(y),
    x0_thread([this]
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lg(m);
                x0_var.wait(lg, [this]
                {
                    return x0_clicked;
                });
                (*x_controller)->set_velocity(20);
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
                (*x_controller)->set_velocity(20);
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
                (*y_controller)->set_velocity(20);
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
                (*y_controller)->set_velocity(20);
                (*y_controller)->move((*y_controller)->get_max_position());
                yN_clicked = false;
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
        double cur_position = (*x_controller)->get_current_position();
        (*x_controller)->set_velocity(5);
        (*x_controller)->move(std::max((*x_controller)->get_min_position(), cur_position - 5));
        (*x_controller)->move(std::min((*x_controller)->get_max_position(), cur_position + 5));
        (*x_controller)->move(cur_position);
        enable_all();
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
        double cur_position = (*y_controller)->get_current_position();
        (*y_controller)->set_velocity(5);
        (*y_controller)->move(std::max((*y_controller)->get_min_position(), cur_position - 5));
        (*y_controller)->move(std::min((*y_controller)->get_max_position(), cur_position + 5));
        (*y_controller)->move(cur_position);
        enable_all();
    });

    connect(ui->changeAxes, &QPushButton::clicked, this, [this]
    {
       disable_all();
       std::string x_name = (*x_controller)->get_axis_name();
       std::string y_name = (*y_controller)->get_axis_name();
       std::swap(*x_controller, *y_controller);
       (*x_controller)->set_axis_name(x_name);
       (*y_controller)->set_axis_name(y_name);
        enable_all();
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
    ui->changeAxes->setEnabled(true);
}

void FindAxes::disable_all()
{
    ui->moveXTo0->setEnabled(false);
    ui->moveXToN->setEnabled(false);
    ui->showX->setEnabled(false);
    ui->moveYTo0->setEnabled(false);
    ui->moveYToN->setEnabled(false);
    ui->showY->setEnabled(false);
    ui->changeAxes->setEnabled(false);
}
