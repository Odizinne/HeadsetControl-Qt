import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore

ApplicationWindow {
    id: root
    title: qsTr("HeadsetControl missing")
    width: 450
    minimumWidth: 450
    maximumWidth: 450
    height: mainColumn.implicitHeight + 30
    minimumHeight: mainColumn.implicitHeight + 30
    maximumHeight: mainColumn.implicitHeight + 30
    flags: Qt.Dialog
    visible: true

    required property var checker

    property string latestVersion: "3.0.0"
    property bool isDownloading: false
    property real downloadProgress: 0
    property string statusMessage: ""

    Material.theme: settings.darkMode ? Material.Dark : Material.Light
    Material.accent: Material.Pink
    Material.primary: Material.Indigo
    color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"

    signal installCancelled()

    function downloadHeadsetControl() {
        isDownloading = true
        statusMessage = qsTr("Downloading HeadsetControl...")
        downloadProgress = 0.1
        root.checker.downloadHeadsetControl()
    }

    Connections {
        target: root.checker

        function onDownloadProgressChanged() {
            root.downloadProgress = root.checker.downloadProgress
            if (root.downloadProgress < 1.0) {
                root.statusMessage = qsTr("Downloading... ")
            }
        }

        function onExtractionStarted() {
            root.statusMessage = qsTr("Extracting files...")
            root.downloadProgress = 1.0
        }

        function onDownloadCompleted() {
            root.close()
        }

        function onDownloadFailed(error) {
            root.statusMessage = qsTr("Error: ") + error
            root.isDownloading = false
        }
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

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Label {
            text: !root.isDownloading ? qsTr("HeadsetControl is not installed") : qsTr("Installing HeadsetControl...")
            font.pixelSize: 16
            font.bold: true
            Layout.fillWidth: true
            color: !root.isDownloading ? Material.accent : Material.foreground
        }

        Label {
            text: qsTr("HeadsetControl is required to monitor and control your headset.\nWould you like to download and install it now?")
            wrapMode: Text.WordWrap
            visible: !root.isDownloading
            Layout.fillWidth: true
        }

        Label {
            text: root.statusMessage
            visible: root.statusMessage !== ""
            Layout.fillWidth: true
        }

        ProgressBar {
            visible: root.isDownloading
            value: root.downloadProgress
            from: 0
            to: 1
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 10
            spacing: 10
            property int buttonWidth: Math.max(dlBtn.implicitWidth, cancelBtn.implicitWidth)

            Item { Layout.fillWidth: true }

            Button {
                id: dlBtn
                text: qsTr("Download")
                Layout.preferredWidth: parent.buttonWidth
                visible: !root.isDownloading
                enabled: !root.isDownloading
                onClicked: root.downloadHeadsetControl()
            }

            Button {
                id: cancelBtn
                text: qsTr("Cancel")
                Layout.preferredWidth: parent.buttonWidth
                visible: !root.isDownloading
                onClicked: {
                    root.installCancelled()
                    root.close()
                }
            }
        }
    }

    onClosing: function(close) {
        if (isDownloading) {
            close.accepted = false
        } else {
            installCancelled()
            close.accepted = true
        }
    }
}
