#ifndef UTILS_H
#define UTILS_H

#include <QIcon>
#include <QString>
#include <QFrame>

namespace Utils {
    QString getBatteryIconPath(int batteryLevel, bool charging, bool missing, int themeIndex);
    QString getTheme();
    void setFrameColorBasedOnWindow(QWidget *window, QFrame *frame);
}

#endif // UTILS_H
