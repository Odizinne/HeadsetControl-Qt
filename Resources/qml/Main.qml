import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import QtCore

ApplicationWindow {
    id: root
    width: 500
    minimumWidth: 500
    maximumWidth: 500
    height: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins
    minimumHeight: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins
    maximumHeight: mainColumn.implicitHeight + 2 * mainColumn.anchors.margins
    visible: false
    title: "HeadsetControl-Qt"

    readonly property real maxFrameHeight: Math.max(
                                               sidetoneFrame.implicitHeight,
                                               lightsFrame.implicitHeight,
                                               lowBatteryFrame.implicitHeight,
                                               themeFrame.implicitHeight,
                                               languageFrame.implicitHeight
                                               )

    required property var mainWindow

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
            anchors.fill: parent
            id: mainPopupColumn
            spacing: 12

            RowLayout {
                spacing: 20
                Layout.bottomMargin: 10

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

            RowLayout {
                spacing: 20
                enabled: root.mainWindow.lightsCapable

                Label {
                    text: qsTr("Disable lights")
                    Layout.fillWidth: true
                }

                Switch {
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
                    checked: settings.notification_low_battery
                    onCheckedChanged: settings.notification_low_battery = checked
                }
            }

            RowLayout {
                spacing: 20
                enabled: root.mainWindow.soundNotifCapable

                Label {
                    text: qsTr("Beep")
                    Layout.fillWidth: true
                }

                Switch {
                    checked: settings.sound_low_battery
                    onCheckedChanged: settings.sound_low_battery = checked
                }
            }

            Button {
                Layout.topMargin: 10
                text: qsTr("Close")
                onClicked: lowBatteryActionsPopup.visible = false
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    ColumnLayout {
        id: mainColumn
        opacity: root.mainWindow.noDevice ? 0 : 1
        enabled: root.mainWindow.noDevice ? false : true
        anchors.fill: parent
        anchors.margins: 15
        spacing: 5

        RowLayout {
            Layout.bottomMargin: 10
            spacing: 10

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

        RowLayout {
            spacing: 20
            Layout.bottomMargin: 10

            ProgressBar {
                id: batteryBar
                from: 0
                to: 100
                value: root.mainWindow.batteryLevel
                Layout.fillWidth: true
                indeterminate: root.mainWindow.status === "Charging"
            }
        }

        Frame {
            id: sidetoneFrame
            Layout.fillWidth: true
            Layout.preferredHeight: root.maxFrameHeight

            RowLayout {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                spacing: 20
                enabled: root.mainWindow.sidetoneCapable

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
        }

        Frame {
            id: lightsFrame
            Layout.fillWidth: true
            Layout.preferredHeight: root.maxFrameHeight

            RowLayout {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                enabled: root.mainWindow.lightsCapable

                Label {
                    text: qsTr("Lights")
                    Layout.fillWidth: true
                }

                Switch {
                    Layout.rightMargin: -8
                    checked: settings.led_state
                    onCheckedChanged: {
                        settings.led_state = checked
                        root.mainWindow.toggleLED(checked)
                    }
                }
            }
        }

        Frame {
            id: lowBatteryFrame
            Layout.fillWidth: true
            Layout.preferredHeight: root.maxFrameHeight

            RowLayout {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
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
        }

        Frame {
            id: themeFrame
            Layout.fillWidth: true
            Layout.preferredHeight: root.maxFrameHeight

            RowLayout {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
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
                        root.mainWindow.updateHeadsetInfo()
                    }
                }
            }
        }

        Frame {
            id: languageFrame
            Layout.fillWidth: true
            Layout.preferredHeight: root.maxFrameHeight

            RowLayout {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                Label {
                    text: qsTr("Language")
                    Layout.fillWidth: true
                }

                ComboBox {
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
