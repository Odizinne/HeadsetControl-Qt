import QtQuick.Controls.Material
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick
import QtCore

ApplicationWindow {
    id: root
    width: 500
    minimumWidth: 500
    maximumWidth: 500
    height: mainColumn.implicitHeight + (2 * mainColumn.anchors.margins) + 60
    minimumHeight: mainColumn.implicitHeight + (2 * mainColumn.anchors.margins) + 60
    maximumHeight: mainColumn.implicitHeight + (2 * mainColumn.anchors.margins) + 60
    visible: false
    Material.theme: settings.darkMode ? Material.Dark : Material.Light
    Material.accent: Material.Pink
    Material.primary: Material.Indigo
    color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"
    title: "HeadsetControl-Qt"

    required property var mainWindow

    header: ToolBar {
        visible: !root.mainWindow.noDevice
        height: 60

        Item {
            height: childrenRect.height
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
                leftMargin: 20
                rightMargin: 20
            }

            RowLayout {
                id: headsetLyt
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }

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
                indeterminate: root.mainWindow.status === "Charging"
                anchors {
                    top: headsetLyt.bottom
                    topMargin: 5
                    left: parent.left
                    right: parent.right
                }
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
        property bool enableChatmix: true
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
            spacing: 9

            RowLayout {
                spacing: 20
                Layout.preferredHeight: 35

                Label {
                    text: qsTr("Low battery threshold")
                    Layout.fillWidth: true
                }

                SpinBox {
                    Layout.preferredHeight: 30
                    from: 5
                    to: 50
                    value: settings.low_battery_threshold
                    onValueChanged: settings.low_battery_threshold = value
                }
            }

            RowLayout {
                spacing: 20
                enabled: root.mainWindow.lightsCapable
                Layout.preferredHeight: 35

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
                Layout.preferredHeight: 35

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
                Layout.preferredHeight: 35

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
            Layout.topMargin: 5
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2B2B2B" : "#FFFFFF"
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
                        Layout.fillWidth: true
                        onPressedChanged: {
                            if (!pressed) {
                                settings.sidetone = value
                                root.mainWindow.setSidetone(Math.round(value))
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 20
                    visible: root.mainWindow.chatmixCapable
                    Layout.preferredHeight: 40

                    Label {
                        id: chatmixLabel
                        text: qsTr("Enable chatmix")
                    }

                    IconImage {
                        source: "qrc:/icons/game.png"
                        sourceSize.width: 20
                        sourceSize.height: 20
                        color: Material.foreground
                    }

                    Slider {
                        from: 0
                        to: 128
                        value: root.mainWindow.chatmix
                        Layout.fillWidth: true
                        enabled: false
                        Layout.leftMargin: -20
                        Layout.rightMargin: -20
                    }

                    IconImage {
                        source: "qrc:/icons/voice.png"
                        sourceSize.width: 20
                        sourceSize.height: 20
                        color: Material.foreground
                    }

                    Switch {
                        checked: settings.enableChatmix
                        onClicked: settings.enableChatmix = checked
                        Layout.rightMargin: -10
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
                        topInset: 0
                        bottomInset: 0
                        onClicked: lowBatteryActionsPopup.visible = true
                        Layout.preferredHeight: 30
                    }
                }
            }
        }

        Label {
            text: qsTr("Application settings")
            Layout.topMargin: 5
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2B2B2B" : "#FFFFFF"
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
                            settings.setValue("theme", currentIndex)
                            Qt.callLater(function() {
                                root.mainWindow.updateHeadsetInfo()
                            })
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

                RowLayout {
                    spacing: 10
                    Layout.preferredHeight: 40

                    Label {
                        text: qsTr("Run at startup")
                        Layout.fillWidth: true
                    }

                    CheckBox {
                        Layout.rightMargin: -5
                        checked: root.mainWindow.isRunAtStartup
                        onClicked: root.mainWindow.setRunAtStartup(checked)
                    }
                }
            }
        }
    }
}
