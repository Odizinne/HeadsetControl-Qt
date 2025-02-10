#include "HeadsetControlQt.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef _WIN32
    a.setStyle("fusion");
#endif
    a.setQuitOnLastWindowClosed(false);
    a.setOrganizationName("Odizinne");
    a.setApplicationName("HeadsetControlQt");
    HeadsetControlQt w;
    return a.exec();
}
