#ifndef FINDAXES_H
#define FINDAXES_H

#include <QDialog>

#include <thread>
#include <condition_variable>
#include <mutex>

#include "pi_controller.h"
#include "ui_findaxes.h"

namespace Ui {
class FindAxes;
}

enum buttons
{
    X0,
    XN,
    Y0,
    YN,
    Z0,
    ZN,
    XS,
    YS,
    ZS,
    XY,
    XZ,
    YZ
};

class FindAxes : public QDialog
{
    Q_OBJECT

public:
    FindAxes(QWidget *parent = nullptr, pi_controller **x = nullptr, pi_controller **y = nullptr, pi_controller **z = nullptr);

    ~FindAxes();

private:
    Ui::FindAxes *ui;
    pi_controller **x_controller;
    pi_controller **y_controller;
    pi_controller **z_controller;

    std::mutex m;

    std::thread x0_thread;
    std::condition_variable x0_var;
    bool x0_clicked = false;

    std::thread xN_thread;
    std::condition_variable xN_var;
    bool xN_clicked = false;

    std::thread y0_thread;
    std::condition_variable y0_var;
    bool y0_clicked = false;

    std::thread yN_thread;
    std::condition_variable yN_var;
    bool yN_clicked = false;

    std::thread z0_thread;
    std::condition_variable z0_var;
    bool z0_clicked = false;

    std::thread zN_thread;
    std::condition_variable zN_var;
    bool zN_clicked = false;

    std::thread xs_thread;
    std::condition_variable xs_var;
    bool xs_clicked = false;

    std::thread ys_thread;
    std::condition_variable ys_var;
    bool ys_clicked = false;

    std::thread zs_thread;
    std::condition_variable zs_var;
    bool zs_clicked = false;

    std::thread xy_thread;
    std::condition_variable xy_var;
    bool xy_clicked = false;

    std::thread xz_thread;
    std::condition_variable xz_var;
    bool xz_clicked = false;

    std::thread yz_thread;
    std::condition_variable yz_var;
    bool yz_clicked = false;

    std::map<std::string, std::thread> threads;
    std::map<std::string, std::condition_variable> vars;
    std::map<std::string, bool> clicks;

    void disable_all();
    void enable_all();
};

#endif // FINDAXES_H
