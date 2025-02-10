#ifndef UTILS_H
#define UTILS_H

#include <QString>

namespace Utils {
    QString getBatteryIconPath(int batteryLevel, bool charging, bool missing, int themeIndex);
    QString getTheme();
}

#endif // UTILS_H
