import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick
import QtCore

ApplicationWindow {
    id: root
    width: 500
    minimumWidth: 500
    maximumWidth: 500
    height: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins + 60
    minimumHeight: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins + 60
    maximumHeight: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins + 60
    visible: false
    Material.theme: settings.darkMode ? Material.Dark : Material.Light
    Material.accent: Material.Pink
    Material.primary: Material.Indigo
    color: Material.theme === Material.Dark ? "#1c1a1f" : "#e8e3ea"
    title: "HeadsetControl-Qt"

    required property var mainWindow

    header: ToolBar {
        height: 60
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 0

            RowLayout {
                Layout.preferredHeight: 20

                Label {
                    text: root.mainWindow.deviceName
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: root.mainWindow.status
                    font.pixelSize: 20
                }
            }
            ProgressBar {
                id: batteryBar
                from: 0
                to: 100
                value: root.mainWindow.batteryLevel
                Layout.fillWidth: true
                Layout.preferredHeight: 20

                indeterminate: root.mainWindow.status === "Charging"
            }
        }
    }

    onClosing: {
        if (settings.firstRun) {
            mainWindow.sendFirstMinimizeNotification()
            settings.firstRun = false
        }
    }

    Label {
        opacity: root.mainWindow.noDevice ? 1 : 0
        enabled: root.mainWindow.noDevice ? true : false
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
        property bool darkMode: true
    }

    Dialog {
        id: lowBatteryActionsPopup
        parent: mainColumn
        anchors.centerIn: parent
        standardButtons: Dialog.Close

        Shortcut {
            enabled: lowBatteryActionsPopup.visible
            sequence: "Esc"
            onActivated: lowBatteryActionsPopup.visible = false
        }

        ColumnLayout {
            anchors.fill: parent
            id: mainPopupColumn
            spacing: 12

            RowLayout {
                spacing: 20
                Layout.bottomMargin: 10
                Layout.preferredHeight: 40

                Label {
                    text: qsTr("Low battery threshold")
                    Layout.fillWidth: true
                }

                SpinBox {
                    Layout.preferredHeight: 35
                    from: 10
                    to: 30
                    value: settings.low_battery_threshold
                    onValueChanged: settings.low_battery_threshold = value
                }
            }

            RowLayout {
                spacing: 20
                enabled: root.mainWindow.lightsCapable
                Layout.preferredHeight: 40

                Label {
                    text: qsTr("Disable lights")
                    Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -10
                    checked: settings.led_low_battery
                    onCheckedChanged: settings.led_low_battery = checked
                }
            }

            RowLayout {
                spacing: 20
                Layout.preferredHeight: 40

                Label {
                    text: qsTr("Send notification")
                    Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -10
                    checked: settings.notification_low_battery
                    onCheckedChanged: settings.notification_low_battery = checked
                }
            }

            RowLayout {
                spacing: 20
                enabled: root.mainWindow.soundNotifCapable
                Layout.preferredHeight: 40

                Label {
                    text: qsTr("Beep")
                    Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -10
                    checked: settings.sound_low_battery
                    onCheckedChanged: settings.sound_low_battery = checked
                }
            }
        }
    }

    ColumnLayout {
        id: mainColumn
        opacity: root.mainWindow.noDevice ? 0 : 1
        enabled: root.mainWindow.noDevice ? false : true
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Label {
            text: qsTr("Headset settings")
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2b2930" : "#fffbfe"
            Material.elevation: 6
            Material.roundedScale: Material.ExtraSmallScale
            ColumnLayout {
                anchors.fill: parent
                spacing: 15

                RowLayout {
                    spacing: 20
                    enabled: root.mainWindow.sidetoneCapable
                    Layout.preferredHeight: 40

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
                        Layout.rightMargin: -15
                        onPressedChanged: {
                            if (!pressed) {
                                settings.sidetone = value
                                root.mainWindow.setSidetone(Math.round(value))
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 10
                    enabled: root.mainWindow.lightsCapable
                    Layout.preferredHeight: 40

                    Label {
                        text: qsTr("Lights")
                        Layout.fillWidth: true
                    }

                    Switch {
                        Layout.rightMargin: -10
                        checked: settings.led_state
                        onCheckedChanged: {
                            settings.led_state = checked
                            root.mainWindow.toggleLED(checked)
                        }
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.preferredHeight: 40

                    Label {
                        text: qsTr("Actions on low battery")
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Configure")
                        onClicked: lowBatteryActionsPopup.visible = true
                    }
                }
            }
        }

        Label {
            text: qsTr("Application settings")
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2b2930" : "#fffbfe"
            Material.elevation: 6
            Material.roundedScale: Material.ExtraSmallScale
            ColumnLayout {
                anchors.fill: parent
                spacing: 15
                RowLayout {
                    spacing: 10
                    Layout.preferredHeight: 40

                    Label {
                        text: qsTr("Dark mode")
                        Layout.fillWidth: true
                    }

                    Switch {
                        Layout.rightMargin: -10
                        checked: settings.darkMode
                        onClicked: settings.darkMode = checked
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.preferredHeight: 40

                    Label {
                        text: qsTr("Icon theme")
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        Layout.preferredHeight: 35
                        model: [qsTr("System"), qsTr("Dark"), qsTr("Light")]
                        currentIndex: settings.theme
                        onActivated: {
                            settings.theme = currentIndex
                            root.mainWindow.updateHeadsetInfo()
                        }
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.preferredHeight: 40

                    Label {
                        text: qsTr("Language")
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        Layout.preferredHeight: 35
                        model: [qsTr("System"), "english", "français", "deutsch", "español", "italiano", "magyar", "türkçe"]
                        currentIndex: settings.language
                        onActivated: {
                            settings.language = currentIndex
                            root.mainWindow.changeApplicationLanguage(currentIndex)
                            currentIndex = settings.language
                        }
                    }
                }
            }
        }
    }
}
