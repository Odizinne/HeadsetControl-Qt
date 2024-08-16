#include "headsetcontrolqt.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("fusion");
    HeadsetControlQt w;
    return a.exec();
}
