import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick
import QtCore

ApplicationWindow {
    width: 450
    minimumWidth: 450
    maximumWidth: 450
    height: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins
    minimumHeight: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins
    maximumHeight: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins
    visible: false
    title: "HeadsetControl-Qt"
    Universal.theme: Universal.System
    Universal.accent: Universal.Green
    required property var mainWindow

    onClosing: {
        if (settings.firstRun) {
            mainWindow.sendFirstMinimizeNotification()
            settings.firstRun = false
        }
    }

    Label {
        opacity: mainWindow.noDevice ? 1 : 0
        enabled: mainWindow.noDevice ? true : false
        anchors.centerIn: parent
        text: qsTr("No supported headset found.")
    }

    Settings {
        id: settings
        property bool led_state: true
        property bool led_low_battery: false
        property bool notification_low_battery: false
        property int sidetone: 0
        property int theme: 0
        property int low_battery_threshold: 20
        property bool sound_low_battery: false
        property int language: 0
        property bool firstRun: true
    }

    Popup {
        id: lowBatteryActionsPopup
        parent: mainColumn
        anchors.centerIn: parent
        modal: true

        Shortcut {
            enabled: lowBatteryActionsPopup.visible
            sequence: "Esc"
            onActivated: lowBatteryActionsPopup.visible = false
        }

        ColumnLayout {
            spacing: 5

            RowLayout {
                spacing: 20
                enabled: mainWindow.lightsCapable

                Label {
                   text: qsTr("Disable lights")
                   Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -8
                    checked: settings.led_low_battery
                    onCheckedChanged: settings.led_low_battery = checked
                }
            }

            RowLayout {
                spacing: 20

                Label {
                   text: qsTr("Send notification")
                   Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -8
                    checked: settings.notification_low_battery
                    onCheckedChanged: settings.notification_low_battery = checked
                }
            }

            RowLayout {
                spacing: 20
                enabled: mainWindow.soundNotifCapable

                Label {
                   text: qsTr("Beep")
                   Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -8
                    checked: settings.sound_low_battery
                    onCheckedChanged: settings.sound_low_battery = checked
                }
            }

            Button {
                text: qsTr("Close")
                onClicked: lowBatteryActionsPopup.visible = false
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    ColumnLayout {
        id: mainColumn
        opacity: mainWindow.noDevice ? 0 : 1
        enabled: mainWindow.noDevice ? false : true
        anchors.fill: parent
        anchors.margins: 15
        spacing: 8

        RowLayout {
            spacing: 10

            Label {
                text: mainWindow.deviceName
                font.pixelSize: 20
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                text: mainWindow.status
                font.pixelSize: 20
            }
        }

        RowLayout {
            spacing: 20

            ProgressBar {
                id: batteryBar
                from: 0
                to: 100
                value: mainWindow.batteryLevel
                Layout.fillWidth: true
                indeterminate: mainWindow.status === "Charging"
                Universal.accent: mainWindow.status === "Charging" ?
                    Universal.Green :
                    Qt.rgba(
                        1 - (value / 100),
                        value / 100,
                        0,
                        1
                    )
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 20
            enabled: mainWindow.sidetoneCapable

            Label {
                text: qsTr("Sidetone")
            }

            Slider {
                from: 0
                to: 128
                value: settings.sidetone
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.leftMargin: -5
                Layout.rightMargin: -5
                onPressedChanged: {
                    if (!pressed) {
                        settings.sidetone = value
                        mainWindow.setSidetone(value)
                    }
                }
            }
        }

        RowLayout {
            spacing: 10
            enabled: mainWindow.lightsCapable
            Label {
               text: qsTr("Lights")
               Layout.fillWidth: true
            }

            Switch {
                Layout.rightMargin: -8
                checked: settings.led_state
                onCheckedChanged: {
                    settings.led_state = checked
                    mainWindow.toggleLED(checked)
                }
            }
        }

        RowLayout {
            spacing: 10

            Label {
                text: qsTr("Actions on low battery")
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Configure")
                onClicked: lowBatteryActionsPopup.visible = true
            }
        }

        RowLayout {
            Label {
                text: qsTr("Low battery threshold")
                Layout.fillWidth: true
            }

            SpinBox {
                from: 10
                to: 30
                value: settings.low_battery_threshold
                onValueChanged: settings.low_battery_threshold = value
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 10

            Label {
                text: qsTr("Icon theme")
                Layout.fillWidth: true
            }

            ComboBox {
                model: [qsTr("System"), qsTr("Dark"), qsTr("Light")]
                currentIndex: settings.theme
                onActivated: {
                    settings.theme = currentIndex
                    mainWindow.updateHeadsetInfo()
                }
            }
        }

        RowLayout {
            spacing: 10

            Label {
                text: qsTr("Language")
                Layout.fillWidth: true
            }

            ComboBox {
                model: [qsTr("System"), "english", "français", "deutsch", "español", "italiano", "magyar"]
                currentIndex: settings.language
                onActivated: {
                    settings.language = currentIndex
                    mainWindow.changeApplicationLanguage(currentIndex)
                    currentIndex = settings.language
                }
            }
        }
    }
}
