#include "HeadsetControlQt.h"
#include "ui_HeadsetControlQt.h"
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
#include <QDebug>
#include <QStandardPaths>


const QString HeadsetControlQt::settingsFile = QStandardPaths::writableLocation(
                                               QStandardPaths::AppDataLocation)
                                           + "/HeadsetControl-Qt/settings.json";

HeadsetControlQt::HeadsetControlQt(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HeadsetControlQt)
    , trayIcon(new QSystemTrayIcon(this))
    , timer(new QTimer(this))
    , firstRun(false)
{
    ui->setupUi(this);
    setWindowTitle("HeadsetControl-Qt");
    setWindowIcon(QIcon(":/icons/icon.png"));
    setFixedSize(size());
    setFont();
    loadSettings();
    initUI();
    populateComboBoxes();
    createTrayIcon();
    updateHeadsetInfo();
    timer->start(10000);
    checkStartupCheckbox();
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
    connect(ui->ledBox, &QCheckBox::stateChanged, this, &HeadsetControlQt::onLedBoxStateChanged);
    connect(ui->lightBatterySpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &HeadsetControlQt::onLightBatterySpinboxValueChanged);
    connect(ui->notificationBatterySpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &HeadsetControlQt::onNotificationBatterySpinboxValueChanged);
    connect(ui->startupCheckbox, &QCheckBox::stateChanged, this, &HeadsetControlQt::onStartupCheckBoxStateChanged);
    connect(ui->sidetoneSlider, &QSlider::sliderReleased, this, &HeadsetControlQt::onSidetoneSliderSliderReleased);
    connect(ui->themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HeadsetControlQt::onThemeComboBoxCurrentIndexChanged);
}

void HeadsetControlQt::populateComboBoxes()
{
    ui->themeComboBox->addItem(tr("System"));
    ui->themeComboBox->addItem(tr("Dark"));
    ui->themeComboBox->addItem(tr("Light"));
}

void HeadsetControlQt::createTrayIcon()
{
    trayIcon->setIcon(QIcon(":/icons/icon.png"));
    QMenu *trayMenu = new QMenu(this);
    QAction *showAction = new QAction("Show", this);
    connect(showAction, &QAction::triggered, this, &HeadsetControlQt::toggleWindow);
    trayMenu->addAction(showAction);
    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &HeadsetControlQt::exitApp);
    trayMenu->addAction(exitAction);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &HeadsetControlQt::trayIconActivated);
}

void HeadsetControlQt::createDefaultSettings()
{
    ui->ledBox->setChecked(true);
    ui->lightBatterySpinbox->setValue(20);
    ui->notificationBatterySpinbox->setValue(20);
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
    ui->lightBatterySpinbox->setEnabled(settings.value("led_state").toBool());
    ui->lightBatteryLabel->setEnabled(settings.value("led_state").toBool());
    ui->lightBatterySpinbox->setValue(settings.value("light_battery_threshold").toInt());
    ui->notificationBatterySpinbox->setValue(settings.value("notification_battery_threshold").toInt());
    ui->sidetoneSlider->setValue(settings.value("sidetone").toInt());
    ui->themeComboBox->setCurrentIndex(settings.value("theme").toInt());
    setSidetone();
    toggleLED();
}

void HeadsetControlQt::saveSettings()
{
    settings["led_state"] = ui->ledBox->isChecked();
    settings["light_battery_threshold"] = ui->lightBatterySpinbox->value();
    settings["notification_battery_threshold"] = ui->notificationBatterySpinbox->value();
    settings["sidetone"] = ui->sidetoneSlider->value();
    settings["theme"] = ui->themeComboBox->currentIndex();

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
    process.start("dependencies/headsetcontrol.exe", QStringList() << "-o" << "json");

    if (!process.waitForStarted()) {
        qDebug() << "Failed to start process:" << process.errorString();
        return;
    }

    if (!process.waitForFinished()) {
        qDebug() << "Process did not finish successfully:" << process.errorString();
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
            manageLEDBasedOnBattery(headsetInfo);
            sendNotificationBasedOnBattery(headsetInfo);
        } else {
            qDebug() << "No devices found.";
            noDeviceFound();
        }
    } else {
        qDebug() << "No 'devices' key found in JSON.";
        noDeviceFound();
    }
}

void HeadsetControlQt::manageLEDBasedOnBattery(const QJsonObject &headsetInfo)
{
    if (!ui->ledBox->isChecked()) {
        return;
    }

    ui->lightBatterySpinbox->setEnabled(true);
    ui->lightBatteryLabel->setEnabled(true);

    QJsonObject batteryInfo = headsetInfo["battery"].toObject();
    int batteryLevel = batteryInfo["level"].toInt();
    QString batteryStatus = batteryInfo["status"].toString();
    bool available = (batteryStatus == "BATTERY_AVAILABLE");

    if (batteryLevel < ui->lightBatterySpinbox->value() && ledState && available) {
        toggleLED();
        ledState = false;
        saveSettings();
    } else if (batteryLevel >= ui->lightBatterySpinbox->value() + 5 && !ledState && available) {
        toggleLED();
        ledState = true;
        saveSettings();
    }
}

