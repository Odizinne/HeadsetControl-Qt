#include "headsetcontrolqt.h"
#include "utils.h"
#include "shortcutmanager.h"
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
    , fetchTimer(new QTimer(this))
    , chargingAnimationTimer(new QTimer(this))
    , ledDisabled(false)
    , notificationSent(false)
    , soundNotificationSent(false)
    , firstRun(false)
    , closing(false)
    , translator(new QTranslator(this))
    , worker(new Worker())
    , qmlWindow(nullptr)
    , usbMonitor(new HIDEventMonitor(this))
    , chatMixSetupRan(false)
    , lastMixLevel(-1)
    , initialFetchDone(false)
{
    createTrayIcon();
    toggleLED(settings.value("led_state", true).toBool());
    setSidetone(settings.value("sidetone", 0).toInt());
    updateHeadsetInfo();
    changeApplicationLanguage(settings.value("language").toInt());

    worker->moveToThread(&workerThread);
    workerThread.start();

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    engine->setInitialProperties({{"mainWindow", QVariant::fromValue(this)}});
    engine->loadFromModule("Odizinne.HeadsetControlQt", "Main");
    const QList<QObject*>& objects = engine->rootObjects();
    qmlWindow = qobject_cast<QWindow*>(objects.isEmpty() ? nullptr : objects.first());

    if (settings.value("firstRun", true).toBool()) {
        qmlWindow->show();
    }

    connect(worker, &Worker::workRequested, worker, &Worker::doWork);
    connect(worker, &Worker::sendHeadsetInfo, this, &::HeadsetControlQt::handleHeadsetInfo);
    connect(fetchTimer, &QTimer::timeout, worker, &Worker::requestWork);
    connect(qmlWindow, &QWindow::visibilityChanged, this, &HeadsetControlQt::reflectWindowState);
    connect(chargingAnimationTimer, &QTimer::timeout, this, &HeadsetControlQt::updateTrayChargingAnimation);

    /*===========================================================
    | Rescan 5s after first scan on plug                        |
    | Void Elite do need it as it take long to init,            |
    | and headset will be reported off for the first seconds    |
    | Maybe i could just scan once 5s after plugged             |
    | but idk how other devices are acting                      |
    ===========================================================*/
    connect(usbMonitor, &HIDEventMonitor::deviceRemoved, worker, &Worker::requestWork);
    connect(usbMonitor, &HIDEventMonitor::deviceAdded, worker, &Worker::requestWork);
    connect(usbMonitor, &HIDEventMonitor::deviceAdded, this, [this]() {
        QTimer::singleShot(5000, this->worker, &Worker::requestWork);
    });

    usbMonitor->startMonitoring();

    fetchTimer->setInterval(60000);
    chargingAnimationTimer->setInterval(2000);

    fetchTimer->start();
}

