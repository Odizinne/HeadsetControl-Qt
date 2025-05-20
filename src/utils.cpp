#include "utils.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileInfo>

QString Utils::getBatteryIconPath(int batteryLevel, bool charging, bool missing, int themeIndex)
{
    QString theme;

    if (themeIndex == 0) {
        theme = getTheme();
    } else if (themeIndex == 1) {
        theme = "dark";
    } else if (themeIndex == 2) {
        theme = "light";
    }

    QString iconName;
    if (missing) {
        iconName = QString("battery-missing-%1").arg(theme);
    } else if (charging) {
        iconName = "battery-100-charging-" + theme;
    } else {
        if (batteryLevel >= 80) iconName = "battery-100-" + theme;
        else if (batteryLevel >= 60) iconName = "battery-080-" + theme;
        else if (batteryLevel >= 40) iconName = "battery-060-" + theme;
        else if (batteryLevel >= 20) iconName = "battery-040-" + theme;
        else if (batteryLevel >= 0) iconName = "battery-020-" + theme;
    }

    return QString(":/icons/%1.png").arg(iconName);;
}


QString Utils::getTheme()
{
#ifdef __linux__
    return "light";
#elif _WIN32
    // Determine the theme based on registry value
    QSettings settings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    int value = settings.value("AppsUseLightTheme", 1).toInt();

    // Return the opposite to match icon (dark icon on light theme)
    return (value == 0) ? "light" : "dark";
#endif
}
