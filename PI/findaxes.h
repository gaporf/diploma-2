#ifndef FINDAXES_H
#define FINDAXES_H

#include <QDialog>

#include <thread>
#include <condition_variable>
#include <mutex>

#include "pi_controller.h"


namespace Ui {
class FindAxes;
}

class FindAxes : public QDialog
{
    Q_OBJECT

public:
    FindAxes(QWidget *parent = nullptr, pi_controller **x = nullptr, pi_controller **y = nullptr);
    ~FindAxes();

private:
    Ui::FindAxes *ui;
    pi_controller **x_controller;
    pi_controller **y_controller;

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

    std::thread xs_thread;
    std::condition_variable xs_var;
    bool xs_clicked = false;

    std::thread ys_thread;
    std::condition_variable ys_var;
    bool ys_clicked = false;

    std::thread ca_thread;
    std::condition_variable ca_var;
    bool ca_clicked = false;

    void disable_all();

    void enable_all();
};

#endif // FINDAXES_H
