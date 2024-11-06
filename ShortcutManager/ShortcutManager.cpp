#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QShortcut>
#include <QStandardPaths>
#include <QSysInfo>
#include <QtWidgets/QWidget>
#include <QStandardPaths>

#ifdef _WIN32
void manageShortcut(bool state)
{
    QString shortcutName = "HeadsetControl-Qt.lnk";
    QString applicationPath = QCoreApplication::applicationFilePath();
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QDir::separator() + "Startup";
    QString shortcutPath = startupPath + QDir::separator() + shortcutName;

    if (state) {
        QFile::link(applicationPath, shortcutPath);
    } else {
        QFile::remove(shortcutPath);
    }
}

bool isShortcutPresent()
{
    QString shortcutName = "HeadsetControl-Qt.lnk";
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QDir::separator() + "Startup";
    QString shortcutPath = startupPath + QDir::separator() + shortcutName;

    return QFile::exists(shortcutPath);
}

#endif

#ifdef __linux__
const QString desktopFile = QDir::homePath() + "/.config/autostart/headsetcontrol-qt.desktop";

bool isDesktopfilePresent()
{
    if (QFile::exists(desktopFile)) {
        return true;
    }
    return false;
}

void createDesktopFile()
{

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
}

void removeDesktopFile()
{
    QFile file(desktopFile);
    if (file.exists()) {
        file.remove();
    }
}

void manageDesktopFile(bool state)
{
    if (state) {
        createDesktopFile();
    } else {
        removeDesktopFile();
    }
}
#endif
