#!/usr/bin/env python3

import sys
import subprocess
import json
import signal
import os
from PyQt6.QtWidgets import QApplication, QMainWindow, QSystemTrayIcon, QMenu
from PyQt6.QtGui import QIcon, QAction
from PyQt6.QtCore import QTimer
from design import Ui_MainWindow

if sys.platform == "linux":
    SETTINGS_DIR = os.path.join(os.path.expanduser("~"), ".config", "headsetcontrol-qt")
    HEADSETCONTROL_EXECUTABLE = "headsetcontrol"
    DESKTOP_FILE_PATH = os.path.join(os.path.expanduser("~"), ".config", "autostart", "headsetcontrol-qt.desktop")
elif sys.platform == "win32":
    import winshell
    import darkdetect
    SETTINGS_DIR = os.path.join(os.getenv("APPDATA"), "headsetcontrol-qt")
    HEADSETCONTROL_EXECUTABLE = os.path.join("dependencies", "headsetcontrol.exe")
    STARTUP_FOLDER = winshell.startup()
    
ICONS_DIR = os.path.join("icons")
APP_ICON = os.path.join(ICONS_DIR, "icon.png")
SETTINGS_FILE = os.path.join(SETTINGS_DIR, "settings.json")

class HeadsetControlApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.setWindowTitle("HeadsetControl-Qt")
        self.setWindowIcon(QIcon(APP_ICON))
        self.setFixedSize(self.size())
        self.led_state = None
        self.light_battery_threshold = None
        self.init_ui()
        self.create_tray_icon()
        self.load_settings()
        self.update_headset_info()
        self.init_timer()
        self.check_startup_checkbox()
        self.set_sidetone()
        self.on_ledBox_state_changed()

    def init_ui(self):
        self.ui.ledBox.stateChanged.connect(self.on_ledBox_state_changed)
        self.ui.lightBatterySpinbox.valueChanged.connect(self.save_settings)
        self.ui.startupCheckbox.stateChanged.connect(self.on_startupCheckbox_state_changed)
        self.ui.sidetoneSlider.sliderReleased.connect(self.set_sidetone)

    def create_tray_icon(self):
        self.tray_icon = QSystemTrayIcon(self)
        self.tray_icon.setIcon(QIcon(APP_ICON))
        tray_menu = QMenu(self)
        show_action = QAction("Show", self)
        show_action.triggered.connect(self.show_window)
        tray_menu.addAction(show_action)
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(self.exit_app)
        tray_menu.addAction(exit_action)
        self.tray_icon.setContextMenu(tray_menu)
        self.tray_icon.show()
        self.tray_icon.activated.connect(self.tray_icon_activated)

    def tray_icon_activated(self, reason):
        if reason == QSystemTrayIcon.ActivationReason.Trigger:
            if self.isVisible():
                self.hide()
                self.tray_icon.contextMenu().actions()[0].setText("Show")
            else:
                self.show()
                self.tray_icon.contextMenu().actions()[0].setText("Hide")

    def init_timer(self):
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_headset_info)
        self.timer.start(10000)

    def load_settings(self):
        if not os.path.exists(SETTINGS_FILE):
            os.makedirs(SETTINGS_DIR, exist_ok=True)
            self.create_default_settings()
        with open(SETTINGS_FILE, 'r') as f:
            settings = json.load(f)
            self.led_state = settings.get("led_state", True)
            self.light_battery_threshold = settings.get("light_battery_threshold", 50)
            self.ui.ledBox.setChecked(self.led_state)
            self.ui.lightBatterySpinbox.setEnabled(self.led_state)
            self.ui.lightBatteryLabel.setEnabled(self.led_state)
            self.ui.lightBatterySpinbox.setValue(self.light_battery_threshold)
            self.ui.sidetoneSlider.setValue(settings.get("sidetone", 0))

    def save_settings(self):
        settings = {
            "led_state": self.ui.ledBox.isChecked(),
            "light_battery_threshold": self.ui.lightBatterySpinbox.value(),
            "sidetone": self.ui.sidetoneSlider.value()
        }
        with open(SETTINGS_FILE, 'w') as f:
            json.dump(settings, f, indent=4)

    def create_default_settings(self):
        settings = {
            "led_state": True,
            "light_battery_threshold": 50,
            "sidetone": 0
        }
        with open(SETTINGS_FILE, 'w') as f:
            json.dump(settings, f, indent=4)

    def update_headset_info(self):
        result = subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-o', 'json'],
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  text=True,
                                  creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0)
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

    def manage_led_based_on_battery(self, headset_info):
        if not self.ui.ledBox.isChecked():
            return
        
        self.ui.lightBatterySpinbox.setEnabled(True)
        self.ui.lightBatteryLabel.setEnabled(True)
        battery_info = headset_info.get("battery", {})
        battery_level = battery_info.get("level", 0)

        if battery_level < self.ui.lightBatterySpinbox.value() and self.led_state:
            self.toggle_led(False)
            self.led_state = False
            self.save_settings()
        elif battery_level >= self.ui.lightBatterySpinbox.value() and not self.led_state:
            self.toggle_led(True)
            self.led_state = True
            self.save_settings()

    def toggle_led(self, state):
        subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-l', '1' if state else '0'],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         text=True,
                         creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0)

    def update_ui_with_headset_info(self, headset_info):
        device_name = headset_info.get("device", "Unknown Device")
        capabilities = headset_info.get("capabilities_str", [])
        battery_info = headset_info.get("battery", {})

        self.ui.deviceLabel.setText(f"{device_name}")

        battery_status = battery_info.get("status", "UNKNOWN")
        if battery_status == "BATTERY_AVAILABLE":
            battery_level = battery_info.get("level", 0)
            self.ui.batteryBar.setEnabled(True)
            self.ui.batteryBar.setValue(battery_level)
            self.ui.statusLabel.setText(f"{battery_level}%")
            self.tray_icon.setToolTip(f"Battery Level: {battery_level}%")

            icon_path = self.get_battery_icon(battery_level, charging=False)
        elif battery_status == "BATTERY_CHARGING":
            self.ui.batteryBar.setEnabled(True)
            self.ui.batteryBar.setValue(0)
            self.ui.statusLabel.setText("Charging")
            self.tray_icon.setToolTip("Battery Charging")

            icon_path = self.get_battery_icon(battery_level=None, charging=True)
        else:
            self.ui.batteryBar.setEnabled(False)
            self.ui.batteryBar.setValue(0)
            self.ui.statusLabel.setText("Off")
            self.tray_icon.setToolTip("Battery Unavailable")

            icon_path = self.get_battery_icon(battery_level=None, missing=True)

        if sys.platform == "win32":
            self.tray_icon.setIcon(QIcon(icon_path))
        elif sys.platform == "linux":
            self.tray_icon.setIcon(QIcon.fromTheme(icon_path))

        if "lights" in capabilities:
            self.ui.ledBox.setEnabled(True)
            self.ui.ledLabel.setEnabled(True)
        else:
            self.ui.ledBox.setEnabled(False)
            self.ui.ledLabel.setEnabled(False)

        if "sidetone" in capabilities:
            self.ui.sidetoneSlider.setEnabled(True)
            self.ui.sidetoneLabel.setEnabled(True)
        else:
            self.ui.sidetoneSlider.setEnabled(False)
            self.ui.sidetoneLabel.setEnabled(False)

        self.toggle_ui_elements(True)

    def get_battery_icon(self, battery_level, charging=False, missing=False):
        if sys.platform == "win32":
            dark_mode = darkdetect.isDark()
            theme = "light" if dark_mode else "dark"
        elif sys.platform == "linux":
            # I will implement using system icons for linux later 
            dark_mode = False
            theme = "symbolic"

        if missing:
            icon_name = f"battery-missing-{theme}"
        elif charging:
            icon_name = f"battery-charging-{theme}"
        else:
            if battery_level is not None:
                battery_levels = {
                    90: "100",
                    80: "090",
                    70: "080",
                    60: "070",
                    50: "060",
                    40: "050",
                    30: "040",
                    20: "030",
                    10: "020",
                    0:  "010"
                }
                icon_name = None
                for level, percentage in battery_levels.items():
                    if battery_level >= level:
                        icon_name = f"battery-{percentage}-{theme}"
                        break
            else:
                icon_name = f"battery-missing-{theme}"

        if sys.platform == "win32":
            icon_name += ".png"
            icon_path = os.path.join(ICONS_DIR, icon_name)
            return icon_path
        elif sys.platform == "linux":
            icon_path = icon_name
            return icon_path

    def no_device_found(self):
        self.toggle_ui_elements(False)
        self.tray_icon.setToolTip("No Device Found")

    def on_ledBox_state_changed(self):
        lights = True if self.ui.ledBox.isChecked() else False
        self.toggle_led(lights)

        self.ui.lightBatterySpinbox.setEnabled(True if self.ui.ledBox.isChecked() else False)
        self.ui.lightBatteryLabel.setEnabled(True if self.ui.ledBox.isChecked() else False)
        self.save_settings()

    def set_sidetone(self):
        sidetone_value = self.ui.sidetoneSlider.value()
        subprocess.Popen([HEADSETCONTROL_EXECUTABLE, '-s', str(sidetone_value)],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         text=True,
                         creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0)
        self.save_settings()

    def toggle_ui_elements(self, show):
        self.ui.deviceLabel.setVisible(show)
        self.ui.statusLabel.setVisible(show)
        self.ui.frame.setVisible(show)
        self.ui.notFoundLabel.setVisible(not show)

    def show_window(self):
        self.show()

    def exit_app(self):
        self.tray_icon.hide()
        QApplication.quit()

    def closeEvent(self, event):
        event.ignore()
        self.hide()

    def on_startupCheckbox_state_changed(self):
        checked = self.ui.startupCheckbox.isChecked()

        if sys.platform == "win32":
            shortcut_path = os.path.join(winshell.startup(), "HeadsetControl-Qt.lnk")
            target_path = sys.executable
            working_directory = os.path.dirname(target_path)

            if checked:
                winshell.CreateShortcut(
                    Path=shortcut_path,
                    Target=target_path,
                    Icon=(target_path, 0),
                    Description="Launch HeadsetControl-Qt",
                    StartIn=working_directory
                )
            else:
                if os.path.exists(shortcut_path):
                    os.remove(shortcut_path)

        elif sys.platform == "linux":
            if checked:
                if not os.path.exists(os.path.dirname(DESKTOP_FILE_PATH)):
                    print("os.path.exists(os.path.dirname(DESKTOP_FILE_PATH))")
                    print(DESKTOP_FILE_PATH)
                    os.makedirs(os.path.dirname(DESKTOP_FILE_PATH))
                
                script_folder = os.path.dirname(__file__)
                desktop_entry_content = (
                    "[Desktop Entry]\n"
                    f"Path={script_folder}\n"
                    "Type=Application\n"
                    f"Exec={sys.executable} {__file__}\n"
                    "Name=HeadsetControl-Qt\n"
)
                with open(DESKTOP_FILE_PATH, 'w') as f:
                    f.write(desktop_entry_content)
            else:
                if os.path.exists(DESKTOP_FILE_PATH):
                    os.remove(DESKTOP_FILE_PATH)

    def check_startup_checkbox(self):
        if sys.platform == "win32":
            shortcut_path = os.path.join(winshell.startup(), "HeadsetControl-Qt.lnk")
            self.ui.startupCheckbox.setChecked(os.path.exists(shortcut_path))
        elif sys.platform == "linux":
            self.ui.startupCheckbox.setChecked(os.path.exists(DESKTOP_FILE_PATH))

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = HeadsetControlApp()
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    sys.exit(app.exec())
