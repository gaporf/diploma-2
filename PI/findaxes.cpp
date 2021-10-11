#include "findaxes.h"
#include "ui_findaxes.h"

FindAxes::FindAxes(QWidget *parent, pi_controller **x, pi_controller **y) :
    QDialog(parent),
    ui(new Ui::FindAxes),
    x_controller(x),
    y_controller(y)
{
    ui->setupUi(this);

    connect(ui->moveXTo0, &QPushButton::clicked, this, [this]
    {
        (*x_controller)->set_velocity(20);
        (*x_controller)->move((*x_controller)->get_min_position());
    });

    connect(ui->moveXToN, &QPushButton::clicked, this, [this]
    {
        (*x_controller)->set_velocity(20);
        (*x_controller)->move((*x_controller)->get_max_position());
    });

    connect(ui->moveYTo0, &QPushButton::clicked, this, [this]
    {
        (*y_controller)->set_velocity(20);
        (*y_controller)->move((*y_controller)->get_min_position());
    });

    connect(ui->moveYToN, &QPushButton::clicked, this, [this]
    {
        (*y_controller)->set_velocity(20);
        (*y_controller)->move((*y_controller)->get_max_position());
    });

    connect(ui->ChangeAxes, &QPushButton::clicked, this, [this]
    {
       std::string x_name = (*x_controller)->get_axis_name();
       std::string y_name = (*y_controller)->get_axis_name();
       std::swap(*x_controller, *y_controller);
       (*x_controller)->set_axis_name(x_name);
       (*y_controller)->set_axis_name(y_name);
    });
}

FindAxes::~FindAxes()
{
    delete ui;
}
