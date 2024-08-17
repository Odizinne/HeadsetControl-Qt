#ifndef HEADSETCONTROLQT_H
#define HEADSETCONTROLQT_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QTranslator>
#include <QLocale>
#include <QCloseEvent>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>

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

private:
    void initUI();
    void populateComboBoxes();
    void checkStartupCheckbox();
    void createTrayIcon();
    void loadSettings();
    void applySettings();
    void saveSettings();
    void createDefaultSettings();
    void manageLEDBasedOnBattery(const QJsonObject &headsetInfo);
    void sendNotificationBasedOnBattery(const QJsonObject &headsetInfo);
    void sendNotification(const QString &title, const QString &message, const QIcon &icon, int duration);
    void toggleLED();
    void setSidetone();
    void setFont();
    void updateUIWithHeadsetInfo(const QJsonObject &headsetInfo);
    void noDeviceFound();
    void toggleUIElements(bool show);
    static const QString settingsFile;
    static const QString headsetcontrolExecutable;
    static const QString desktopFile;

    Ui::HeadsetControlQt *ui;
    QSystemTrayIcon *trayIcon;
    QTimer *timer;
    QJsonObject settings;
    bool ledState;
    bool notificationSent;
    bool firstRun;

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // HEADSETCONTROLQT_H