HeadsetControlQt::~HeadsetControlQt()
{
    chargingAnimationTimer->stop();
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

            // Auto-setup ChatMix after initial fetch if enabled
            if (!initialFetchDone) {
                initialFetchDone = true;
                checkAndSetupChatMixOnStartup();
            }
        } else {
            noDeviceFound();
            if (!initialFetchDone) {
                initialFetchDone = true;
            }
        }
    } else {
        noDeviceFound();
        if (!initialFetchDone) {
            initialFetchDone = true;
        }
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

void HeadsetControlQt::updateTrayChargingAnimation()
{
    currentChargingFrame += 20;
    if (currentChargingFrame > 100) {
        currentChargingFrame = 20;
    }

    bool isDarkTheme = settings.value("theme", 0).toInt() == 1;
    QString iconPath = QString(":/icons/battery-%1-charging-%2.png")
                           .arg(currentChargingFrame)
                           .arg(isDarkTheme ? "dark" : "light");

    trayIcon->setIcon(QIcon(iconPath));
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
#else
    deviceName = headsetInfo["device"].toString();
#endif

    setDeviceName(deviceName);

    QStringList capabilities = headsetInfo["capabilities_str"].toVariant().toStringList();
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();

    QString batteryStatus = batteryInfo["status"].toString();
    int batteryLevel = batteryInfo["level"].toInt();

    if (batteryStatus == "BATTERY_AVAILABLE") {
        chargingAnimationTimer->stop();
        setBatteryLevel(batteryLevel);
        setStatus(QString::number(batteryLevel) + "%");
        trayIcon->setToolTip(QString("%1: %2%").arg(deviceName).arg(batteryLevel));
        QString iconPath = Utils::getBatteryIconPath(batteryLevel, false, false, settings.value("theme", 0).toInt());
        trayIcon->setIcon(QIcon(iconPath));
    }
    else if (batteryStatus == "BATTERY_CHARGING") {
        setBatteryLevel(0);
        setStatus("Charging");
        trayIcon->setToolTip(QString(tr("%1: Charging")).arg(deviceName));
        if (!chargingAnimationTimer->isActive()) {
            currentChargingFrame = 20;
            chargingAnimationTimer->start();
        }
    }
    else {
        chargingAnimationTimer->stop();
        setBatteryLevel(0);
        setStatus("Off");
        trayIcon->setToolTip(tr("No headset connected"));
        QString iconPath = Utils::getBatteryIconPath(batteryLevel, false, true, settings.value("theme", 0).toInt());
        trayIcon->setIcon(QIcon(iconPath));
    }

    setLightsCapable(capabilities.contains("lights"));
    setSidetoneCapable(capabilities.contains("sidetone"));
    setSoundNotifCapable(capabilities.contains("notification sound"));
    setChatmixCapable(capabilities.contains("chatmix"));

    if (headsetInfo.contains("chatmix")) {
        int newChatmix = headsetInfo["chatmix"].toInt();
        if (newChatmix >= 0 && newChatmix <= 128 && m_chatmix != newChatmix) {
            m_chatmix = newChatmix;
            emit chatmixChanged();

#ifdef __linux__
            if (deviceName == "Arctis Nova 7" &&
                settings.value("enableChatmix", true).toBool() &&
                chatMixSetupRan) {
                updateChatMixVolumes(newChatmix);
            }
#endif
        }
    }

    setNoDevice(false);
}

void HeadsetControlQt::noDeviceFound()
{
    trayIcon->setToolTip(tr("No Device Found"));
    QString iconPath = Utils::getBatteryIconPath(0, false, true, settings.value("theme", 0).toInt());

    trayIcon->setIcon(QIcon(iconPath));
    setNoDevice(true);

    // Reset ChatMix if no device found
#ifdef __linux__
    if (chatMixSetupRan) {
        fetchTimer->setInterval(60000);
        removeChatMixSinks();
        chatMixSetupRan = false;
    }
#endif
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

    QString translationFile = QString(":/i18n/HeadsetControl-Qt_%1.qm").arg(languageCode);
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

void HeadsetControlQt::toggleChatMixSetup(bool enable)
{
#ifdef __linux__
    // Only proceed if we have a Nova 7 connected
    if (m_deviceName != "Arctis Nova 7") {
        return;
    }

    if (enable) {
        // Enable ChatMix: create sinks and change timer
        fetchTimer->setInterval(1000);
        createChatMixScripts();
        runChatMixSetup();
    } else {
        // Disable ChatMix: remove sinks and reset timer
        fetchTimer->setInterval(60000);
        removeChatMixSinks();
        chatMixSetupRan = false;
    }
#endif
}

void HeadsetControlQt::checkAndSetupChatMixOnStartup()
{
#ifdef __linux__
    if (m_deviceName == "Arctis Nova 7" && settings.value("enableChatmix", true).toBool()) {
        fetchTimer->setInterval(1000);
        createChatMixScripts();
        runChatMixSetup();
    }
#endif
}

void HeadsetControlQt::removeChatMixSinks()
{
#ifdef __linux__
    QString audioSystem = getActiveAudioSystem();
    if (audioSystem.isEmpty()) {
        return;
    }

    // Remove GameMix sink
    QProcess *gameProcess = new QProcess(this);
    connect(gameProcess, &QProcess::finished, gameProcess, &QProcess::deleteLater);

    // Remove ChatMix sink
    QProcess *chatProcess = new QProcess(this);
    connect(chatProcess, &QProcess::finished, chatProcess, &QProcess::deleteLater);

    if (audioSystem == "pipewire") {
        // For PipeWire, we need to find and unload the specific modules
        gameProcess->start("pactl", QStringList() << "unload-module" << "module-null-sink");
        chatProcess->start("pactl", QStringList() << "unload-module" << "module-loopback");
    } else { // pulseaudio
        // For PulseAudio, similar approach
        gameProcess->start("pacmd", QStringList() << "unload-module" << "module-null-sink");
        chatProcess->start("pacmd", QStringList() << "unload-module" << "module-loopback");
    }

    qDebug() << "Removed ChatMix sinks";
#endif
}

bool HeadsetControlQt::isAudioSystemAvailable(const QString &system)
{
    QProcess process;
    if (system == "pipewire") {
        process.start("systemctl", QStringList() << "--user" << "is-active" << "pipewire.service");
    } else if (system == "pulseaudio") {
        process.start("pulseaudio", QStringList() << "--check");
    }

    process.waitForFinished(3000);
    return process.exitCode() == 0;
}

QString HeadsetControlQt::getActiveAudioSystem()
{
    if (isAudioSystemAvailable("pipewire")) {
        return "pipewire";
    } else if (isAudioSystemAvailable("pulseaudio")) {
        return "pulseaudio";
    }
    return QString();
}

void HeadsetControlQt::createChatMixScripts()
{
    QString binPath = QDir::homePath() + "/.local/bin/headsetcontrol-qt";
    QDir().mkpath(binPath);

    QString audioSystem = getActiveAudioSystem();
    if (audioSystem.isEmpty()) {
        qDebug() << "No supported audio system found";
        return;
    }

    QString scriptPath = binPath + "/setup-chatmix.sh";
    QFile scriptFile(scriptPath);

    if (scriptFile.exists()) {
        return; // Script already exists
    }

    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "#!/bin/bash\n\n";

        if (audioSystem == "pipewire") {
            out << "#Reset Pipewire\n";
            out << "systemctl --user restart pipewire.service\n\n";
            out << "#Create Game Mix\n";
            out << "pactl load-module module-null-sink sink_name=GameMix\n";
            out << "pactl load-module module-loopback source=GameMix.monitor sink=alsa_output.usb-SteelSeries_Arctis_Nova_7-00.analog-stereo\n\n";
            out << "#Create Chat Mix\n";
            out << "pactl load-module module-null-sink sink_name=ChatMix\n";
            out << "pactl load-module module-loopback source=ChatMix.monitor sink=alsa_output.usb-SteelSeries_Arctis_Nova_7-00.analog-stereo\n";
        } else { // pulseaudio
            out << "#Reset PulseAudio\n";
            out << "pulseaudio -k\n\n";
            out << "#Create GameMix\n";
            out << "pacmd load-module module-null-sink sink_name=GameMix\n";
            out << "pacmd update-sink-proplist GameMix device.description=GameMix\n";
            out << "pacmd load-module module-loopback source=GameMix.monitor sink=alsa_output.usb-SteelSeries_Arctis_Nova_7-00.iec958-stereo\n\n";
            out << "#Create ChatMix\n";
            out << "pacmd load-module module-null-sink sink_name=ChatMix\n";
            out << "pacmd update-sink-proplist ChatMix device.description=ChatMix\n";
            out << "pacmd load-module module-loopback source=ChatMix.monitor sink=alsa_output.usb-SteelSeries_Arctis_Nova_7-00.iec958-stereo\n";
        }

        scriptFile.close();

        // Make script executable
        QFile::setPermissions(scriptPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }
}

void HeadsetControlQt::runChatMixSetup()
{
    if (chatMixSetupRan) {
        return;
    }

    QString scriptPath = QDir::homePath() + "/.local/bin/headsetcontrol-qt/setup-chatmix.sh";
    QProcess *process = new QProcess(this);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0) {
                    chatMixSetupRan = true;
                    qDebug() << "ChatMix setup completed successfully";
                } else {
                    qDebug() << "ChatMix setup failed with exit code:" << exitCode;
                }
                process->deleteLater();
            });

    process->start("/bin/bash", QStringList() << scriptPath);
}

void HeadsetControlQt::updateChatMixVolumes(int mixLevel)
{
    if (lastMixLevel == mixLevel) {
        return;
    }

    int gameLevel, chatLevel;

    if (mixLevel > 64) {
        gameLevel = 200 - (mixLevel * 100 / 64);
        chatLevel = 100;
    } else if (mixLevel < 64) {
        gameLevel = 100;
        chatLevel = (mixLevel * 100 / 64);
    } else {
        gameLevel = 100;
        chatLevel = 100;
    }

    lastMixLevel = mixLevel;

    QProcess *gameProcess = new QProcess(this);
    connect(gameProcess, &QProcess::finished, gameProcess, &QProcess::deleteLater);
    gameProcess->start("pactl", QStringList() << "--" << "set-sink-volume" << "GameMix" << QString("%1%").arg(gameLevel));

    QProcess *chatProcess = new QProcess(this);
    connect(chatProcess, &QProcess::finished, chatProcess, &QProcess::deleteLater);
    chatProcess->start("pactl", QStringList() << "--" << "set-sink-volume" << "ChatMix" << QString("%1%").arg(chatLevel));

    qDebug() << "Updated ChatMix volumes - Game:" << gameLevel << "% Chat:" << chatLevel << "%";
}
