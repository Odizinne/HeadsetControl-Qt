#ifndef HEADSETCONTROLQT_H
#define HEADSETCONTROLQT_H

#include "Worker.h"
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QCloseEvent>
#include <QThread>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui {
class HeadsetControlQt;
}
QT_END_NAMESPACE

class HeadsetControlQt : public QMainWindow
{
    Q_OBJECT

public:
    explicit HeadsetControlQt(QWidget *parent = nullptr);
    ~HeadsetControlQt();

private slots:
    void onLedBoxStateChanged();
    void onSidetoneSliderSliderReleased();
    void onThemeComboBoxCurrentIndexChanged();
    void updateHeadsetInfo();
    void toggleWindow();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onStartupCheckBoxStateChanged();
    void handleHeadsetInfo(const QJsonObject &headsetInfo);
    void changeApplicationLanguage();

private:
    void initUI();
    void setupUIConnections();
    void populateComboBoxes();
    void checkStartupCheckbox();
    void createTrayIcon();
    void createTrayMenu();
    void loadSettings();
    void applySettings();
    void saveSettings();
    void createDefaultSettings();
    void manageLEDBasedOnBattery(const QJsonObject &headsetInfo);
    void sendNotificationBasedOnBattery(const QJsonObject &headsetInfo);
    void sendNotification(const QString &title, const QString &message, const QIcon &icon, int duration);
    void toggleLED(bool state);
    void setSidetone();
    void sendSoundNotification();
    void sendSoundNotificationBasedOnBattery(const QJsonObject &headsetInfo);
    void updateUIWithHeadsetInfo(const QJsonObject &headsetInfo);
    void noDeviceFound();
    void toggleUIElements(bool show);
    void sendFirstMinimizeNotification();
    void updateTrayMenu();
    static const QString headsetcontrolExecutable;
    static const QString desktopFile;
    QSettings settings;

    Ui::HeadsetControlQt *ui;
    QSystemTrayIcon *trayIcon;
    QTimer *timer;
    bool ledDisabled;
    bool notificationSent;
    bool soundNotificationSent;
    bool firstRun;
    bool closing;
    QMenu *trayMenu;
    QAction *exitAction;
    QAction *showAction;

    QThread workerThread;
    Worker *worker;

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // HEADSETCONTROLQT_H
