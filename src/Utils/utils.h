#ifndef UTILS_H
#define UTILS_H
#include <QIcon>
#include <QString>
#include <QFrame>

QString getBatteryIconPath(int batteryLevel, bool charging, bool missing, int themeIndex);
QIcon getBatteryIcon(const QString &BatteryIconPath);
QString getTheme();
QString getKDEPlasmaVersion();
void setFrameColorBasedOnWindow(QWidget *window, QFrame *frame);
#endif // UTILS_H
