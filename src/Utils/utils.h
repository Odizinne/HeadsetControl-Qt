#ifndef UTILS_H
#define UTILS_H
#include <QIcon>
#include <QString>

QString getBatteryIcon(int batteryLevel, bool charging, bool missing, int themeIndex);
QString getTheme();
QString getKDEPlasmaVersion();

#endif // UTILS_H
