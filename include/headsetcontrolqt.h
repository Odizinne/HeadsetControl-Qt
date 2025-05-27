#ifndef HEADSETCONTROLQT_H
#define HEADSETCONTROLQT_H

#include "worker.h"
#include "hideventmonitor.h"
#include <QWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QCloseEvent>
#include <QThread>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QProperty>

class HeadsetControlQt : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged FINAL)
    Q_PROPERTY(int batteryLevel READ batteryLevel WRITE setBatteryLevel NOTIFY batteryLevelChanged FINAL)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool lightsCapable READ lightsCapable WRITE setLightsCapable NOTIFY lightsCapableChanged FINAL)
    Q_PROPERTY(bool chatmixCapable READ chatmixCapable WRITE setChatmixCapable NOTIFY chatmixCapableChanged FINAL)
    Q_PROPERTY(bool sidetoneCapable READ sidetoneCapable WRITE setSidetoneCapable NOTIFY sidetoneCapableChanged FINAL)
    Q_PROPERTY(bool soundNotifCapable READ soundNotifCapable WRITE setSoundNotifCapable NOTIFY soundNotifCapableChanged FINAL)
    Q_PROPERTY(bool noDevice READ noDevice WRITE setNoDevice NOTIFY noDeviceChanged FINAL)
    Q_PROPERTY(bool isRunAtStartup READ isRunAtStartup NOTIFY isRunAtStartupChanged FINAL)
    Q_PROPERTY(int chatmix READ chatmix NOTIFY chatmixChanged)

public:
    explicit HeadsetControlQt(QWidget *parent = nullptr);
    ~HeadsetControlQt();

    // Property getters
    QString deviceName() const { return m_deviceName; }
    int batteryLevel() const { return m_batteryLevel; }
    QString status() const { return m_status; }
    bool lightsCapable() const { return m_lightsCapable; }
    bool sidetoneCapable() const { return m_sidetoneCapable; }
    bool soundNotifCapable() const { return m_soundNotifCapable; }
    bool noDevice() const { return m_noDevice; }
    bool isRunAtStartup() const { return m_isRunAtStartup; }
    bool chatmixCapable() const { return m_chatmixCapable; }

    Q_INVOKABLE void setSidetone(int value);
    Q_INVOKABLE void toggleLED(bool state);
    Q_INVOKABLE void changeApplicationLanguage(int languageIndex);
    Q_INVOKABLE void updateHeadsetInfo();
    Q_INVOKABLE void sendFirstMinimizeNotification();
    Q_INVOKABLE void setRunAtStartup(bool enable);

    int chatmix() const { return m_chatmix; }

public slots:
    // Property setters
    void setDeviceName(const QString &deviceName) {
        if (m_deviceName != deviceName) {
            m_deviceName = deviceName;
            emit deviceNameChanged();
        }
    }

    void setBatteryLevel(int batteryLevel) {
        if (m_batteryLevel != batteryLevel) {
            m_batteryLevel = batteryLevel;
            emit batteryLevelChanged();
        }
    }

    void setStatus(const QString &status) {
        if (m_status != status) {
            m_status = status;
            emit statusChanged();
        }
    }

    void setLightsCapable(bool capable) {
        if (m_lightsCapable != capable) {
            m_lightsCapable = capable;
            emit lightsCapableChanged();
        }
    }

    void setSidetoneCapable(bool capable) {
        if (m_sidetoneCapable != capable) {
            m_sidetoneCapable = capable;
            emit sidetoneCapableChanged();
        }
    }

    void setSoundNotifCapable(bool capable) {
        if (m_soundNotifCapable != capable) {
            m_soundNotifCapable = capable;
            emit soundNotifCapableChanged();
        }
    }

    void setNoDevice(bool noDevice) {
        if (m_noDevice != noDevice) {
            m_noDevice = noDevice;
            emit noDeviceChanged();
        }
    }

    void setChatmixCapable(bool capable) {
        if (m_chatmixCapable != capable) {
            m_chatmixCapable = capable;
            emit chatmixCapableChanged();
        }
    }

signals:
    // Property change signals
    void deviceNameChanged();
    void batteryLevelChanged();
    void statusChanged();
    void lightsCapableChanged();
    void sidetoneCapableChanged();
    void soundNotifCapableChanged();
    void noDeviceChanged();
    void isRunAtStartupChanged();
    void chatmixChanged();
    void chatmixCapableChanged();

private slots:
    void toggleWindow();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void handleHeadsetInfo(const QJsonObject &headsetInfo);
    void reflectWindowState(QWindow::Visibility visibility);
    void updateTrayChargingAnimation();

private:
    // Property members
    QString m_deviceName;
    int m_batteryLevel{0};
    QString m_status;
    bool m_lightsCapable{false};
    bool m_sidetoneCapable{false};
    bool m_soundNotifCapable{false};
    bool m_noDevice{true};
    bool m_isRunAtStartup{false};
    int m_chatmix = 64;
    bool m_chatmixCapable{false};

    void createTrayIcon();
    void manageLEDBasedOnBattery(const QJsonObject &headsetInfo);
    void sendNotificationBasedOnBattery(const QJsonObject &headsetInfo);
    void sendNotification(const QString &title, const QString &message, const QIcon &icon, int duration);
    void sendSoundNotification();
    void sendSoundNotificationBasedOnBattery(const QJsonObject &headsetInfo);
    void updateUIWithHeadsetInfo(const QJsonObject &headsetInfo);
    void noDeviceFound();
    void updateTrayMenu();

    static const QString headsetcontrolExecutable;
    static const QString desktopFile;
    QSettings settings;
    QQmlApplicationEngine *engine;
    QSystemTrayIcon *trayIcon;
    QTimer *fetchTimer;
    QTimer *chargingAnimationTimer;
    bool ledDisabled;
    bool notificationSent;
    bool soundNotificationSent;
    bool firstRun;
    bool closing;
    QTranslator *translator;
    QMenu *trayMenu;
    QAction *exitAction;
    QAction *showAction;
    QThread workerThread;
    Worker *worker;
    QWindow *qmlWindow;
    HIDEventMonitor *usbMonitor;
    int currentChargingFrame{20};

    bool isAudioSystemAvailable(const QString &system);
    QString getActiveAudioSystem();
    void createChatMixScripts();
    void runChatMixSetup();
    void updateChatMixVolumes(int mixLevel);
    bool chatMixSetupRan;
    int lastMixLevel;
};

#endif // HEADSETCONTROLQT_H
