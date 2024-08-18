#include "utils.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileInfo>

QString getBatteryIcon(int batteryLevel, bool charging, bool missing, int themeIndex)
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
            if (kdeVersion.startsWith("5")) {
                qDebug() << "KDE Plasma 5 detected";
                theme = "light";
            } else if (kdeVersion.startsWith("6")) {
                qDebug() << "KDE Plasma 6 detected";
                theme = "symbolic";
            } else {
                qDebug() << "Unknown KDE Plasma version";
                theme = "light";
            }
        } else {
            theme = "dark"; // Fallback for non-KDE environments
        }
#endif
    } else if (themeIndex == 1) {
        theme = "light";
    } else if (themeIndex == 2) {
        theme = "dark";
    }

    QString iconName;
    if (missing) {
        iconName = QString("battery-missing-%1").arg(theme);
    } else if (charging) {
        iconName = "battery-100-charging-" + theme;
    } else {
        if (batteryLevel >= 90) iconName = "battery-100-" + theme;
        else if (batteryLevel >= 80) iconName = "battery-090-" + theme;
        else if (batteryLevel >= 70) iconName = "battery-080-" + theme;
        else if (batteryLevel >= 60) iconName = "battery-070-" + theme;
        else if (batteryLevel >= 50) iconName = "battery-060-" + theme;
        else if (batteryLevel >= 40) iconName = "battery-050-" + theme;
        else if (batteryLevel >= 30) iconName = "battery-040-" + theme;
        else if (batteryLevel >= 20) iconName = "battery-030-" + theme;
        else if (batteryLevel >= 10) iconName = "battery-020-" + theme;
        else iconName = "battery-010-" + theme;
    }

    QString iconPath;
    if (theme == "symbolic") {
        iconPath = iconName;
    } else {
        iconPath = QString(":/icons/%1.png").arg(iconName);
    }

    qDebug() << iconPath;
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
