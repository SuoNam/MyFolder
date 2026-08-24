pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Labs

// "Send to" sheet: pick the file, confirm destination, go.
// Every listed device shows the channel the transfer would take.
Popup {
    id: root
    modal: true
    anchors.centerIn: parent
    width: 452
    padding: 0

    property string targetDeviceId: ""
    property var pickedFilePaths: []
    signal sendConfirmed(string deviceId, string destinationPath, var filePaths)

    function selectionSummary() {
        if (pickedFilePaths.length === 0) return qsTr("未选择文件")
        if (pickedFilePaths.length === 1) return String(pickedFilePaths[0]).split(/[\\/]/).pop()
        return qsTr("已选择 %1 个文件").arg(pickedFilePaths.length)
    }

    function selfDevice() {
        for (var i = 0; i < DeviceManager.devices.length; ++i)
            if (DeviceManager.devices[i].deviceId === DeviceManager.deviceId) return DeviceManager.devices[i]
        return null
    }

    function canUseLan(device) {
        return device && device.online === true && Number(device.listenPort || 0) > 0
                && DeviceManager.reachableLanAddress(device.localAddresses || []).length > 0
    }

    function openFor(deviceId) {
        targetDeviceId = deviceId
        pickedFilePaths = []
        fileDialog.open()
    }

    function openWithFiles(filePaths, deviceId) {
        pickedFilePaths = filePaths || []
        targetDeviceId = deviceId || ""
        open()
    }

    Labs.FileDialog {
        id: fileDialog
        title: qsTr("选择要发送的一个或多个文件")
        fileMode: Labs.FileDialog.OpenFiles
        onAccepted: {
            var selectedPaths = []
            for (var i = 0; i < files.length; ++i) {
                var selectedPath = ShellIntegration.localFilePath(files[i])
                if (selectedPath.length > 0) selectedPaths.push(selectedPath)
            }
            root.pickedFilePaths = selectedPaths
            if (root.pickedFilePaths.length > 0) root.open()
        }
    }

    background: Rectangle {
        radius: 11
        color: Theme.surface
        border.width: 1; border.color: "#C3D2CB"
    }

    contentItem: ColumnLayout {
        spacing: 0

        // head
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            Layout.bottomMargin: 14
            spacing: 4
            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("发送到"); font.pixelSize: 15; font.bold: true; color: Theme.ink }
                Item { Layout.fillWidth: true }
                Text {
                    Layout.maximumWidth: 240
                    text: root.selectionSummary()
                    font.family: Theme.dataFont; font.pixelSize: 11
                    color: Theme.muted
                    elide: Text.ElideMiddle
                }
            }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

        // device picker
        ListView {
            id: picker
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 232)
            clip: true
            model: DeviceManager.devices.filter(function(d) {
                return d.deviceId !== DeviceManager.deviceId
                       && String(d.deviceType || "").toUpperCase() !== "WEB"
            })

            delegate: Rectangle {
                id: pick
                required property var modelData
                readonly property bool picked: root.targetDeviceId === modelData.deviceId
                readonly property bool online: modelData.online === true
                readonly property bool lanReady: root.canUseLan(modelData)
                width: picker.width
                height: 56
                color: picked ? Theme.signalWash : hoverArea.containsMouse ? Theme.sunken : "transparent"

                MouseArea {
                    id: hoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.targetDeviceId = pick.modelData.deviceId
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 14
                    spacing: 10
                    UiLed { on: pick.online }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: pick.modelData.deviceName; font.pixelSize: 13; font.weight: Font.Medium; color: Theme.ink; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text {
                            text: pick.online ? qsTr("自动选择直连方式，失败后转服务器")
                                               : qsTr("离线 · 自动存入服务器，上线后下载")
                            font.family: Theme.dataFont; font.pixelSize: 10
                            color: Theme.muted; elide: Text.ElideRight; Layout.fillWidth: true
                        }
                    }
                    UiChip { channel: "AUTO"; text: "AUTO" }
                    Text { visible: pick.picked; text: "✓"; font.pixelSize: 13; font.bold: true; color: Theme.signalDeep }
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.lineSoft }
            }
        }

        // foot
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 56
            color: Theme.sunken
            Rectangle { width: parent.width; height: 1; color: Theme.lineSoft }
            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8
                Text { text: qsTr("存到对方"); font.pixelSize: 12; color: Theme.muted }
                UiInput {
                    id: destinationField
                    Layout.fillWidth: true
                    implicitHeight: 32
                    mono: true
                    text: "Downloads"
                    placeholderText: qsTr("目标设备保存目录")
                }
                UiButton {
                    kind: "primary"
                    text: qsTr("发送")
                    enabled: root.targetDeviceId.length > 0
                             && root.pickedFilePaths.length > 0
                             && ForwardManager.validatePath(destinationField.text.trim())
                    onClicked: {
                        root.sendConfirmed(root.targetDeviceId, destinationField.text.trim(), root.pickedFilePaths)
                        root.close()
                    }
                }
            }
        }
    }
}
