#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QShortcut>
#include <QStandardPaths>
#include <QSysInfo>
#include <QtWidgets/QWidget>

namespace ShortcutManager {

bool isShortcutPresent()
{
#ifdef _WIN32
    QString shortcutName = "HeadsetControl-Qt.lnk";
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QDir::separator() + "Startup";
    QString shortcutPath = startupPath + QDir::separator() + shortcutName;
    return QFile::exists(shortcutPath);
#elif __linux__
    QString desktopFile = QDir::homePath() + "/.config/autostart/headsetcontrol-qt.desktop";
    return QFile::exists(desktopFile);
#else
    return false;
#endif
}

void manageShortcut(bool state)
{
#ifdef _WIN32
    QString shortcutName = "HeadsetControl-Qt.lnk";
    QString applicationPath = QCoreApplication::applicationFilePath();
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QDir::separator() + "Startup";
    QString shortcutPath = startupPath + QDir::separator() + shortcutName;

    if (state) {
        QFile::link(applicationPath, shortcutPath);
    } else {
        QFile::remove(shortcutPath);
    }
#elif __linux__
    QString desktopFile = QDir::homePath() + "/.config/autostart/headsetcontrol-qt.desktop";

    if (state) {
        QFileInfo fileInfo(desktopFile);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString applicationFolder = QCoreApplication::applicationDirPath();
        QString desktopEntryContent =
            "[Desktop Entry]\n"
            "Path=" + applicationFolder + "\n"
                                  "Type=Application\n"
                                  "Exec=" + QCoreApplication::applicationFilePath() + "\n"
                                                        "Name=HeadsetControl-Qt\n";

        QFile file(desktopFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << desktopEntryContent;
            file.close();
        }
    } else {
        QFile file(desktopFile);
        if (file.exists()) {
            file.remove();
        }
    }
#endif
}

} // namespace ShortcutManager
