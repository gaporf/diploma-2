#include "pi.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PI w;
    w.show();
    return a.exec();
}
