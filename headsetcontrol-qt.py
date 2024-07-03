import sys
import subprocess
import json
import signal
import os
import platform
import darkdetect
from PyQt6.QtWidgets import QApplication, QMainWindow, QSystemTrayIcon, QMenu, QMessageBox
from PyQt6.QtGui import QIcon, QAction
from PyQt6.QtCore import QTimer
from PyQt6 import uic

if platform.system() == "Linux":
    SETTINGS_DIR = os.path.join(os.path.expanduser("~"), ".config", "headsetcontrol-qt")
    HEADSETCONTROL_EXECUTABLE = "headsetcontrol"
elif platform.system() == "Windows":
    SETTINGS_DIR = os.path.join(os.getenv("APPDATA"), "headsetcontrol-qt")
    HEADSETCONTROL_EXECUTABLE = os.path.join("dependencies", "headsetcontrol.exe")

SETTINGS_FILE = os.path.join(SETTINGS_DIR, "settings.json")

if not os.path.exists(SETTINGS_DIR):
    os.makedirs(SETTINGS_DIR)

class HeadsetControlApp(QMainWindow):
    def __init__(self):
        super().__init__()
        ui_path = os.path.join("design.ui")
        uic.loadUi(ui_path, self)
        self.led_state = None
        self.init_ui()
        self.read_settings()
        self.update_headset_info()
        self.init_timer()

    def init_ui(self):
        self.setWindowTitle("HeadsetControl-Qt")

        if platform.system() == "Linux":
            icon = QIcon.fromTheme("audio-headset-symbolic")
        elif platform.system() == "Windows":
            dark_mode = darkdetect.isDark()
            icon_path = os.path.join("icons", "icon_dark.png") if not dark_mode else os.path.join("icons", "icon_light.png")
            icon = QIcon(icon_path)

        self.setWindowIcon(icon)
        self.tray_icon = QSystemTrayIcon(self)
        self.tray_icon.setIcon(icon)

        tray_menu = QMenu(self)
        show_action = QAction("Show", self)
        show_action.triggered.connect(self.show_window)
        tray_menu.addAction(show_action)

        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.exit_app)
        tray_menu.addAction(exit_action)

        self.tray_icon.setContextMenu(tray_menu)
        self.tray_icon.show()

        self.batteryBar.setTextVisible(False)
        self.ledBox.stateChanged.connect(self.on_ledbox_state_changed)

        self.installEventFilter(self)

    def init_timer(self):
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_headset_info)
        self.timer.start(10000)

    def read_settings(self):
        if os.path.exists(SETTINGS_FILE):
            with open(SETTINGS_FILE, 'r') as f:
                settings = json.load(f)
                self.led_state = settings.get("led_state", True)
        else:
            self.save_settings()
        
        self.ledBox.setChecked(self.led_state)

    def save_settings(self):
        settings = {
            "led_state": self.ledBox.isChecked()
        }
        try:
            with open(SETTINGS_FILE, 'w') as f:
                json.dump(settings, f, indent=4)
        except Exception as e:
            print(f"Error saving settings: {e}")

    def update_headset_info(self):
        try:
            result = subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-o', 'json'], 
                                      stdout=subprocess.PIPE, 
                                      stderr=subprocess.PIPE, 
                                      text=True, 
                                      creationflags=subprocess.CREATE_NO_WINDOW if platform.system() == "Windows" else 0)
            stdout, stderr = result.communicate(timeout=10)
            if result.returncode == 0:
                data = json.loads(stdout)
                if 'devices' in data and len(data['devices']) > 0:
                    headset_info = data['devices'][0]
                    self.update_ui_with_headset_info(headset_info)
                else:
                    self.no_device_found()
            else:
                print("Error running headsetcontrol:", stderr)
                self.no_device_found()
        except Exception as e:
            print("Exception occurred:", str(e))
            self.no_device_found()

    def update_ui_with_headset_info(self, headset_info):
        device_name = headset_info.get("device", "Unknown Device")
        capabilities = headset_info.get("capabilities_str", [])
        battery_info = headset_info.get("battery", {})

        self.deviceLabel.setText(f"{device_name}")

        battery_status = battery_info.get("status", "UNKNOWN")
        if battery_status == "BATTERY_AVAILABLE":
            battery_level = battery_info.get("level", 0)
            self.batteryBar.setEnabled(True)
            self.batteryBar.setValue(battery_level)
            self.statusLabel.setText(f"{battery_level}%")
            self.tray_icon.setToolTip(f"Battery Level: {battery_level}%")
        elif battery_status == "BATTERY_CHARGING":
            self.batteryBar.setEnabled(True)
            self.batteryBar.setValue(0)
            self.statusLabel.setText("Charging")
            self.tray_icon.setToolTip("Battery Charging")
        else:
            self.batteryBar.setEnabled(False)
            self.batteryBar.setValue(0)
            self.statusLabel.setText("Off")
            self.tray_icon.setToolTip("Battery Unavailable")

        if "lights" in capabilities:
            self.ledBox.setEnabled(True)
        else:
            self.ledBox.setEnabled(False)

        self.show_ui_elements()

    def no_device_found(self):
        self.hide_ui_elements()
        self.tray_icon.setToolTip("No Device Found")

    def on_ledbox_state_changed(self, state):
        self.save_settings()

        try:
            if state == 2:
                subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-l', '1'], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE, 
                                 text=True, 
                                 creationflags=subprocess.CREATE_NO_WINDOW if platform.system() == "Windows" else 0)
            else:
                subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-l', '0'], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE, 
                                 text=True, 
                                 creationflags=subprocess.CREATE_NO_WINDOW if platform.system() == "Windows" else 0)
        except Exception as e:
            print(f"Error running headsetcontrol: {e}")
            QMessageBox.warning(self, "Error", "Failed to change LED settings.")

    def show_ui_elements(self):
        self.deviceLabel.show()
        self.batteryBar.show()
        self.ledBox.show()
        self.statusLabel.show()
        self.ledLabel.show()
        self.batteryLabel.show()
        self.notFoundLabel.hide()

    def hide_ui_elements(self):
        self.deviceLabel.hide()
        self.batteryBar.hide()
        self.ledBox.hide()
        self.statusLabel.hide()
        self.ledLabel.hide()
        self.batteryLabel.hide()
        self.notFoundLabel.show()

    def show_window(self):
        self.show()

    def exit_app(self):
        self.tray_icon.hide()
        QApplication.quit()

    def closeEvent(self, event):
        event.ignore()
        self.hide()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = HeadsetControlApp()
    
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    
    sys.exit(app.exec())
