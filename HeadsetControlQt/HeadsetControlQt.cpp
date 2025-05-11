#include "HeadsetControlQt.h"
#include "Utils.h"
#include "ShortcutManager.h"
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QLocale>
#include <QTranslator>
#include <QMenu>
#include <QQmlContext>
#include <QApplication>

#ifdef _WIN32
const QString HeadsetControlQt::headsetcontrolExecutable = "dependencies/headsetcontrol.exe";
#elif __linux__
const QString HeadsetControlQt::headsetcontrolExecutable = "headsetcontrol";
const QString HeadsetControlQt::desktopFile = QDir::homePath() + "/.config/autostart/headsetcontrol-qt.desktop";
#endif

HeadsetControlQt::HeadsetControlQt(QWidget *parent)
    : QWidget(parent)
    , m_isRunAtStartup(ShortcutManager::isShortcutPresent())
    , settings("Odizinne", "HeadsetControlQt")
    , engine(new QQmlApplicationEngine(this))
    , trayIcon(new QSystemTrayIcon(this))
    , timer(new QTimer(this))
    , ledDisabled(false)
    , notificationSent(false)
    , soundNotificationSent(false)
    , firstRun(false)
    , closing(false)
    , translator(new QTranslator(this))
    , worker(new Worker())
    , qmlWindow(nullptr)
    , usbMonitor(new HIDEventMonitor())
{
    createTrayIcon();
    toggleLED(settings.value("led_state", true).toBool());
    setSidetone(settings.value("sidetone", 0).toInt());
    updateHeadsetInfo();
    changeApplicationLanguage(settings.value("language").toInt());

    connect(worker, &Worker::workRequested, worker, &Worker::doWork);
    connect(worker, &Worker::sendHeadsetInfo, this, &::HeadsetControlQt::handleHeadsetInfo);
    connect(timer, &QTimer::timeout, worker, &Worker::requestWork);

    worker->moveToThread(&workerThread);
    workerThread.start();
    timer->start(60000);

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    engine->setInitialProperties({{"mainWindow", QVariant::fromValue(this)}});
    engine->load(QUrl("qrc:/qml/Main.qml"));
    const QList<QObject*>& objects = engine->rootObjects();
    qmlWindow = qobject_cast<QWindow*>(objects.isEmpty() ? nullptr : objects.first());

    if (settings.value("firstRun", true).toBool()) {
        qmlWindow->show();
    }

    connect(qmlWindow, &QWindow::visibilityChanged, this, &HeadsetControlQt::reflectWindowState);

    QObject::connect(usbMonitor, &HIDEventMonitor::deviceAdded, worker, &Worker::requestWork);

    /*===========================================================
    | Rescan 5s after first scan on plug                        |
    | Void Elite do need it as it take long to init,            |
    | and headset will be reported off for the first seconds    |
    | Maybe i could just scan once 5s after plugged             |
    | but idk how other devices are acting                      |
    ===========================================================*/
    QObject::connect(usbMonitor, &HIDEventMonitor::deviceAdded, this, [this]() {
        QTimer::singleShot(5000, this->worker, &Worker::requestWork);
    });
    QObject::connect(usbMonitor, &HIDEventMonitor::deviceRemoved, worker, &Worker::requestWork);

    usbMonitor->startMonitoring();
}

HeadsetControlQt::~HeadsetControlQt()
{
    worker->abort();
    workerThread.quit();
    workerThread.wait();
}

void HeadsetControlQt::handleHeadsetInfo(const QJsonObject &headsetInfo)
{
    if (headsetInfo.contains("devices")) {
        QJsonArray devicesArray = headsetInfo["devices"].toArray();
        if (devicesArray.size() > 0) {
            QJsonObject headsetInfo = devicesArray.first().toObject();
            updateUIWithHeadsetInfo(headsetInfo);
            if (settings.value("led_low_battery", false).toBool()) {
                manageLEDBasedOnBattery(headsetInfo);
            }
            if (settings.value("notification_low_battery", false).toBool()) {
                sendNotificationBasedOnBattery(headsetInfo);
            }
            if (settings.value("sound_low_battery", false).toBool()) {
                sendSoundNotificationBasedOnBattery(headsetInfo);
            }
        } else {
            noDeviceFound();
        }
    } else {
        noDeviceFound();
    }
}

void HeadsetControlQt::setRunAtStartup(bool enable)
{
    ShortcutManager::manageShortcut(enable);

    if (m_isRunAtStartup != enable) {
        m_isRunAtStartup = enable;
        emit isRunAtStartupChanged();
    }
}

