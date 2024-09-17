#include "utils.h"
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

void setFrameColorBasedOnWindow(QWidget *window, QFrame *frame) {
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

QString getBatteryIconPath(int batteryLevel, bool charging, bool missing, int themeIndex)
{
    QString theme;
    if (themeIndex == 0) {
#ifdef _WIN32
        theme = getTheme();
#elif __linux__
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString desktop = env.value("XDG_CURRENT_DESKTOP");
        if (desktop.contains("KDE", Qt::CaseInsensitive)) {
            QString kdeVersion = getKDEPlasmaVersion();
            if (kdeVersion.startsWith("6")) {
                theme = "symbolic";
            } else {
                theme = "light";
            }
        } else if (desktop.contains("GNOME", Qt::CaseInsensitive)) {
            theme = "light";
        } else {
            theme = "dark";
        }
#endif
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

    QString iconPath;
    if (theme == "symbolic") {
        iconPath = iconName;
    } else {
        iconPath = QString(":/icons/%1.png").arg(iconName);
    }

    return iconPath;
}


#ifdef _WIN32
QString getTheme()
{
    // Determine the theme based on registry value
    QSettings settings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    int value = settings.value("AppsUseLightTheme", 1).toInt();

    // Return the opposite to match icon (dark icon on light theme)
    return (value == 0) ? "light" : "dark";
}
#endif

#ifdef __linux__
QString getKDEPlasmaVersion() {
    QProcess process;
    process.start("plasmashell", QStringList() << "--version");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    QString versionLine;

    // Find the line containing the version info
    for (const QString &line : lines) {
        if (line.contains("plasmashell")) {
            versionLine = line;
            break;
        }
    }

    // Extract version number from the line
    QString version = versionLine.split(' ').last().trimmed();
    return version;
}
#endif

QIcon getBatteryIcon(const QString &BatteryIconPath)
{
#ifdef _WIN32
    return QIcon(BatteryIconPath);
#elif __linux__
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString desktop = env.value("XDG_CURRENT_DESKTOP");
    if (desktop.contains("KDE", Qt::CaseInsensitive)) {
        QString kdeVersion = getKDEPlasmaVersion();
        if (kdeVersion.startsWith("6")) {
            return QIcon::fromTheme(BatteryIconPath);
        } else {
            return QIcon(BatteryIconPath);
        }
    } else {
        return QIcon(BatteryIconPath);
    }
#endif
}
