#ifndef HEADSETCONTROLCHECKER_H
#define HEADSETCONTROLCHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

class HeadsetControlChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isInstalled READ isInstalled NOTIFY isInstalledChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    explicit HeadsetControlChecker(QObject *parent = nullptr);

    bool isInstalled() const;
    double downloadProgress() const;

    Q_INVOKABLE bool checkHeadsetControlInstallation();
    Q_INVOKABLE void downloadHeadsetControl();

    static QString getDependenciesPath();

signals:
    void isInstalledChanged();
    void downloadProgressChanged();
    void downloadCompleted();
    void downloadFailed(const QString &error);
    void extractionStarted();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    bool extractZip(const QString &zipFile);

    bool m_isInstalled;
    double m_downloadProgress;
    QString m_downloadUrl;
    QString m_zipFilePath;
    QNetworkAccessManager m_networkManager;
    QNetworkReply *m_currentReply;
    QFile m_downloadFile;
};

#endif // HEADSETCONTROLCHECKER_H
