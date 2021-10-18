#ifndef PI_H
#define PI_H

#include <QTimer>
#include <QMainWindow>

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
    int button_mask = 0;
};
#endif // PI_H
