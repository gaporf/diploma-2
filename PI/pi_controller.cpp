#include <iostream>

#include "pi_controller.h"

pi_controller::pi_controller(const std::string &axis, const std::string &stage) : axis(axis), stage(stage), lib("PI_GCS2_DLL_x64.dll") {}

pi_controller::~pi_controller()
{
    push_log("Closing the connection");
    typedef long (*FuncPtr)(long id);
    FuncPtr func = (FuncPtr) lib.resolve("PI_CloseConnection");
    if (func == nullptr)
    {
        push_log("Couldn't find PI_CloseConnection function in dll");
    }
    int status = func(this->id);
    if (status != 0)
    {
        push_log("Couldn't close");
    }
    push_log("The connection is closed");
}

std::string pi_controller::get_logs()
{
    std::string res;
    while (!logs.empty())
    {
        res += logs.front();
        logs.pop();
    }
    return res;
}

void pi_controller::push_log(std::string log)
{
    if (log[log.length() - 1] == '\n')
    {
        log = log.substr(0, log.size() - 1);
    }
    logs.push(axis + ": " + log + "<br>");
}

int pi_controller::connect_vid_usb(int initial_port)
{
    push_log("Looking for USB ports");
    typedef long (*FuncPtr)(long nPortNr, long iBaudRate);
    FuncPtr func = (FuncPtr) lib.resolve("PI_ConnectRS232");
    if (func == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_ConnectRS232 function in dll");
    }
    for (int port = initial_port; port < 100; port++)
    {
        int id = func(port, 38400);
        if (id >= 0)
        {
            this->port = port;
            this->id = id;
            push_log("USB port id = " + std::to_string(port) + ", id = " + std::to_string(id));
            return port;
        }
    }
    throw std::runtime_error("Couldn't find connected PI controller");
}

void pi_controller::get_controller_info()
{
    push_log("Getting info about PI controller");
    char buf[255];
    typedef int (*FuncPtr)(long ID, char* szBuffer, int iBufferSize);
    FuncPtr func = (FuncPtr) lib.resolve("PI_qIDN");
    if (func == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qIDN function in dll");
    }
    {
        int res = func(id, buf, 255);
        if (res == 0)
        {
            throw std::runtime_error("Couldn't get information from PI controller");
        }
        push_log("Connected to " + std::string(buf));
    }
}

void pi_controller::get_axes_info()
{
    push_log("Getting info about axes");
    char buf[255];
    typedef int (*FuncPtr)(long ID, char* szAxes, int iBufferSize);
    FuncPtr func = (FuncPtr) lib.resolve("PI_qSAI_ALL");
    if (func == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qSAI_ALL function in dll");
    }
    {
        int res = func(id, buf, 255);
        if (res == 0)
        {
            throw std::runtime_error("Couldn't get information about axes");
        }
        push_log("Connected axes: " + std::string(buf));
    }
}

void pi_controller::init_stage()
{
    push_log("Initializing stage");
    const char* axes = "1";
    char buf[255];

    typedef int (*FuncPI_qCST)(long ID, const char* szAxes, char* szNames, int iBufferSize);
    FuncPI_qCST PI_qCST = (FuncPI_qCST) lib.resolve("PI_qCST");
    if (PI_qCST == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qCST function in dll");
    }
    {
        int res = PI_qCST(id, axes, buf, 255);
        if (res == 0)
        {
            throw std::runtime_error("Couldnt get information about stage definition");
        }
        const char* ptr_stage = strchr(buf, '=');
        ptr_stage++;
        if (strnicmp(stage.c_str(), ptr_stage, stage.length()) == 0)
        {
            push_log("stage type is already defined");
            return;
        }
    }

    typedef int (*FuncPI_CST)(long ID, const char* szAxes, const char* szNames);
    FuncPI_CST PI_CST = (FuncPI_CST) lib.resolve("PI_CST");
    if (PI_CST == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_CST function in dll");
    }
    {
        int res = PI_CST(id, axes, stage.c_str());
        if (res == 0)
        {
            throw std::runtime_error("CST failed");
        }
    }
}

