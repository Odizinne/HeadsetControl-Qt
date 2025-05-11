#ifndef HIDEventMonitor_H
#define HIDEventMonitor_H

#include <QObject>

#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <libudev.h>
#include <thread>
#endif

class HIDEventMonitor : public QObject {
    Q_OBJECT

public:
    explicit HIDEventMonitor(QObject *parent = nullptr);
    ~HIDEventMonitor();

    void startMonitoring();
    void stopMonitoring();

signals:
    void deviceAdded(const QString &deviceName);
    void deviceRemoved(const QString &deviceName);

private:
#ifdef _WIN32
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void registerForDeviceNotifications(HWND hwnd);
    HWND _hiddenWindow;
    HDEVNOTIFY _notificationHandle;
#elif __linux__
    void monitorUSBChanges();
    struct udev *udevContext;
    std::thread usbMonitorThread;
    bool monitoring;
#endif
};

#endif // HIDEventMonitor_H
