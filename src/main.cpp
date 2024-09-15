#include "headsetcontrolqt.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef _WIN32
    a.setStyle("fusion");
#endif
    a.setQuitOnLastWindowClosed(false);
    HeadsetControlQt w;
    return a.exec();

}
