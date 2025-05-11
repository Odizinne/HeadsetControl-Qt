#include "HIDEventMonitor.h"

#ifdef _WIN32

#include <Dbt.h> // Required for device broadcast definitions
#include <QDebug>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // Windows XP or later
#endif

HIDEventMonitor::HIDEventMonitor(QObject *parent) : QObject(parent), _hiddenWindow(nullptr), _notificationHandle(nullptr) {}

HIDEventMonitor::~HIDEventMonitor() {
    stopMonitoring();
}

void HIDEventMonitor::startMonitoring() {
    // Create a hidden window for receiving device change messages
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"HIDEventMonitor";
    RegisterClass(&wc);

    _hiddenWindow = CreateWindow(wc.lpszClassName, L"", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    SetWindowLongPtr(_hiddenWindow, GWLP_USERDATA, (LONG_PTR)this);

    registerForDeviceNotifications(_hiddenWindow);
}

void HIDEventMonitor::stopMonitoring() {
    if (_notificationHandle) {
        UnregisterDeviceNotification(_notificationHandle);
        _notificationHandle = nullptr;
    }

    if (_hiddenWindow) {
        DestroyWindow(_hiddenWindow);
        _hiddenWindow = nullptr;
    }
}

void HIDEventMonitor::registerForDeviceNotifications(HWND hwnd) {
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter = {0};
    notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    // HID device interface class GUID
    const GUID HID_GUID = {0x4D1E55B2, 0xF16F, 0x11CF, {0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}};
    notificationFilter.dbcc_classguid = HID_GUID;

    _notificationHandle = RegisterDeviceNotification(hwnd, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (!_notificationHandle) {
        qDebug() << "Failed to register for device notifications.";
    }
}

LRESULT CALLBACK HIDEventMonitor::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DEVICECHANGE) {
        auto monitor = reinterpret_cast<HIDEventMonitor*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (monitor) {
            if (wParam == DBT_DEVICEARRIVAL) {
                emit monitor->deviceAdded("HID Device Connected");
            } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
                emit monitor->deviceRemoved("HID Device Removed");
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#elif __linux__

#include <QDebug>

HIDEventMonitor::HIDEventMonitor(QObject *parent) : QObject(parent), udevContext(nullptr), monitoring(false) {}

HIDEventMonitor::~HIDEventMonitor() {
    stopMonitoring();
}

void HIDEventMonitor::startMonitoring() {
    udevContext = udev_new();
    if (!udevContext) {
        qDebug() << "Failed to initialize udev context.";
        return;
    }

    monitoring = true;
    usbMonitorThread = std::thread(&HIDEventMonitor::monitorUSBChanges, this);
}

void HIDEventMonitor::stopMonitoring() {
    monitoring = false;
    if (usbMonitorThread.joinable()) {
        usbMonitorThread.join();
    }

    if (udevContext) {
        udev_unref(udevContext);
        udevContext = nullptr;
    }
}

void HIDEventMonitor::monitorUSBChanges() {
    struct udev_monitor *udevMonitor = udev_monitor_new_from_netlink(udevContext, "udev");
    if (!udevMonitor) {
        qDebug() << "Failed to create udev monitor.";
        return;
    }

    // Filter for HID devices (part of "input" subsystem)
    udev_monitor_filter_add_match_subsystem_devtype(udevMonitor, "input", NULL);
    udev_monitor_enable_receiving(udevMonitor);

    int monitorFd = udev_monitor_get_fd(udevMonitor);

    while (monitoring) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(monitorFd, &fds);

        struct timeval timeout = {1, 0}; // 1-second timeout
        int ret = select(monitorFd + 1, &fds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(monitorFd, &fds)) {
            struct udev_device *device = udev_monitor_receive_device(udevMonitor);
            if (device) {
                const char *action = udev_device_get_action(device);
                const char *subsystem = udev_device_get_subsystem(device);
                const char *devNode = udev_device_get_devnode(device);

                if (action && subsystem && devNode) {
                    if (strcmp(action, "add") == 0) {
                        emit deviceAdded(QString("HID Device Connected: %1").arg(devNode));
                    } else if (strcmp(action, "remove") == 0) {
                        emit deviceRemoved(QString("HID Device Removed: %1").arg(devNode));
                    }
                }
                udev_device_unref(device);
            }
        }
    }

    udev_monitor_unref(udevMonitor);
}

#endif // __linux__
