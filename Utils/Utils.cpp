#include "Utils.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileInfo>
#include <QColor>
#include <QPalette>
#include <QBrush>
#include <QWidget>
#include <QFrame>
#include <algorithm>

QColor adjustColor(const QColor &color, double factor) {
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();

    r = std::min(std::max(static_cast<int>(r * factor), 0), 255);
    g = std::min(std::max(static_cast<int>(g * factor), 0), 255);
    b = std::min(std::max(static_cast<int>(b * factor), 0), 255);

    return QColor(r, g, b, a);
}

bool isDarkMode(const QColor &color) {
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    double brightness = (r + g + b) / 3.0;
    return brightness < 127;
}

void Utils::setFrameColorBasedOnWindow(QWidget *window, QFrame *frame) {
    QColor main_bg_color = window->palette().color(QPalette::Window);
    QColor frame_bg_color;

    if (isDarkMode(main_bg_color)) {
        frame_bg_color = adjustColor(main_bg_color, 1.75);  // Brighten color
    } else {
        frame_bg_color = adjustColor(main_bg_color, 0.95);  // Darken color
    }

    QPalette palette = frame->palette();
    palette.setBrush(QPalette::Window, QBrush(frame_bg_color));
    frame->setAutoFillBackground(true);
    frame->setPalette(palette);
}

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
