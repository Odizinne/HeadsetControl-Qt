#include "headsetcontrolqt.h"
#include "ui_headsetcontrolqt.h"
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

#ifdef _WIN32
const QString HeadsetControlQt::headsetcontrolExecutable = "dependencies/headsetcontrol.exe";
const QString HeadsetControlQt::settingsFile = QStandardPaths::writableLocation(
                                                   QStandardPaths::AppDataLocation)
                                               + "/HeadsetControl-Qt/settings.json";
#elif __linux__
const QString HeadsetControlQt::headsetcontrolExecutable = "headsetcontrol";
const QString HeadsetControlQt::settingsFile = QDir::homePath() + "/.config/HeadsetControl-Qt/settings.json";
const QString HeadsetControlQt::desktopFile = QDir::homePath() + "/.config/autostart/headsetcontrol-qt.desktop";

#endif

HeadsetControlQt::HeadsetControlQt(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HeadsetControlQt)
    , trayIcon(new QSystemTrayIcon(this))
    , timer(new QTimer(this))
    , ledDisabled(false)
    , notificationSent(false)
    , soundNotificationSent(false)
    , firstRun(false)
    , closing(false)
    , worker(new Worker())
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/icon.png"));
    initUI();
    createTrayIcon();
    loadSettings();
    setupUIConnections();
    updateHeadsetInfo();

    connect(worker, &Worker::workRequested, worker, &Worker::doWork);
    connect(worker, &Worker::sendHeadsetInfo, this, &::HeadsetControlQt::handleHeadsetInfo);
    connect(timer, &QTimer::timeout, worker, &Worker::requestWork);

    worker->moveToThread(&workerThread);
    workerThread.start();
    timer->start(5000);
    if (firstRun) {
        this->show();
    }
}

HeadsetControlQt::~HeadsetControlQt()
{
    worker->abort();
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void HeadsetControlQt::handleHeadsetInfo(const QJsonObject &headsetInfo)
{
    if (headsetInfo.contains("devices")) {
        QJsonArray devicesArray = headsetInfo["devices"].toArray();
        if (devicesArray.size() > 0) {
            QJsonObject headsetInfo = devicesArray.first().toObject();
            updateUIWithHeadsetInfo(headsetInfo);
            if (ui->ledBatteryCheckBox->isChecked()) {
                manageLEDBasedOnBattery(headsetInfo);
            }
            if (ui->notificationBatteryCheckBox->isChecked()) {
                sendNotificationBasedOnBattery(headsetInfo);
            }
            if (ui->soundBatteryCheckBox->isChecked()) {
                sendSoundNotificationBasedOnBattery(headsetInfo);
            }
        } else {
            noDeviceFound();
        }
    } else {
        noDeviceFound();
    }
}

void HeadsetControlQt::initUI()
{
    QString currentStyle = QApplication::style()->objectName();
    if (currentStyle == QStyleFactory::create("Fusion")->objectName()) {
        setFrameColorBasedOnWindow(this, ui->frame);
        setFrameColorBasedOnWindow(this, ui->frame_2);
    }
    populateComboBoxes();
    checkStartupCheckbox();
}

void HeadsetControlQt::setupUIConnections()
{
    connect(ui->ledBox, &QCheckBox::stateChanged, this, &HeadsetControlQt::onLedBoxStateChanged);
    connect(ui->ledBatteryCheckBox, &QCheckBox::stateChanged, this, &HeadsetControlQt::saveSettings);
    connect(ui->notificationBatteryCheckBox, &QCheckBox::stateChanged, this, &HeadsetControlQt::saveSettings);
    connect(ui->startupCheckbox, &QCheckBox::stateChanged, this, &HeadsetControlQt::onStartupCheckBoxStateChanged);
    connect(ui->sidetoneSlider, &QSlider::sliderReleased, this, &HeadsetControlQt::onSidetoneSliderSliderReleased);
    connect(ui->themeComboBox, &QComboBox::currentIndexChanged, this, &HeadsetControlQt::onThemeComboBoxCurrentIndexChanged);
    connect(ui->lowBatteryThresholdSpinBox, &QSpinBox::valueChanged, this, &HeadsetControlQt::saveSettings);
    connect(ui->soundBatteryCheckBox, &QCheckBox::stateChanged, this, &HeadsetControlQt::saveSettings);
    connect(ui->languageComboBox, &QComboBox::currentIndexChanged, this, &HeadsetControlQt::changeApplicationLanguage);
}

void HeadsetControlQt::populateComboBoxes()
{
    ui->themeComboBox->addItem(tr("System"));
    ui->themeComboBox->addItem(tr("Dark"));
    ui->themeComboBox->addItem(tr("Light"));
    ui->languageComboBox->addItem("System");
    ui->languageComboBox->addItem("english");
    ui->languageComboBox->addItem("français");
    ui->languageComboBox->addItem("deutsch");
    ui->languageComboBox->addItem("español");
    ui->languageComboBox->addItem("italiano");
    ui->languageComboBox->addItem("magyar");
}

void HeadsetControlQt::checkStartupCheckbox()
{
#ifdef _WIN32
    ui->startupCheckbox->setChecked(isShortcutPresent());
#elif __linux__
    ui->startupCheckbox->setChecked(isDesktopfilePresent());
#endif
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

void HeadsetControlQt::createDefaultSettings()
{
    ui->ledBox->setChecked(true);
    ui->sidetoneSlider->setValue(0);
    ui->themeComboBox->setCurrentIndex(0);
    saveSettings();

    firstRun = true;
}

void HeadsetControlQt::loadSettings()
{
    QDir settingsDir(QFileInfo(settingsFile).absolutePath());
    if (!settingsDir.exists()) {
        settingsDir.mkpath(settingsDir.absolutePath());
    }

    QFile file(settingsFile);
    if (!file.exists()) {
        createDefaultSettings();

    } else {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                settings = doc.object();
            }
            file.close();
        }
    }
    applySettings();
}

