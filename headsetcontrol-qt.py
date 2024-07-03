import sys
import subprocess
import json
import signal
import os
import platform
from PyQt6.QtWidgets import QApplication, QMainWindow, QSystemTrayIcon, QMenu, QMessageBox
from PyQt6.QtGui import QIcon, QAction
from PyQt6.QtCore import QTimer
from PyQt6 import uic

if platform.system() == "Linux":
    SETTINGS_DIR = os.path.join(os.path.expanduser("~"), ".config", "headsetcontrol-qt")
    HEADSETCONTROL_EXECUTABLE = "headsetcontrol"
    DESKTOP_FILE_PATH = os.path.join(os.path.expanduser("~"), ".config", "autostart", "headsetcontrol-qt.desktop")
elif platform.system() == "Windows":
    SETTINGS_DIR = os.path.join(os.getenv("APPDATA"), "headsetcontrol-qt")
    HEADSETCONTROL_EXECUTABLE = os.path.join("dependencies", "headsetcontrol.exe")
    import winshell
    import darkdetect
    STARTUP_FOLDER = winshell.startup()

SETTINGS_FILE = os.path.join(SETTINGS_DIR, "settings.json")

if not os.path.exists(SETTINGS_DIR):
    os.makedirs(SETTINGS_DIR)

