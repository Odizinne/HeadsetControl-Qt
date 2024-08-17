#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QShortcut>
#include <QStandardPaths>
#include <QSysInfo>
#include <QtWidgets/QWidget>

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#include <windows.h>

QString getStartupFolder()
{
    QString path;
    WCHAR szPath[MAX_PATH];
    if (SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, szPath) == S_OK) {
        path = QString::fromWCharArray(szPath);
    }
    return path;
}

void setPaths(QString &targetPath, QString &startupFolder)
{
    TCHAR executablePath[MAX_PATH];
    GetModuleFileName(NULL, executablePath, MAX_PATH);
    targetPath = QString::fromWCharArray(executablePath);

    startupFolder = getStartupFolder();
}

QString getShortcutPath()
{
    static QString shortcutName = "HeadsetControl-Qt.lnk";
    return getStartupFolder() + "\\" + shortcutName;
}

void createShortcut(const QString &targetPath)
{
    QString shortcutPath = getShortcutPath();
    QString workingDirectory = QFileInfo(targetPath).path();

    IShellLink *pShellLink = nullptr;
    IPersistFile *pPersistFile = nullptr;

    if (FAILED(CoInitialize(nullptr))) {
        qDebug() << "Failed to initialize COM library.";
        return;
    }

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink,
                                   nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IShellLink,
                                   (void **) &pShellLink))) {
        pShellLink->SetPath(targetPath.toStdWString().c_str());
        pShellLink->SetWorkingDirectory(workingDirectory.toStdWString().c_str());
        pShellLink->SetDescription(L"Launch HeadsetControl-Qt");

        if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (void **) &pPersistFile))) {
            pPersistFile->Save(shortcutPath.toStdWString().c_str(), TRUE);
            pPersistFile->Release();
        }

        pShellLink->Release();
    } else {
        qDebug() << "Failed to create ShellLink instance.";
    }

    CoUninitialize();
}

bool isShortcutPresent()
{
    QString shortcutPath = getShortcutPath();
    return QFile::exists(shortcutPath);
}

void removeShortcut()
{
    QString shortcutPath = getShortcutPath();
    if (isShortcutPresent()) {
        QFile::remove(shortcutPath);
    }
}

void manageShortcut(bool state)
{
    QString targetPath;
    QString startupFolder;

    setPaths(targetPath, startupFolder);

    if (state) {
        if (!isShortcutPresent()) {
            createShortcut(targetPath);
        }
    } else {
        if (isShortcutPresent()) {
            removeShortcut();
        }
    }
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
