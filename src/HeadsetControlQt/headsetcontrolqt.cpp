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
    , firstRun(false)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/icon.png"));
    loadSettings();
    initUI();
    createTrayIcon();
    updateHeadsetInfo();
    timer->start(5000);
    connect(timer, &QTimer::timeout, this, &HeadsetControlQt::updateHeadsetInfo);
    if (firstRun) {
        this->show();
    }
}

HeadsetControlQt::~HeadsetControlQt()
{
    delete ui;
}

void HeadsetControlQt::initUI()
{
    populateComboBoxes();
    setFont();
    checkStartupCheckbox();
    setupUIConnections();
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
}

void HeadsetControlQt::populateComboBoxes()
{
    ui->themeComboBox->addItem(tr("System"));
    ui->themeComboBox->addItem(tr("Dark"));
    ui->themeComboBox->addItem(tr("Light"));
}

void HeadsetControlQt::setFont()
{
    QList<QGroupBox*> groupBoxes = {
        ui->deviceGroupBox,
        ui->generalGroupBox
    };

    for (QGroupBox* groupBox : groupBoxes) {
        groupBox->setStyleSheet("font-weight: bold;");

        const QList<QWidget*> children = groupBox->findChildren<QWidget*>();
        for (QWidget* child : children) {
            child->setStyleSheet("font-weight: normal;");
        }
    }
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
    QMenu *trayMenu = new QMenu(this);
    QAction *showAction = new QAction("Show", this);
    connect(showAction, &QAction::triggered, this, &HeadsetControlQt::toggleWindow);
    trayMenu->addAction(showAction);
    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
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
    ui->notificationBatteryCheckBox->setChecked(settings.value("notification_low_battery").toInt());
    ui->sidetoneSlider->setValue(settings.value("sidetone").toInt());
    ui->themeComboBox->setCurrentIndex(settings.value("theme").toInt());
    ui->lowBatteryThresholdSpinBox->setValue(settings.value("low_battery_threshold").toInt());
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
    QString deviceName = headsetInfo["device"].toString();
    QStringList capabilities = headsetInfo["capabilities_str"].toVariant().toStringList();
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();

    ui->deviceGroupBox->setTitle(deviceName);

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
            } else {
                trayIcon->setIcon(QIcon(iconPath));
            }
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
            } else {
                trayIcon->setIcon(QIcon(iconPath));
            }
        }
#endif
    } else {
        ui->batteryBar->setValue(0);
        ui->batteryBar->setFormat("Off");
        trayIcon->setToolTip("No headset connected");

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
            } else {
                trayIcon->setIcon(QIcon(iconPath));
            }
        }
#endif
    }

    ui->ledBox->setEnabled(capabilities.contains("lights"));
    ui->ledLabel->setEnabled(capabilities.contains("lights"));
    ui->ledBatteryCheckBox->setEnabled(capabilities.contains("lights"));

    ui->sidetoneSlider->setEnabled(capabilities.contains("sidetone"));
    ui->sidetoneLabel->setEnabled(capabilities.contains("sidetone"));

    toggleUIElements(true);
}

void HeadsetControlQt::noDeviceFound()
{
    toggleUIElements(false);
    trayIcon->setToolTip("No Device Found");
}

void HeadsetControlQt::toggleUIElements(bool show)
{
    ui->deviceGroupBox->setVisible(show);
    ui->generalGroupBox->setVisible(show);
    ui->notFoundLabel->setVisible(!show);
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
    updateHeadsetInfo();
    saveSettings();
}

void HeadsetControlQt::setSidetone()
{
    QProcess process;
    process.start(headsetcontrolExecutable, QStringList() << "-s" << QString::number(ui->sidetoneSlider->value()));
    process.waitForFinished();
}

void HeadsetControlQt::toggleWindow()
{
    if (this->isVisible()) {
        this->close();
        trayIcon->contextMenu()->actions().first()->setText("Show");
    } else {
        this->show();
        trayIcon->contextMenu()->actions().first()->setText("Hide");
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
    trayIcon->contextMenu()->actions().first()->setText("Show");
}
