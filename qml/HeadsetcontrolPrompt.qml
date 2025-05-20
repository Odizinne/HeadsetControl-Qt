import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform
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
        checker.downloadHeadsetControl()
    }

    Connections {
        target: checker

        function onDownloadProgressChanged() {
            downloadProgress = checker.downloadProgress
            if (downloadProgress < 1.0) {
                statusMessage = qsTr("Downloading... ")
            }
        }

        function onExtractionStarted() {
            statusMessage = qsTr("Extracting files...")
            downloadProgress = 1.0
        }

        function onDownloadCompleted() {
            root.close()
        }

        function onDownloadFailed(error) {
            statusMessage = qsTr("Error: ") + error
            isDownloading = false
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
            text: !isDownloading ? qsTr("HeadsetControl is not installed") : qsTr("Installing HeadsetControl...")
            font.pixelSize: 16
            font.bold: true
            Layout.fillWidth: true
            color: !isDownloading ? Material.accent : Material.foreground
        }

        Label {
            text: qsTr("HeadsetControl is required to monitor and control your headset.\nWould you like to download and install it now?")
            wrapMode: Text.WordWrap
            visible: !isDownloading
            Layout.fillWidth: true
        }

        Label {
            text: statusMessage
            visible: statusMessage !== ""
            Layout.fillWidth: true
        }

        ProgressBar {
            visible: isDownloading
            value: downloadProgress
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
                visible: !isDownloading
                enabled: !isDownloading
                onClicked: downloadHeadsetControl()
            }

            Button {
                id: cancelBtn
                text: qsTr("Cancel")
                Layout.preferredWidth: parent.buttonWidth
                visible: !isDownloading
                onClicked: {
                    installCancelled()
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
