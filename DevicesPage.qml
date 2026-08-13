pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Labs

// Devices page: readout strip, device card grid, send entry.
// Wires the original DevicesList.qml forwarding logic (RELAY upload → create
// forward) to the new card-based layout.
Item {
    id: root
    signal storageSettingsRequested()
    signal serverUploadRequested(string filePath)

    property string selectedDeviceId: ""
    property string statusMessage: ""
    property string pendingActionFile: ""
    property var pendingUploads: ({})
    property string creatingUploadId: ""
    readonly property var visibleDevices: DeviceManager.devices.filter(function(device) {
        return String(device.deviceType || "").toUpperCase() !== "WEB"
    })
    readonly property bool hasRemoteDevice: visibleDevices.some(function(device) {
        return device.deviceId !== DeviceManager.deviceId
    })

    function showStatus(message) {
        statusMessage = message
        statusTimer.restart()
    }

    function sendFileFromShell(filePath) {
        sendDialog.openWithFile(filePath, "")
    }

    Timer {
        id: statusTimer
        interval: 10000
        onTriggered: root.statusMessage = ""
    }

    Labs.FileDialog {
        id: serverFileDialog
        title: qsTr("选择要上传到 MyFolder 服务器的文件")
        onAccepted: {
            var selectedPath = decodeURIComponent(file.toString()
                                                   .replace(/^file:\/\/\//, "")
                                                   .replace(/^file:\/\//, ""))
            root.pendingActionFile = selectedPath
            sendActionPopup.open()
        }
    }

    Popup {
        id: sendActionPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(470, parent ? parent.width - 40 : 470)
        modal: true
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: "#520E211A" }
        background: Rectangle {
            radius: 12
            color: Theme.surface
            border.width: 1
            border.color: "#C3D2CB"
        }
        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                spacing: 10
                Rectangle {
                    width: 34; height: 34; radius: 9; color: Theme.signalWash
                    Text { anchors.centerIn: parent; text: "↑"; font.pixelSize: 18; font.bold: true; color: Theme.signalDeep }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 2
                    Text { text: qsTr("发送文件"); font.pixelSize: 15; font.bold: true; color: Theme.ink }
                    Text {
                        Layout.fillWidth: true
                        text: root.pendingActionFile.split(/[\\/]/).pop()
                        elide: Text.ElideMiddle
                        font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.muted
                    }
                }
                ToolButton {
                    text: "×"; implicitWidth: 30; implicitHeight: 30
                    onClicked: sendActionPopup.close()
                    background: Rectangle { radius: 6; color: parent.hovered ? Theme.sunken : "transparent" }
                }
            }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 16
                spacing: 10
                SendOption {
                    title: qsTr("上传到 MyFolder 服务器")
                    description: qsTr("选择默认位置或服务器上的指定文件夹")
                    symbol: "☁"
                    onChosen: {
                        sendActionPopup.close()
                        root.serverUploadRequested(root.pendingActionFile)
                    }
                }
                SendOption {
                    enabled: root.selectedDeviceId.length > 0
                    title: qsTr("发送到已选设备")
                    description: enabled ? qsTr("通过中转通道发送给当前选中的客户端")
                                         : qsTr("请先选择一台非本机客户端")
                    symbol: "→"
                    onChosen: {
                        sendActionPopup.close()
                        sendDialog.openWithFile(root.pendingActionFile, root.selectedDeviceId)
                    }
                }
            }
        }
    }

    function taskById(uploadId) {
        for (var i = 0; i < TransferManager.taskList.length; ++i)
            if (TransferManager.taskList[i].uploadId === uploadId) return TransferManager.taskList[i]
        return null
    }
    function createForwardWhenReady(uploadId) {
        var intent = pendingUploads[uploadId]
        var task = taskById(uploadId)
        if (!intent || !task || task.state !== "COMPLETED" || creatingUploadId.length > 0) return
        creatingUploadId = uploadId
        ForwardManager.createForward(intent.targetDeviceId, intent.destinationPath, false,
                                     "RELAY", uploadId, task.files)
        showStatus(qsTr("上传完成，正在创建转发任务"))
    }

    Connections {
        target: TransferManager
        function onTaskIdChanged(temporaryId, uploadId) {
            var intent = root.pendingUploads[temporaryId]
            if (!intent) return
            var next = {}
            for (var k in root.pendingUploads) if (k !== temporaryId) next[k] = root.pendingUploads[k]
            next[uploadId] = intent
            root.pendingUploads = next
        }
        function onTaskStatusChanged(uploadId, status, errorMessage) {
            if (status === "COMPLETED") root.createForwardWhenReady(uploadId)
            else if (status === "FAILED" && root.pendingUploads[uploadId])
                root.showStatus(errorMessage || qsTr("RELAY 上传失败"))
        }
    }
    Connections {
        target: ForwardManager
        function onForwardCreated(forwardId) {
            var next = {}
            for (var k in root.pendingUploads) if (k !== root.creatingUploadId) next[k] = root.pendingUploads[k]
            root.pendingUploads = next
            root.creatingUploadId = ""
            root.showStatus(qsTr("已发出，等待对方接收"))
        }
        function onActionFailed(operation, code, message) {
            root.creatingUploadId = ""
            root.showStatus((code ? code + ": " : "") + (message || qsTr("发送失败")))
        }
    }

    SendToDialog {
        id: sendDialog
        onSendConfirmed: function(deviceId, destinationPath, filePath) {
            var temporaryId = TransferManager.startFileUpload(filePath, "relay")
            if (temporaryId.length === 0) {
                root.showStatus(qsTr("无法创建上传任务，请检查文件是否仍然存在"))
                return
            }
            var next = {}
            for (var k in root.pendingUploads) next[k] = root.pendingUploads[k]
            next[temporaryId] = { targetDeviceId: deviceId, destinationPath: destinationPath }
            root.pendingUploads = next
            root.showStatus(qsTr("正在上传，完成后自动通知对方"))
        }
    }

    DeviceDetailsDialog {
        id: detailsDialog
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            ColumnLayout {
                spacing: 3
                Text { text: qsTr("设备"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
                Text {
                    text: root.hasRemoteDevice
                          ? qsTr("登录同一账号的设备会出现在这里。选一台，然后发送文件。")
                          : qsTr("当前只有本机。可上传到服务器；发送到设备需要另一台客户端在线。")
                    font.pixelSize: 12; color: Theme.muted
                }
            }
            Item { Layout.fillWidth: true }
            UiButton {
                text: qsTr("刷新")
                enabled: !DeviceManager.busy
                onClicked: { DeviceManager.refreshDevices(); WebSocketClient.requestDeviceList(); }
            }
            UiButton {
                kind: "primary"
                text: qsTr("发送文件")
                onClicked: serverFileDialog.open()
            }
        }

        // readouts
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 60
            radius: Theme.radius
            color: Theme.surface
            border.width: 1; border.color: Theme.line
            RowLayout {
                anchors.fill: parent
                spacing: 0
                Repeater {
                    model: [
                        { label: "可用客户端", value: root.visibleDevices.length, live: false },
                        { label: "在线", value: root.visibleDevices.filter(function(d){ return d.online }).length, live: true },
                        { label: "转发任务", value: ForwardManager.tasks.length, live: false }
                    ]
                    delegate: Item {
                        id: readout
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Rectangle { visible: readout.index > 0; width: 1; height: parent.height; color: Theme.lineSoft }
                        ColumnLayout {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 18
                            spacing: 4
                            Text {
                                text: readout.modelData.value
                                font.family: Theme.dataFont
                                font.pixelSize: 21; font.weight: Font.Medium
                                color: readout.modelData.live ? Theme.signalDeep : Theme.ink
                            }
                            Text {
                                text: readout.modelData.label
                                font.family: Theme.dataFont
                                font.pixelSize: 10; font.letterSpacing: 1.2
                                color: Theme.faint
                            }
                        }
                    }
                }
            }
        }

        // card grid
        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 280)))
            cellHeight: 252
            model: root.visibleDevices

            delegate: Item {
                id: cell
                required property var modelData
                width: grid.cellWidth; height: grid.cellHeight
                DeviceCard {
                    anchors.fill: parent
                    anchors.rightMargin: 11; anchors.bottomMargin: 11
                    device: cell.modelData
                    isSelf: cell.modelData.deviceId === DeviceManager.deviceId
                    selected: root.selectedDeviceId === cell.modelData.deviceId
                    onClicked: if (!isSelf) root.selectedDeviceId = cell.modelData.deviceId
                    onSendRequested: sendDialog.openFor(cell.modelData.deviceId)
                    onDetailRequested: {
                        if (cell.modelData.deviceId === DeviceManager.deviceId)
                            root.storageSettingsRequested()
                        else
                            detailsDialog.openFor(cell.modelData)
                    }
                }
            }

            // empty state
            Item {
                anchors.fill: parent
                visible: grid.count === 0
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("还没有其他设备"); font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink }
                    Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("在另一台电脑登录同一账号，它就会出现在这里。"); font.pixelSize: 12; color: Theme.muted }
                }
            }
        }

        // status line
        Text {
            visible: root.statusMessage.length > 0
            Layout.fillWidth: true
            text: root.statusMessage
            font.pixelSize: 11
            color: "#8B651F"
            elide: Text.ElideRight
        }
    }

    component SendOption: Rectangle {
        id: option
        property string title: ""
        property string description: ""
        property string symbol: ""
        signal chosen()
        Layout.fillWidth: true
        implicitHeight: 68
        radius: Theme.radius
        color: option.enabled && optionMouse.containsMouse ? Theme.signalWash : Theme.sunken
        border.width: 1
        border.color: option.enabled && optionMouse.containsMouse ? Theme.signalEdge : Theme.line
        opacity: option.enabled ? 1 : 0.55
        MouseArea {
            id: optionMouse
            anchors.fill: parent
            enabled: option.enabled
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: option.chosen()
        }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 13; anchors.rightMargin: 14; spacing: 11
            Rectangle {
                width: 36; height: 36; radius: 9; color: Theme.surface; border.width: 1; border.color: Theme.line
                Text { anchors.centerIn: parent; text: option.symbol; color: Theme.signalDeep; font.pixelSize: 17; font.bold: true }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                Text { text: option.title; color: Theme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                Text { Layout.fillWidth: true; text: option.description; color: Theme.muted; font.pixelSize: 11; wrapMode: Text.WordWrap }
            }
            Text { text: "›"; font.pixelSize: 22; color: Theme.faint }
        }
    }
}