void pi_controller::reference()
{
    push_log("Referencing axes");
    const char* axes = "1";
    int b_flag = true;

    typedef int (*FuncPI_SVO)(long ID, const char* szAxes, const int* pbValueArray);
    FuncPI_SVO PI_SVO = (FuncPI_SVO) lib.resolve("PI_SVO");
    if (PI_SVO == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_SVO function in dll");
    }
    {
        int res = PI_SVO(id, axes, &b_flag);
        if (res == 0)
        {
            throw std::runtime_error("SVO failed");
        }
    }

    typedef int (*FuncPI_qFRF)(long ID, const char* szAxes, int* pbValueArray);
    FuncPI_qFRF PI_qFRF = (FuncPI_qFRF) lib.resolve("PI_qFRF");
    if (PI_qFRF == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qFRF function in dll");
    }
    {
        int res = PI_qFRF(id, axes, &b_flag);
        if (res == 0)
        {
            throw std::runtime_error("qFRF failed");
        }
    }
    if (b_flag)
    {
        push_log("axes are already references");
        return;
    }

    typedef int (*FuncPI_qTRS)(long ID, const char* szAxes, int* pbValueArray);
    FuncPI_qTRS PI_qTRS = (FuncPI_qTRS) lib.resolve("PI_qTRS");
    if (PI_qTRS == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qTRS function in dll");
    }
    {
        int res = PI_qTRS(id, axes, &b_flag);
        if (res == 0)
        {
            throw std::runtime_error("qTRS failed");
        }
    }
    if (b_flag)
    {
        typedef int (*FuncPI_FRF)(long ID, const char* szAxes);
        FuncPI_FRF PI_FRF = (FuncPI_FRF) lib.resolve("PI_FRF");
        if (PI_FRF == nullptr)
        {
            throw std::runtime_error("Couldn't find PI_FRF function in dll");
        }
        {
            int res = PI_FRF(id, axes);
            if (res == 0)
            {
                throw std::runtime_error("REF failed");
            }
        }
        push_log("Stage has reference switch");
    }
    else
    {
        typedef int (*FuncPI_qLIM)(long ID, const char* szAxes, int* pbValueArray);
        FuncPI_qLIM PI_qLIM = (FuncPI_qLIM) lib.resolve("PI_qLIM");
        if (PI_qLIM == nullptr)
        {
            throw std::runtime_error("Couldn't find PI_qLIM function in dll");
        }
        {
            int res = PI_qLIM(id, axes, &b_flag);
            if (res == 0)
            {
                throw std::runtime_error("qLIM failed");
            }
        }
        if (b_flag)
        {
            typedef int (*FuncPI_FNL)(long ID, const char* szAxes);
            FuncPI_FNL PI_FNL = (FuncPI_FNL) lib.resolve("PI_FNL");
            if (PI_FNL == nullptr)
            {
                throw std::runtime_error("Couldn't find PI_FNL function in dll");
            }
            {
                int res = PI_FNL(id, axes);
                if (res == 0)
                {
                    throw std::runtime_error("MNL failed");
                }
            }
            push_log("Reference stage for axis " + std::string(axes) + " by negative limit switch");
        }
        else
        {
            throw std::runtime_error("Stage for axis 1 has no reference nor limit switch");
        }
    }

    typedef int (*FuncPI_IsControllerReady)(long ID, int* piControllerReady);
    FuncPI_IsControllerReady PI_IsControllerReady = (FuncPI_IsControllerReady) lib.resolve("PI_IsControllerReady");
    if (PI_IsControllerReady == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_IsControllerReady function in dll");
    }
    do
    {
        _sleep(1000);
        {
            int res = PI_IsControllerReady(id, &b_flag);
            if (res == 0)
            {
                throw std::runtime_error("IsControllerReady failed");
            }
            push_log(".");
        }
    }
    while (!b_flag);
    push_log("");
    {
        int res = PI_qFRF(id, axes, &b_flag);
        if (res == 0)
        {
            throw std::runtime_error("qFRF failed");
        }
    }
    if (b_flag)
    {
        push_log("Axis " + std::string(axes) + " is successfully referenced!");
        return;
    }
    else
    {
        throw std::runtime_error("Axis 1 is not referenced!");
    }
}

