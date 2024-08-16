#include "headsetcontrolqt.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("fusion");
    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);
    QTranslator translator;
    if (translator.load(":/translations/tr/HeadsetControl-Qt_" + languageCode + ".qm")) {
        a.installTranslator(&translator);
    }
    HeadsetControlQt w;
    return a.exec();
}
