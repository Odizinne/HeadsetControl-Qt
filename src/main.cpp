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
    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);
    QTranslator translator;
    if (translator.load(":/translations/tr/HeadsetControl-Qt_" + languageCode + ".qm")) {
        a.installTranslator(&translator);
    }
    HeadsetControlQt w;
    return a.exec();

}
