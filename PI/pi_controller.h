#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

#include <queue>

#include <QLibrary>


struct pi_controller
{
    pi_controller(const std::string &axis, const std::string &stage);

    std::string get_logs();

    int connect_vid_usb(int initial_port);

    void get_controller_info();

    void get_axes_info();

    void init_stage();

    void reference();

    void get_limits();

    double get_min_position();

    double get_max_position();

    void move(double target);

    void set_velocity(double velocity);

    std::string get_axis_name();

    void set_axis_name(std::string new_axis);

    double get_current_position();
private:
    std::string axis;
    std::string stage;
    int port;
    long id;
    double min_position;
    double max_position;
    QLibrary lib;
    std::queue<std::string> logs;

    void push_log(std::string log);
};

#endif // PI_CONTROLLER_H
