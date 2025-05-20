#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QJsonObject>

class Worker : public QObject
{
    Q_OBJECT

public:
    explicit Worker(QObject *parent = nullptr);
    void requestWork();
    void abort();

signals:
    void workRequested();
    void workFinished();
    void sendHeadsetInfo(const QJsonObject &headsetInfo);

public slots:
    void doWork();

private:
    static const QString headsetcontrolExecutable;
    bool _abort;
    bool _working;
};

#endif // WORKER_H
