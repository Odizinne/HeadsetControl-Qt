#include "HeadsetControlQt.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    QApplication a(argc, argv);
#ifdef _WIN32
    a.setStyle("fusion");
#endif
    a.setQuitOnLastWindowClosed(false);
    a.setOrganizationName("Odizinne");
    a.setApplicationName("HeadsetControlQt");
    QApplication::setWindowIcon(QIcon(":/icons/icon.png"));
    HeadsetControlQt w;
    return a.exec();
}
