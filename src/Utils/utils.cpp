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

QIcon getIconForTheme()
{
    QString theme = getTheme();
    QString iconPath = QString(":/icons/icon_%1.png").arg(theme);
    return QIcon(iconPath);
}

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