void HeadsetControlQt::sendNotificationBasedOnBattery(const QJsonObject &headsetInfo)
{
    QJsonObject batteryInfo = headsetInfo["battery"].toObject();
    QString headsetName = headsetInfo["device"].toString();
    int batteryLevel = batteryInfo["level"].toInt();
    QString batteryStatus = batteryInfo["status"].toString();
    bool available = (batteryStatus == "BATTERY_AVAILABLE");

    if (batteryLevel < ui->notificationBatterySpinbox->value() && !notificationSent && available) {
        sendNotification("Low battery", QString("%1 has %2% battery left.").arg(headsetName).arg(batteryLevel), QIcon(":/icons/icon.png"), 3000);
        notificationSent = true;
    } else if (batteryLevel >= ui->notificationBatterySpinbox->value() + 5 && notificationSent && available) {
        notificationSent = false;
    }
}

void HeadsetControlQt::sendNotification(const QString &title, const QString &message, const QIcon &icon, int duration)
{
    trayIcon->showMessage(title, message, icon, duration);
}

void HeadsetControlQt::toggleLED()
{
    QProcess process;
    process.start("dependencies/headsetcontrol.exe", QStringList() << "-l" << (ui->ledBox->isChecked() ? "1" : "0"));
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
        trayIcon->setToolTip(QString("Battery Level: %1%").arg(batteryLevel));

        QString iconPath = getBatteryIcon(batteryLevel, false, false);
        trayIcon->setIcon(QIcon(iconPath));
    } else if (batteryStatus == "BATTERY_CHARGING") {
        ui->batteryBar->setValue(0);
        ui->batteryBar->setFormat("Charging");
        trayIcon->setToolTip("Battery Charging");

        QString iconPath = getBatteryIcon(batteryLevel, true, false);
        trayIcon->setIcon(QIcon(iconPath));
    } else {
        ui->batteryBar->setValue(0);
        ui->batteryBar->setFormat("Off");
        trayIcon->setToolTip("Battery Unavailable");

        QString iconPath = getBatteryIcon(batteryLevel, false, true);
        trayIcon->setIcon(QIcon(iconPath));
    }

    ui->ledBox->setEnabled(capabilities.contains("lights"));
    ui->ledLabel->setEnabled(capabilities.contains("lights"));

    ui->sidetoneSlider->setEnabled(capabilities.contains("sidetone"));
    ui->sidetoneLabel->setEnabled(capabilities.contains("sidetone"));

    toggleUIElements(true);
}

QString HeadsetControlQt::getBatteryIcon(int batteryLevel, bool charging, bool missing)
{
    QString theme;
    int themeIndex = ui->themeComboBox->currentIndex();
    if (themeIndex == 0) {
        theme = getTheme();
    } else if (themeIndex == 1) {
        theme = "light";
    } else if (themeIndex == 2) {
        theme = "dark";
    }

    QString iconName;
    if (missing) {
        iconName = QString("battery-missing-%1").arg(theme);
    } else if (charging) {
        iconName = "battery-100-charging-" + theme;
    } else {
        if (batteryLevel >= 90) iconName = "battery-100-" + theme;
        else if (batteryLevel >= 80) iconName = "battery-090-" + theme;
        else if (batteryLevel >= 70) iconName = "battery-080-" + theme;
        else if (batteryLevel >= 60) iconName = "battery-070-" + theme;
        else if (batteryLevel >= 50) iconName = "battery-060-" + theme;
        else if (batteryLevel >= 40) iconName = "battery-050-" + theme;
        else if (batteryLevel >= 30) iconName = "battery-040-" + theme;
        else if (batteryLevel >= 20) iconName = "battery-030-" + theme;
        else if (batteryLevel >= 10) iconName = "battery-020-" + theme;
        else iconName = "battery-010-" + theme;
    }

    return QString(":/icons/%1.png").arg(iconName);
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
    this->setMinimumSize(0, 0);
    this->adjustSize();
}

void HeadsetControlQt::onLedBoxStateChanged()
{
    toggleLED();
    ui->lightBatterySpinbox->setEnabled(ui->ledBox->isChecked());
    ui->lightBatteryLabel->setEnabled(ui->ledBox->isChecked());
    saveSettings();
}

void HeadsetControlQt::onLightBatterySpinboxValueChanged(int value)
{
    saveSettings();
}

void HeadsetControlQt::onNotificationBatterySpinboxValueChanged(int value)
{
    saveSettings();
}

void HeadsetControlQt::onStartupCheckBoxStateChanged()
{
    if (ui->startupCheckbox->isChecked()) {
        manageShortcut(true);
    } else {
        manageShortcut(false);
    }
}

void HeadsetControlQt::onSidetoneSliderSliderReleased()
{
    setSidetone();
    saveSettings();
}

void HeadsetControlQt::onThemeComboBoxCurrentIndexChanged(int index)
{
    updateHeadsetInfo();
    saveSettings();
}

void HeadsetControlQt::setSidetone()
{
    QProcess process;
    process.start("dependencies/headsetcontrol.exe", QStringList() << "-s" << QString::number(ui->sidetoneSlider->value()));
    process.waitForFinished();
}

void HeadsetControlQt::toggleWindow()
{
    if (isVisible()) {
        hide();
        trayIcon->contextMenu()->actions().first()->setText("Show");
    } else {
        show();
        trayIcon->contextMenu()->actions().first()->setText("Hide");
    }
}

void HeadsetControlQt::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        toggleWindow();
    }
}

void HeadsetControlQt::exitApp()
{
    trayIcon->hide();
    QApplication::quit();
}

void HeadsetControlQt::checkStartupCheckbox()
{
    if (isShortcutPresent()) {
        ui->startupCheckbox->setChecked(true);
    }
}

void HeadsetControlQt::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
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
