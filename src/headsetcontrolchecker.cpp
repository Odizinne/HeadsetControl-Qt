#include "headsetcontrolchecker.h"
#include <QFileInfo>
#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QThread>
#include <QProcess>

HeadsetControlChecker::HeadsetControlChecker(QObject *parent)
    : QObject(parent)
    , m_isInstalled(false)
    , m_downloadProgress(0.0)
    , m_downloadUrl("https://github.com/Sapd/HeadsetControl/releases/download/3.0.0/headsetcontrol-windows.zip")
    , m_zipFilePath(getDependenciesPath() + "/headsetcontrol-windows.zip")
    , m_currentReply(nullptr)
{
    checkHeadsetControlInstallation();
}

QString HeadsetControlChecker::getDependenciesPath()
{
    return QCoreApplication::applicationDirPath() + "/dependencies";
}

bool HeadsetControlChecker::isInstalled() const
{
    return m_isInstalled;
}

double HeadsetControlChecker::downloadProgress() const
{
    return m_downloadProgress;
}

bool HeadsetControlChecker::checkHeadsetControlInstallation()
{
    QString execPath = getDependenciesPath() + "/headsetcontrol.exe";
    QFileInfo fileInfo(execPath);

    m_isInstalled = fileInfo.exists() && fileInfo.isFile();
    emit isInstalledChanged();
    return m_isInstalled;
}

void HeadsetControlChecker::downloadHeadsetControl()
{
    QDir dir;
    if (!dir.exists(getDependenciesPath())) {
        dir.mkpath(getDependenciesPath());
    }

    m_downloadFile.setFileName(m_zipFilePath);
    if (!m_downloadFile.open(QIODevice::WriteOnly)) {
        emit downloadFailed("Could not open file for writing");
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(m_downloadUrl));
    m_currentReply = m_networkManager.get(request);

    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &HeadsetControlChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_currentReply && m_downloadFile.isOpen()) {
            m_downloadFile.write(m_currentReply->readAll());
        }
    });
    connect(m_currentReply, &QNetworkReply::finished,
            this, &HeadsetControlChecker::onDownloadFinished);
}

void HeadsetControlChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        m_downloadProgress = static_cast<double>(bytesReceived) / bytesTotal;
        emit downloadProgressChanged();
    }
}

void HeadsetControlChecker::onDownloadFinished()
{
    if (m_currentReply->error() != QNetworkReply::NoError) {
        m_downloadFile.close();
        m_downloadFile.remove();
        emit downloadFailed(m_currentReply->errorString());
    } else {
        m_downloadFile.write(m_currentReply->readAll());
        m_downloadFile.close();

        m_downloadProgress = 1.0;
        emit downloadProgressChanged();

        emit extractionStarted();

        QFuture<bool> future = QtConcurrent::run([this]() {
            return this->extractZip(m_zipFilePath);
        });

        QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>();
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
            bool success = watcher->result();
            watcher->deleteLater();

            if (success) {
                m_isInstalled = true;
                QFile::remove(m_zipFilePath);

                emit isInstalledChanged();
                emit downloadCompleted();
            } else {
                emit downloadFailed("Failed to extract the downloaded file");
            }
        });

        watcher->setFuture(future);
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

bool HeadsetControlChecker::extractZip(const QString &zipFile)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    QString command = QString("powershell.exe -Command \"& {Expand-Archive -Path '%1' -DestinationPath '%2' -Force}\"")
                          .arg(QDir::toNativeSeparators(zipFile),
                               QDir::toNativeSeparators(getDependenciesPath()));

    process.start(command);

    process.waitForFinished(60000);
    QThread::sleep(2);

    QFileInfo fileInfo(getDependenciesPath() + "/headsetcontrol.exe");
    bool exists = fileInfo.exists() && fileInfo.isFile();

    return exists;
}