void HeadsetControlQt::createTrayIcon()
{
    trayIcon->setIcon(QIcon(":/icons/icon.png"));
    trayMenu = new QMenu(this);
    showAction = new QAction(tr("Show"), this);
    exitAction = new QAction(tr("Exit"), this);

    connect(showAction, &QAction::triggered, this, &HeadsetControlQt::toggleWindow);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(showAction);
    trayMenu->addAction(exitAction);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &HeadsetControlQt::trayIconActivated);
}

void HeadsetControlQt::reflectWindowState(QWindow::Visibility visibility)
{
    if (visibility == QWindow::Hidden) {
        trayIcon->contextMenu()->actions().first()->setText(tr("Show"));
    }
    else {
        worker->requestWork();
        trayIcon->contextMenu()->actions().first()->setText(tr("Hide"));
    }
}

void HeadsetControlQt::updateHeadsetInfo()
{
    QProcess process;
    process.start(headsetcontrolExecutable, QStringList() << "-o" << "json");

    if (!process.waitForStarted()) {
        noDeviceFound();
        return;
    }

    if (!process.waitForFinished()) {
        noDeviceFound();
        return;
    }

    QByteArray output = process.readAllStandardOutput();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(output);
    if (jsonDoc.isNull()) {
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (jsonObj.contains("devices")) {
        QJsonArray devicesArray = jsonObj["devices"].toArray();
        if (devicesArray.size() > 0) {
            QJsonObject headsetInfo = devicesArray.first().toObject();
            updateUIWithHeadsetInfo(headsetInfo);
            if (settings.value("led_low_battery", false).toBool()) {
                manageLEDBasedOnBattery(headsetInfo);
            }
            if (settings.value("notification_low_battery", false).toBool()) {
                sendNotificationBasedOnBattery(headsetInfo);
            }
            if (settings.value("sound_low_battery", false).toBool()) {
                sendSoundNotificationBasedOnBattery(headsetInfo);
            }
        } else {
            noDeviceFound();
        }
    } else {
        noDeviceFound();
    }
}

void HeadsetControlQt::manageLEDBasedOnBattery(const QJsonObject &headsetInfo)
{
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();
    int batteryLevel = batteryInfo["level"].toInt();
    QString batteryStatus = batteryInfo["status"].toString();
    bool available = (batteryStatus == "BATTERY_AVAILABLE");

    if (batteryLevel < settings.value("low_battery_threshold", 20).toInt() && !ledDisabled && available) {
        //ui->ledBox->setChecked(false);
        ledDisabled = true;
    } else if (batteryLevel >= settings.value("low_battery_threshold", 20).toInt() + 5 && ledDisabled && available) {
        //ui->ledBox->setChecked(true);
        ledDisabled = false;
    }
}

void HeadsetControlQt::sendNotificationBasedOnBattery(const QJsonObject &headsetInfo)
{
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();
    QString headsetName = headsetInfo["device"].toString();
    int batteryLevel = batteryInfo["level"].toInt();
    QString batteryStatus = batteryInfo["status"].toString();
    bool available = (batteryStatus == "BATTERY_AVAILABLE");

    if (batteryLevel < settings.value("low_battery_threshold", 20).toInt() && !notificationSent && available) {
        sendNotification(tr("Low battery"), QString(tr("%1 has %2% battery left.")).arg(headsetName).arg(batteryLevel), QIcon(":/icons/icon.png"), 5000);
        notificationSent = true;
    } else if (batteryLevel >= settings.value("low_battery_threshold", 20).toInt() + 5 && notificationSent && available) {
        notificationSent = false;
    }
}

void HeadsetControlQt::sendSoundNotificationBasedOnBattery(const QJsonObject &headsetInfo)
{
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();
    int batteryLevel = batteryInfo["level"].toInt();
    QString batteryStatus = batteryInfo["status"].toString();
    bool available = (batteryStatus == "BATTERY_AVAILABLE");

    if (batteryLevel < settings.value("low_battery_threshold", 20).toInt() && !soundNotificationSent && available) {
        sendSoundNotification();
        soundNotificationSent = true;
    } else if (batteryLevel >= settings.value("low_battery_threshold", 20).toInt() + 5 && soundNotificationSent && available) {
        soundNotificationSent = false;
    }
}

void HeadsetControlQt::sendNotification(const QString &title, const QString &message, const QIcon &icon, int duration)
{
    trayIcon->showMessage(title, message, icon, duration);
}

void HeadsetControlQt::updateUIWithHeadsetInfo(const QJsonObject &headsetInfo)
{
    // "product" returns "HID Device" on windows, hidapi limitation i guess.
    // We can use it on linux to get a more precise device name.
    QString deviceName;
#ifdef __linux__
    deviceName = headsetInfo["product"].toString();
#endif
    deviceName = headsetInfo["device"].toString();
    QStringList capabilities = headsetInfo["capabilities_str"].toVariant().toStringList();
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();

    setDeviceName(deviceName);

    QString batteryStatus = batteryInfo["status"].toString();
    int batteryLevel = batteryInfo["level"].toInt();

    if (batteryStatus == "BATTERY_AVAILABLE") {
        setBatteryLevel(batteryLevel);
        setStatus(QString::number(batteryLevel) + "%");

        trayIcon->setToolTip(QString("%1: %2%").arg(deviceName).arg(batteryLevel));

        QString iconPath = Utils::getBatteryIconPath(batteryLevel, false, false, settings.value("theme", 0).toInt());
        trayIcon->setIcon(QIcon(iconPath));

    } else if (batteryStatus == "BATTERY_CHARGING") {
        setBatteryLevel(0);
        setStatus("Charging");

        trayIcon->setToolTip(QString(tr("%1: Charging")).arg(deviceName));

        QString iconPath = Utils::getBatteryIconPath(batteryLevel, true, false, settings.value("theme", 0).toInt());
        trayIcon->setIcon(QIcon(iconPath));

    } else {
        setBatteryLevel(0);
        setStatus("Off");

        trayIcon->setToolTip(tr("No headset connected"));

        QString iconPath = Utils::getBatteryIconPath(batteryLevel, false, true, settings.value("theme", 0).toInt());
        trayIcon->setIcon(QIcon(iconPath));

    }

    setLightsCapable(capabilities.contains("lights"));
    setSidetoneCapable(capabilities.contains("sidetone"));
    setSoundNotifCapable(capabilities.contains("notification sound"));
    setNoDevice(false);
}

void HeadsetControlQt::noDeviceFound()
{
    trayIcon->setToolTip(tr("No Device Found"));
    QString iconPath = Utils::getBatteryIconPath(0, false, true, settings.value("theme", 0).toInt());

    trayIcon->setIcon(QIcon(iconPath));
    setNoDevice(true);
}

void HeadsetControlQt::toggleLED(bool state)
{
    QProcess* process = new QProcess(this);
    connect(process, &QProcess::finished, process, &QProcess::deleteLater);
    process->start(headsetcontrolExecutable, QStringList() << "-l" << (state ? "1" : "0"));
}

void HeadsetControlQt::setSidetone(int value) {
    QProcess* process = new QProcess(this);
    connect(process, &QProcess::finished, process, &QProcess::deleteLater);
    process->start(headsetcontrolExecutable, QStringList() << "-s" << QString::number(value));
}

void HeadsetControlQt::sendSoundNotification() {
    QProcess* process = new QProcess(this);
    connect(process, &QProcess::finished, process, &QProcess::deleteLater);
    process->start(headsetcontrolExecutable, QStringList() << "-n" << "0");
}

void HeadsetControlQt::toggleWindow()
{
    if (qmlWindow->isVisible()) {
        qmlWindow->close();
    } else {
        qmlWindow->show();
    }
}

void HeadsetControlQt::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        toggleWindow();
    }
}

