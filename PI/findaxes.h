#ifndef FINDAXES_H
#define FINDAXES_H

#include <QDialog>

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
};

#endif // FINDAXES_H