void pi_controller::get_limits()
{
    const char *axes = "1";

    typedef int (*FuncPI_qTMN)(long ID, const char* szAxes, double* pdValueArray);
    FuncPI_qTMN PI_qTMN = (FuncPI_qTMN) lib.resolve("PI_qTMN");
    if (PI_qTMN == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qTMN function in dll");
    }
    {
        int res = PI_qTMN(id, axes, &min_position);
        if (res == 0)
        {
            throw std::runtime_error("qTMN failed");
        }
    }

    typedef int (*FuncPI_qTMX)(long ID, const char* szAxes, double* pdValueArray);
    FuncPI_qTMX PI_qTMX = (FuncPI_qTMX) lib.resolve("PI_qTMX");
    if (PI_qTMX == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qTMX function in dll");
    }
    {
        int res = PI_qTMX(id, axes, &max_position);
        if (res == 0)
        {
            throw std::runtime_error("qTMX failed");
        }
    }
    push_log("min position = " + std::to_string(min_position) + ", max position = " + std::to_string(max_position));
}

void pi_controller::move(double target)
{
    const char *axes = "1";
    typedef int (*FuncPI_MOV)(long ID, const char* szAxes, const double* pdValueArray);
    FuncPI_MOV PI_MOV = (FuncPI_MOV) lib.resolve("PI_MOV");
    if (PI_MOV == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_MOV function in dll");
    }
    {
        int res = PI_MOV(id, axes, &target);
        if (res == 0)
        {
            throw std::runtime_error("MOV failed");
        }
    }
    int b_flag = true;
    double position;
    typedef int (*FuncPI_IsMoving)(long ID, const char* szAxes, int* pbValueArray);
    FuncPI_IsMoving PI_IsMoving = (FuncPI_IsMoving) lib.resolve("PI_IsMoving");
    if (PI_IsMoving == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_IsMoving in dll");
    }
    typedef int (*FuncPI_qPos)(long ID, const char* szAxes, double* pdValueArray);
    FuncPI_qPos PI_qPos = (FuncPI_qPos) lib.resolve("PI_qPOS");
    if (PI_qPos == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qPOS in dll");
    }

    do
    {
        _sleep(1000);
        {
            int res = PI_IsMoving(id, axes, &b_flag);
            if (res == 0)
            {
                throw std::runtime_error("IsMoving failed");
            }
        }
        {
            int res = PI_qPos(id, axes, &position);
            if (res == 0)
            {
                throw std::runtime_error("qPos failed");
            }
        }
        push_log("Current position = " + std::to_string(position));
    }
    while (std::abs(position - target) > 1e-3);
    push_log("On the final position = " + std::to_string(target));
}

void pi_controller::set_velocity(double velocity)
{
    const char *axes = "1";
    typedef int (*FuncPI_VEL)(long ID, const char* szAxes, const double* pdValueArray);
    FuncPI_VEL PI_VEL = (FuncPI_VEL) lib.resolve("PI_VEL");
    if (PI_VEL == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_VEL function in dll");
    }
    {
        int res = PI_VEL(id, axes, &velocity);
        if (res == 0)
        {
            throw std::runtime_error("PI_VEL failed");
        }
    }
}

int pi_controller::get_port()
{
    return port;
}

double pi_controller::get_min_position()
{
    return min_position;
}

double pi_controller::get_max_position()
{
    return max_position;
}

std::string pi_controller::get_axis_name()
{
    return axis;
}

void pi_controller::set_axis_name(std::string new_axis)
{
    push_log("Changed axis from " + axis + " to " + new_axis);
    axis = new_axis;
}

double pi_controller::get_current_position()
{
    double position;
    const char *axes = "1";
    typedef int (*FuncPI_qPos)(long ID, const char* szAxes, double* pdValueArray);
    FuncPI_qPos PI_qPos = (FuncPI_qPos) lib.resolve("PI_qPOS");
    if (PI_qPos == nullptr)
    {
        throw std::runtime_error("Couldn't find PI_qPOS in dll");
    }
    int res = PI_qPos(id, axes, &position);
    if (res == 0)
    {
        throw std::runtime_error("qPos failed");
    }
    return position;
}