void HeadsetControlQt::applySettings()
{
    ui->ledBox->setChecked(settings.value("led_state").toBool());
    ui->ledBatteryCheckBox->setChecked(settings.value("led_low_battery").toBool());
    ui->notificationBatteryCheckBox->setChecked(settings.value("notification_low_battery").toBool());
    ui->sidetoneSlider->setValue(settings.value("sidetone").toInt());
    ui->themeComboBox->setCurrentIndex(settings.value("theme").toInt());
    ui->lowBatteryThresholdSpinBox->setValue(settings.value("low_battery_threshold").toInt());
    ui->soundBatteryCheckBox->setChecked(settings.value("sound_low_battery").toBool());
    ui->languageComboBox->setCurrentIndex(settings.value("language").toInt());
    changeApplicationLanguage();
    setSidetone();
    toggleLED(ui->ledBox->isChecked());
}

void HeadsetControlQt::saveSettings()
{
    settings["led_state"] = ui->ledBox->isChecked();
    settings["led_low_battery"] = ui->ledBatteryCheckBox->isChecked();
    settings["notification_low_battery"] = ui->notificationBatteryCheckBox->isChecked();
    settings["sidetone"] = ui->sidetoneSlider->value();
    settings["theme"] = ui->themeComboBox->currentIndex();
    settings["low_battery_threshold"] = ui->lowBatteryThresholdSpinBox->value();
    settings["sound_low_battery"] = ui->soundBatteryCheckBox->isChecked();
    settings["language"] = ui->languageComboBox->currentIndex();

    QFile file(settingsFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(settings);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
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
            if (ui->ledBatteryCheckBox->isChecked()) {
                manageLEDBasedOnBattery(headsetInfo);
            }
            if (ui->notificationBatteryCheckBox->isChecked()) {
                sendNotificationBasedOnBattery(headsetInfo);
            }
            if (ui->soundBatteryCheckBox->isChecked()) {
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

    if (batteryLevel < ui->lowBatteryThresholdSpinBox->value() && !ledDisabled && available) {
        ui->ledBox->setChecked(false);
        ledDisabled = true;
    } else if (batteryLevel >= ui->lowBatteryThresholdSpinBox->value() + 5 && ledDisabled && available) {
        ui->ledBox->setChecked(true);
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

    if (batteryLevel < ui->lowBatteryThresholdSpinBox->value() && !notificationSent && available) {
        sendNotification(tr("Low battery"), QString(tr("%1 has %2% battery left.")).arg(headsetName).arg(batteryLevel), QIcon(":/icons/icon.png"), 5000);
        notificationSent = true;
    } else if (batteryLevel >= ui->lowBatteryThresholdSpinBox->value() + 5 && notificationSent && available) {
        notificationSent = false;
    }
}

void HeadsetControlQt::sendSoundNotificationBasedOnBattery(const QJsonObject &headsetInfo)
{
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();
    int batteryLevel = batteryInfo["level"].toInt();
    QString batteryStatus = batteryInfo["status"].toString();
    bool available = (batteryStatus == "BATTERY_AVAILABLE");

    if (batteryLevel < ui->lowBatteryThresholdSpinBox->value() && !soundNotificationSent && available) {
        sendSoundNotification();
        soundNotificationSent = true;
    } else if (batteryLevel >= ui->lowBatteryThresholdSpinBox->value() + 5 && soundNotificationSent && available) {
        soundNotificationSent = false;
    }
}

void HeadsetControlQt::sendNotification(const QString &title, const QString &message, const QIcon &icon, int duration)
{
    trayIcon->showMessage(title, message, icon, duration);
}

void HeadsetControlQt::toggleLED(bool state)
{
    QProcess process;
    process.start(headsetcontrolExecutable, QStringList() << "-l" << (state ? "1" : "0"));
    process.waitForFinished();
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

    ui->deviceLabel->setText(deviceName);

    QString batteryStatus = batteryInfo["status"].toString();
    int batteryLevel = batteryInfo["level"].toInt();

    if (batteryStatus == "BATTERY_AVAILABLE") {
        ui->batteryBar->setValue(batteryLevel);
        ui->batteryBar->setFormat(QString::number(batteryLevel) + "%");
        trayIcon->setToolTip(QString("%1: %2%").arg(deviceName).arg(batteryLevel));
        ui->themeComboBox->currentIndex();
        QString iconPath = getBatteryIcon(batteryLevel, false, false, ui->themeComboBox->currentIndex());
#ifdef _WIN32
        trayIcon->setIcon(QIcon(iconPath));
#elif __linux__
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString desktop = env.value("XDG_CURRENT_DESKTOP");
        if (desktop.contains("KDE", Qt::CaseInsensitive)) {
            QString kdeVersion = getKDEPlasmaVersion();
            if (kdeVersion.startsWith("5")) {
                trayIcon->setIcon(QIcon(iconPath));
            } else if (kdeVersion.startsWith("6")) {
                trayIcon->setIcon(QIcon::fromTheme(iconPath));
            }
        } else {
            trayIcon->setIcon(QIcon(iconPath));
        }
#endif
    } else if (batteryStatus == "BATTERY_CHARGING") {
        ui->batteryBar->setValue(0);
        ui->batteryBar->setFormat("Charging");
        trayIcon->setToolTip(QString("%1: Charging").arg(deviceName));

        QString iconPath = getBatteryIcon(batteryLevel, true, false, ui->themeComboBox->currentIndex());
#ifdef _WIN32
        trayIcon->setIcon(QIcon(iconPath));
#elif __linux__
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString desktop = env.value("XDG_CURRENT_DESKTOP");
        if (desktop.contains("KDE", Qt::CaseInsensitive)) {
            QString kdeVersion = getKDEPlasmaVersion();
            if (kdeVersion.startsWith("5")) {
                trayIcon->setIcon(QIcon(iconPath));
            } else if (kdeVersion.startsWith("6")) {
                trayIcon->setIcon(QIcon::fromTheme(iconPath));
            }
        } else {
            trayIcon->setIcon(QIcon(iconPath));
        }
#endif
    } else {
        ui->batteryBar->setValue(0);
        ui->batteryBar->setFormat(tr("Off"));
        trayIcon->setToolTip(tr("No headset connected"));

        QString iconPath = getBatteryIcon(batteryLevel, false, true, ui->themeComboBox->currentIndex());
#ifdef _WIN32
        trayIcon->setIcon(QIcon(iconPath));
#elif __linux__
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString desktop = env.value("XDG_CURRENT_DESKTOP");
        if (desktop.contains("KDE", Qt::CaseInsensitive)) {
            QString kdeVersion = getKDEPlasmaVersion();
            if (kdeVersion.startsWith("5")) {
                trayIcon->setIcon(QIcon(iconPath));
            } else if (kdeVersion.startsWith("6")) {
                trayIcon->setIcon(QIcon::fromTheme(iconPath));
            }
        } else {
            trayIcon->setIcon(QIcon(iconPath));
        }
#endif
    }
    ui->ledBox->setEnabled(capabilities.contains("lights"));
    ui->ledLabel->setEnabled(capabilities.contains("lights"));
    ui->ledBatteryCheckBox->setEnabled(capabilities.contains("lights"));
    ui->soundBatteryLabel->setEnabled(capabilities.contains("notification sound"));
    ui->soundBatteryCheckBox->setEnabled(capabilities.contains("notification sound"));
    ui->sidetoneSlider->setEnabled(capabilities.contains("sidetone"));
    ui->sidetoneLabel->setEnabled(capabilities.contains("sidetone"));
    toggleUIElements(true);
}

void HeadsetControlQt::noDeviceFound()
{
    toggleUIElements(false);
    trayIcon->setToolTip(tr("No Device Found"));
    QString iconPath = getBatteryIcon(0, false, true, ui->themeComboBox->currentIndex());
#ifdef _WIN32
    trayIcon->setIcon(QIcon(iconPath));
#elif __linux__
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString desktop = env.value("XDG_CURRENT_DESKTOP");
    if (desktop.contains("KDE", Qt::CaseInsensitive)) {
        QString kdeVersion = getKDEPlasmaVersion();
        if (kdeVersion.startsWith("5")) {
            trayIcon->setIcon(QIcon(iconPath));
        } else if (kdeVersion.startsWith("6")) {
            trayIcon->setIcon(QIcon::fromTheme(iconPath));
        }
    } else {
        trayIcon->setIcon(QIcon(iconPath));
    }
#endif
}

void HeadsetControlQt::toggleUIElements(bool show)
{
    ui->settingsLabel->setVisible(show);
    ui->frame->setVisible(show);
    ui->deviceLabel->setVisible(show);
    ui->frame_2->setVisible(show);
    ui->notFoundLabel->setVisible(!show);
    this->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    this->setMinimumSize(380, 0);
    this->adjustSize();
    this->setFixedSize(this->size());
}

void HeadsetControlQt::onLedBoxStateChanged()
{
    toggleLED(ui->ledBox->isChecked());
    saveSettings();
}

void HeadsetControlQt::onStartupCheckBoxStateChanged()
{
    if (ui->startupCheckbox->isChecked()) {
#ifdef _WIN32
        manageShortcut(true);
#elif __linux__
        manageDesktopFile(true);
#endif
    } else {
#ifdef _WIN32
        manageShortcut(false);
#elif __linux__
        manageDesktopFile(false);
#endif
    }
}

void HeadsetControlQt::onSidetoneSliderSliderReleased()
{
    setSidetone();
    saveSettings();
}

void HeadsetControlQt::onThemeComboBoxCurrentIndexChanged()
{
    //updateHeadsetInfo();
    saveSettings();
}

void HeadsetControlQt::setSidetone()
{
    QProcess process;
    process.start(headsetcontrolExecutable, QStringList() << "-s" << QString::number(ui->sidetoneSlider->value()));
    process.waitForFinished();
}

void HeadsetControlQt::sendSoundNotification()
{
    QProcess process;
    process.start(headsetcontrolExecutable, QStringList() << "-n" << "0");
    process.waitForFinished();
}

void HeadsetControlQt::toggleWindow()
{
    if (this->isVisible()) {
        this->close();
        trayIcon->contextMenu()->actions().first()->setText(tr("Show"));
    } else {
        this->show();
        trayIcon->contextMenu()->actions().first()->setText(tr("Hide"));
    }
}

void HeadsetControlQt::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        toggleWindow();
    }
}

void HeadsetControlQt::closeEvent(QCloseEvent *event)
{
    closing = true;
    event->accept();
    trayIcon->contextMenu()->actions().first()->setText(tr("Show"));
    sendFirstMinimizeNotification();
}

void HeadsetControlQt::sendFirstMinimizeNotification()
{
    if (closing) {
        return;
    }

    if (firstRun) {
        sendNotification(tr("HeadsetControl-Qt"), QString(tr("The application is still running in the background.")), QIcon(":/icons/icon.png"), 5000);
        firstRun = false;
    }
}

void HeadsetControlQt::changeApplicationLanguage()
{
    QString language = ui->languageComboBox->currentText().toLower();
    QTranslator translator;

    if (qApp->removeTranslator(&translator)) {
    }

    QString languageCode;
    if (language == "system") {
        QLocale systemLocale;
        languageCode = systemLocale.name().left(2);
    } else {
        QMap<QString, QString> languageCodes;
        languageCodes["english"] = "en";
        languageCodes["français"] = "fr";
        languageCodes["deutsch"] = "de";
        languageCodes["español"] = "es";
        languageCodes["italiano"] = "it";
        languageCodes["magyar"] = "hu";

        languageCode = languageCodes.value(language, "en");
    }

    QString translationFile = QString(":/translations/tr/HeadsetControl-Qt_%1.qm").arg(languageCode);
    if (translator.load(translationFile)) {
        qApp->installTranslator(&translator);
    } else {
        qWarning() << "Failed to load translation file:" << translationFile;
    }

    ui->retranslateUi(this);
    updateTrayMenu();
    saveSettings();
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
