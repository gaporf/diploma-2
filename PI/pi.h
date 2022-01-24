#ifndef PI_H
#define PI_H

#include <QTimer>
#include <QMainWindow>

#include <thread>
#include <condition_variable>
#include <mutex>

#include "findaxes.h"
#include "pi_controller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PI; }
QT_END_NAMESPACE

class PI : public QMainWindow
{
    Q_OBJECT

public:
    PI(QWidget *parent = nullptr);
    ~PI();

private:
    Ui::PI *ui;
    QTimer *timer = new QTimer(this);
    pi_controller *x_controller = nullptr;
    pi_controller *y_controller = nullptr;
    pi_controller *z_controller = nullptr;

    int button_mask = 0;

    std::mutex m;

    std::thread move_to_start_position_thread;
    std::condition_variable move_to_start_position_var;
    bool move_to_start_position_clicked = false;

    std::thread scanning_thread;
    std::condition_variable scanning_var;
    bool scanning_clicked = false;

    std::atomic_bool is_cancelled;

    void get_camera(double x_pos, double y_pos);
};
#endif // PI_H