class HeadsetControlApp(QMainWindow):
    def __init__(self):
        super().__init__()
        ui_path = os.path.join("design.ui")
        uic.loadUi(ui_path, self)
        self.led_state = None
        self.light_battery_threshold = None
        self.init_ui()
        self.read_settings()
        self.update_headset_info()
        self.init_timer()
        self.check_startup_checkbox()

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
        self.lightBatterySpinbox.setRange(0, 100)
        self.installEventFilter(self)
        self.lightBatterySpinbox.valueChanged.connect(self.save_settings)
        self.startupCheckbox.stateChanged.connect(self.on_startupCheckbox_state_changed)

    def init_timer(self):
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_headset_info)
        self.timer.start(10000)

    def read_settings(self):
        if not os.path.exists(SETTINGS_FILE):
            self.save_settings()
        with open(SETTINGS_FILE, 'r') as f:
            settings = json.load(f)
            self.led_state = settings.get("led_state", True)
            if self.led_state is True:
                self.lightBatterySpinbox.setEnabled(True)
                self.lightBatteryLabel.setEnabled(True)
            else:
                self.lightBatterySpinbox.setEnabled(False)
                self.lightBatteryLabel.setEnabled(False)
            self.light_battery_threshold = settings.get("light_battery_threshold", 50)

        self.ledBox.setChecked(self.led_state)
        self.lightBatterySpinbox.setValue(self.light_battery_threshold)

    def save_settings(self):
        settings = {
            "led_state": self.ledBox.isChecked(),
            "light_battery_threshold": self.lightBatterySpinbox.value()
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
                    self.manage_led_based_on_battery(headset_info)
                else:
                    self.no_device_found()
            else:
                print("Error running headsetcontrol:", stderr)
                self.no_device_found()
        except Exception as e:
            print("Exception occurred:", str(e))
            self.no_device_found()

    def manage_led_based_on_battery(self, headset_info):
        if not self.ledBox.isChecked():
            return
        
        self.lightBatterySpinbox.setEnabled(True)
        self.lightBatteryLabel.setEnabled(True)
        battery_info = headset_info.get("battery", {})
        battery_level = battery_info.get("level", 0)

        if battery_level < self.lightBatterySpinbox.value() and self.led_state:
            self.toggle_led(False)
            self.led_state = False
            self.save_settings()
        elif battery_level >= self.lightBatterySpinbox.value() and not self.led_state:
            self.toggle_led(True)
            self.led_state = True
            self.save_settings()

    def toggle_led(self, state):
        try:
            subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-l', '1' if state else '0'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             text=True,
                             creationflags=subprocess.CREATE_NO_WINDOW if platform.system() == "Windows" else 0)
        except Exception as e:
            print(f"Error running headsetcontrol: {e}")
            QMessageBox.warning(self, "Error", "Failed to change LED settings.")

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

        self.toggle_ui_elements(True)

    def no_device_found(self):
        self.toggle_ui_elements(False)
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
                self.lightBatterySpinbox.setEnabled(True)
                self.lightBatteryLabel.setEnabled(True)
            else:
                subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-l', '0'],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE,
                                 text=True,
                                 creationflags=subprocess.CREATE_NO_WINDOW if platform.system() == "Windows" else 0)
                self.lightBatterySpinbox.setEnabled(False)
                self.lightBatteryLabel.setEnabled(False)
        except Exception as e:
            print(f"Error running headsetcontrol: {e}")
            QMessageBox.warning(self, "Error", "Failed to change LED settings.")

    def toggle_ui_elements(self, show: bool):
        self.deviceLabel.setVisible(show)
        self.batteryBar.setVisible(show)
        self.ledBox.setVisible(show)
        self.statusLabel.setVisible(show)
        self.ledLabel.setVisible(show)
        self.batteryLabel.setVisible(show)
        self.lightBatterySpinbox.setVisible(show)
        self.lightBatteryLabel.setVisible(show)
        self.startupLabel.setVisible(show)
        self.startupCheckbox.setVisible(show)
        self.notFoundLabel.setVisible(not show)

    def show_window(self):
        self.show()

    def exit_app(self):
        self.tray_icon.hide()
        QApplication.quit()

    def closeEvent(self, event):
        event.ignore()
        self.hide()

    def on_startupCheckbox_state_changed(self, state):
        if platform.system() == "Windows":
            self.set_windows_startup(state)
        elif platform.system() == "Linux":
            self.set_linux_startup(state)

    def set_windows_startup(self, state):
        try:
            startup_folder = winshell.startup()
            shortcut_path = os.path.join(startup_folder, "HeadsetControl-Qt.lnk")
            target_path = sys.executable
            working_directory = os.path.dirname(target_path)

            if state == 2:  # Checked
                winshell.CreateShortcut(
                    Path=shortcut_path,
                    Target=target_path,
                    Icon=(target_path, 0),
                    Description="Launch HeadsetControl-Qt",
                    StartIn=working_directory
                )
            else:  # Unchecked
                if os.path.exists(shortcut_path):
                    os.remove(shortcut_path)
        except Exception as e:
            print(f"Error managing startup shortcut on Windows: {e}")

    def set_linux_startup(self, state):
        try:
            if state == 2:  # Checked
                if not os.path.exists(os.path.dirname(DESKTOP_FILE_PATH)):
                    os.makedirs(os.path.dirname(DESKTOP_FILE_PATH))
                with open(DESKTOP_FILE_PATH, 'w') as f:
                    f.write(f"""
                    [Desktop Entry]
                    Type=Application
                    Exec={sys.executable} {__file__}
                    Hidden=false
                    NoDisplay=false
                    X-GNOME-Autostart-enabled=true
                    Name=HeadsetControl-Qt
                    Comment=HeadsetControl-Qt
                    """)
            else:  # Unchecked
                if os.path.exists(DESKTOP_FILE_PATH):
                    os.remove(DESKTOP_FILE_PATH)
        except Exception as e:
            print(f"Error managing startup file on Linux: {e}")

    def check_startup_checkbox(self):
        if platform.system() == "Windows":
            startup_folder = winshell.startup()
            shortcut_path = os.path.join(startup_folder, "HeadsetControl-Qt.lnk")
            self.startupCheckbox.setChecked(os.path.exists(shortcut_path))
        elif platform.system() == "Linux":
            if os.path.exists(DESKTOP_FILE_PATH):
                self.startupCheckbox.setChecked(True)
            else:
                self.startupCheckbox.setChecked(False)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = HeadsetControlApp()

    signal.signal(signal.SIGINT, signal.SIG_DFL)

    sys.exit(app.exec())
