#ifndef PI_H
#define PI_H

#include <QTimer>
#include <QMainWindow>

#include <thread>
#include <condition_variable>
#include <mutex>

#include "uEyeCamera.h"
#include "findaxes.h"
#include "findcamera.h"
#include "pi_controller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PI; }
QT_END_NAMESPACE

#define X0_MASK 0x001
#define Y0_MASK 0x002
#define XN_MASK 0x004
#define YN_MASK 0x008
#define XS_MASK 0x010
#define YS_MASK 0x020
#define Z0_MASK 0x040
#define ZN_MASK 0x080
#define ZS_MASK 0x100
#define CM_MASK 0x200
#define BR_MASK 0x400

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
    uEyeCamera *camera = nullptr;

    int button_mask = 0;

    void add_enabled(int mask);

    void add_disabled(int mask);

    void disable_all();

    void enable_all();

    std::mutex m;

    std::thread move_to_start_position_thread;
    std::condition_variable move_to_start_position_var;
    bool move_to_start_position_clicked = false;

    std::thread scanning_thread;
    std::condition_variable scanning_var;
    bool scanning_clicked = false;

    std::thread x_moving_left_thread;
    std::condition_variable x_moving_left_var;
    bool x_moving_left_clicked = false;

    std::thread x_moving_right_thread;
    std::condition_variable x_moving_right_var;
    bool x_moving_right_clicked = false;

    std::thread y_moving_left_thread;
    std::condition_variable y_moving_left_var;
    bool y_moving_left_clicked = false;

    std::thread y_moving_right_thread;
    std::condition_variable y_moving_right_var;
    bool y_moving_right_clicked = false;

    std::thread z_moving_down_thread;
    std::condition_variable z_moving_down_var;
    bool z_moving_down_clicked = false;

    std::thread z_moving_up_thread;
    std::condition_variable z_moving_up_var;
    bool z_moving_up_clicked = false;

    std::atomic_bool is_cancelled;

    void capture_and_save(size_t x_pos, size_t y_pos);
};
#endif // PI_H
