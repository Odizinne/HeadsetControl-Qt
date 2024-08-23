#include "worker.h"
#include <QProcess>
#include <QJsonDocument>

#ifdef _WIN32
const QString Worker::headsetcontrolExecutable = "dependencies/headsetcontrol.exe";
#elif __linux__
const QString Worker::headsetcontrolExecutable = "headsetcontrol";
#endif

Worker::Worker(QObject *parent) : QObject(parent), _abort(false), _working(false)
{
}

void Worker::requestWork()
{
    if (_working) return;
    _working = true;
    emit workRequested();
}

void Worker::abort()
{
    if (!_working) return;
    _abort = true;
}

void Worker::doWork()
{
    _abort = false;

    QProcess process;
    process.start(headsetcontrolExecutable, QStringList() << "-o" << "json");

    if (!process.waitForStarted() || _abort) {
        qDebug() << "Process did not start or was aborted.";
        _working = false;
        emit workFinished();
        return;
    }

    if (!process.waitForFinished() || _abort) {
        qDebug() << "Process did not finish or was aborted.";
        _working = false;
        emit workFinished();
        return;
    }

    QByteArray output = process.readAllStandardOutput();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(output);

    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        emit sendHeadsetInfo(jsonDoc.object());
    }

    _working = false;
    emit workFinished();
}