void HeadsetControlQt::sendFirstMinimizeNotification()
{
    sendNotification(tr("HeadsetControl-Qt"), QString(tr("The application is still running in the background.")), QIcon(":/icons/icon.png"), 5000);
}

void HeadsetControlQt::changeApplicationLanguage(int languageIndex)
{
    qApp->removeTranslator(translator);
    delete translator;
    translator = new QTranslator(this);

    QString languageCode;
    if (languageIndex == 0) {
        QLocale systemLocale;
        languageCode = systemLocale.name().left(2);
    } else {
        switch (languageIndex) {
        case 1:
            languageCode = "en";
            break;
        case 2:
            languageCode = "fr";
            break;
        case 3:
            languageCode = "de";
            break;
        case 4:
            languageCode = "es";
            break;
        case 5:
            languageCode = "it";
            break;
        case 6:
            languageCode = "hu";
            break;
        case 7:
            languageCode = "tr";
            break;
        default:
            languageCode = "en";
            break;
        }
    }

    QString translationFile = QString(":/translations/HeadsetControl-Qt_%1.qm").arg(languageCode);
    if (translator->load(translationFile)) {
        qGuiApp->installTranslator(translator);
    } else {
        qWarning() << "Failed to load translation file:" << translationFile;
    }

    engine->retranslate();
    updateTrayMenu();
}

void HeadsetControlQt::updateTrayMenu()
{
    if (this->isVisible()) {
        showAction->setText(tr("Hide"));
    } else {
        showAction->setText(tr("Show"));
    }
    exitAction->setText(tr("Exit"));
}
