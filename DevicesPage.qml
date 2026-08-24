pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Qt.labs.platform as Labs

// Devices page: readout strip, device card grid, send entry.
// Wires the original DevicesList.qml forwarding logic (RELAY upload → create
// forward) to the new card-based layout.
Item {
    id: root

    function refreshContent() {
        DeviceManager.refreshDevices()
        WebSocketClient.requestDeviceList()
        ForwardManager.refreshTasks()
    }
    signal storageSettingsRequested()
    signal serverUploadRequested(var filePaths)

    property string selectedDeviceId: ""
    property string statusMessage: ""
    property string statusTone: "normal"
    property var pendingActionFiles: []
    property var pendingUploads: ({})
    property string creatingUploadId: ""
    property int relayForwardRetryCount: 0
    readonly property var visibleDevices: DeviceManager.devices.filter(function(device) {
        return String(device.deviceType || "").toUpperCase() !== "WEB"
    })
    readonly property bool hasRemoteDevice: visibleDevices.some(function(device) {
        return device.deviceId !== DeviceManager.deviceId
    })

    function showStatus(message, tone) {
        statusMessage = message
        statusTone = tone || "normal"
    }

    function sendFilesFromShell(filePaths) {
        sendDialog.openWithFiles(filePaths, "")
    }

    function selectionSummary(filePaths) {
        if (!filePaths || filePaths.length === 0) return qsTr("未选择文件")
        if (filePaths.length === 1) return String(filePaths[0]).split(/[\\/]/).pop()
        return qsTr("已选择 %1 个文件").arg(filePaths.length)
    }

    function deviceById(deviceId) {
        for (var i = 0; i < DeviceManager.devices.length; ++i)
            if (DeviceManager.devices[i].deviceId === deviceId) return DeviceManager.devices[i]
        return null
    }

    function lanRoute(target) {
        if (!target) return ({})
        var addresses = target.localAddresses || []
        // Older clients only publish deviceAddress. Keep LAN eligible when
        // the server has that legacy field but no CIDR list yet.
        if (addresses.length === 0 && String(target.deviceAddress || "").length > 0)
            addresses = [target.deviceAddress]
        return DeviceManager.reachableLanRoute(addresses)
    }

    function lanAddress(target) {
        return String(lanRoute(target).targetAddress || "")
    }

    function canUseLan(target) {
        return target && target.online === true && Number(target.listenPort || 0) > 0
                && lanAddress(target).length > 0
    }

    function enqueueRelay(targetDeviceId, destinationPath, filePath, directory) {
        var temporaryId = directory ? TransferManager.startFolderUpload(filePath, "relay")
                                    : TransferManager.startFileUpload(filePath, "relay")
        if (temporaryId.length === 0) {
            showStatus(qsTr("无法创建上传任务，请检查文件是否仍然存在"), "error")
            return
        }
        var next = {}
        for (var k in pendingUploads) next[k] = pendingUploads[k]
        next[temporaryId] = { targetDeviceId: targetDeviceId, destinationPath: destinationPath }
        setPendingUploads(next)
        showStatus(qsTr("正在上传至中转服务器，完成后自动通知对方"))
    }

    function setPendingUploads(next) {
        pendingUploads = next
        relayIntentSettings.pendingUploadsJson = JSON.stringify(next)
    }

    function removePendingUpload(uploadId) {
        var next = {}
        for (var k in pendingUploads) if (k !== uploadId) next[k] = pendingUploads[k]
        setPendingUploads(next)
    }

    function fallbackToRelay(targetDeviceId, destinationPath, filePath, directory, reason) {
        showStatus(qsTr("直连失败，正在自动切换服务器中转：") + reason, "warning")
        enqueueRelay(targetDeviceId, destinationPath, filePath, directory)
    }

    function fallbackFromLan(targetDeviceId, destinationPath, filePath, directory, reason) {
        var target = deviceById(targetDeviceId)
        if (target && target.online === true && P2pTransferManager.available) {
            showStatus(qsTr("LAN 失败，正在尝试 P2P：") + reason, "warning")
            P2pTransferManager.sendPath(targetDeviceId, destinationPath, filePath)
        } else {
            fallbackToRelay(targetDeviceId, destinationPath, filePath, directory, reason)
        }
    }

    Settings {
        id: relayIntentSettings
        category: "ForwardV11"
        property string pendingUploadsJson: "{}"
    }

    Timer {
        id: relayForwardRetryTimer
        interval: Math.min(30000, 2000 * Math.pow(2, root.relayForwardRetryCount))
        onTriggered: {
            root.creatingUploadId = ""
            root.resumePendingRelays()
        }
    }

    Labs.FileDialog {
        id: serverFileDialog
        title: qsTr("选择要发送的一个或多个文件")
        fileMode: Labs.FileDialog.OpenFiles
        onAccepted: {
            var selectedPaths = []
            for (var i = 0; i < files.length; ++i) {
                var selectedPath = ShellIntegration.localFilePath(files[i])
                if (selectedPath.length > 0) selectedPaths.push(selectedPath)
            }
            if (selectedPaths.length === 0) {
                root.showStatus(qsTr("无法读取所选文件路径"), "error")
                return
            }
            root.pendingActionFiles = selectedPaths
            sendActionPopup.open()
        }
    }

    Labs.FolderDialog {
        id: serverFolderDialog
        title: qsTr("选择要发送的文件夹")
        onAccepted: {
            var selectedFolder = ShellIntegration.localFilePath(folder)
            if (selectedFolder.length === 0) {
                root.showStatus(qsTr("无法读取所选文件夹路径"), "error")
                return
            }
            root.pendingActionFiles = [selectedFolder]
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
                        text: root.selectionSummary(root.pendingActionFiles)
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
                        root.serverUploadRequested(root.pendingActionFiles)
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
                        sendDialog.openWithFiles(root.pendingActionFiles, root.selectedDeviceId)
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
    function existingForward(uploadId, intent) {
        for (var i = 0; i < ForwardManager.tasks.length; ++i) {
            var task = ForwardManager.tasks[i]
            if (task.relayUploadId === uploadId && task.channel === "RELAY"
                    && task.sourceDeviceId === DeviceManager.deviceId
                    && task.targetDeviceId === intent.targetDeviceId
                    && task.destinationPath === intent.destinationPath
                    && task.state !== "FAILED" && task.state !== "CANCELLED") return task
        }
        return null
    }
    function createForwardWhenReady(uploadId) {
        var intent = pendingUploads[uploadId]
        var task = taskById(uploadId)
        if (!intent || !task || task.state !== "COMPLETED" || creatingUploadId.length > 0) return
        if (existingForward(uploadId, intent)) {
            removePendingUpload(uploadId)
            return
        }
        creatingUploadId = uploadId
        ForwardManager.createForward(intent.targetDeviceId, intent.destinationPath, false,
                                     "RELAY", uploadId, task.files)
        showStatus(qsTr("上传完成，正在创建转发任务"))
    }
    function resumePendingRelays() {
        if (creatingUploadId.length > 0) return
        for (var uploadId in pendingUploads) {
            var task = taskById(uploadId)
            if (task && task.state === "COMPLETED") {
                createForwardWhenReady(uploadId)
                return
            }
        }
    }

    Component.onCompleted: {
        try {
            var restored = JSON.parse(relayIntentSettings.pendingUploadsJson || "{}")
            pendingUploads = restored && typeof restored === "object" ? restored : ({})
        } catch (error) {
            setPendingUploads({})
        }
        relayResumeTimer.start()
    }

    Timer {
        id: relayResumeTimer
        interval: 1500
        onTriggered: {
            ForwardManager.refreshTasks()
        }
    }

    Connections {
        target: TransferManager
        function onTaskIdChanged(temporaryId, uploadId) {
            var intent = root.pendingUploads[temporaryId]
            if (!intent) return
            var next = {}
            for (var k in root.pendingUploads) if (k !== temporaryId) next[k] = root.pendingUploads[k]
            next[uploadId] = intent
            root.setPendingUploads(next)
        }
        function onTaskStatusChanged(uploadId, status, errorMessage) {
            if (status === "COMPLETED") root.createForwardWhenReady(uploadId)
            else if (status === "FAILED" && root.pendingUploads[uploadId])
                root.showStatus(errorMessage || qsTr("RELAY 上传失败"), "error")
        }
    }
    Connections {
        target: ForwardManager
        function onForwardCreated(forwardId) {
            root.removePendingUpload(root.creatingUploadId)
            root.creatingUploadId = ""
            root.relayForwardRetryCount = 0
            root.showStatus(qsTr("已发出，等待对方接收"))
            Qt.callLater(root.resumePendingRelays)
        }
        function onActionFailed(operation, code, message) {
            if (operation !== "create" || root.creatingUploadId.length === 0) return
            root.creatingUploadId = ""
            root.relayForwardRetryCount++
            root.showStatus((code ? code + ": " : "")
                            + (message || qsTr("创建中转任务失败")) + qsTr("，正在自动重试"), "warning")
            relayForwardRetryTimer.restart()
        }
        function onTasksChanged() { Qt.callLater(root.resumePendingRelays) }
    }

    Connections {
        target: LanTransferManager
        function onForwardCreated(forwardId) {
            ForwardManager.queryTask(forwardId)
            root.showStatus(qsTr("LAN 任务已发出，等待对方接收"))
        }
        function onCreateFailed(message) {
            root.showStatus(qsTr("LAN 任务创建失败：") + message, "warning")
        }
    }
    Connections {
        target: P2pTransferManager
        function onForwardCreated(forwardId) {
            ForwardManager.queryTask(forwardId)
            root.showStatus(qsTr("P2P 任务已发出，等待对方接收"))
        }
        function onCreateFailed(message) {
            root.showStatus(qsTr("P2P 任务创建失败，正在改用服务器中转：") + message, "warning")
        }
    }

    SendToDialog {
        id: sendDialog
        onSendConfirmed: function(deviceId, destinationPath, filePaths) {
            var target = root.deviceById(deviceId)
            for (var i = 0; i < filePaths.length; ++i) {
                var filePath = String(filePaths[i] || "")
                if (root.canUseLan(target)) {
                    var route = root.lanRoute(target)
                    LanTransferManager.sendPathViaRoute(deviceId, String(route.targetAddress || ""),
                                                        String(route.sourceAddress || ""),
                                                        Number(target.listenPort), destinationPath, filePath)
                } else if (target && target.online === true && P2pTransferManager.available) {
                    P2pTransferManager.sendPath(deviceId, destinationPath, filePath)
                } else {
                    root.enqueueRelay(deviceId, destinationPath, filePath,
                                      LanTransferManager.isDirectory(filePath))
                }
            }
            root.showStatus(filePaths.length > 1
                            ? qsTr("正在创建 %1 个文件传输任务").arg(filePaths.length)
                            : qsTr("正在计算文件校验值并创建传输任务"))
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
            UiButton {
                kind: "primary"
                text: qsTr("发送文件夹")
                onClicked: serverFolderDialog.open()
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
                        { label: "进行中传输", value: ForwardManager.tasks.filter(function(t) {
                            return t.state !== "COMPLETED" && t.state !== "CANCELLED"
                                    && t.state !== "FAILED" && t.state !== "REJECTED"
                        }).length, live: false }
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

        UiNotice {
            Layout.fillWidth: true
            message: root.statusMessage
            tone: root.statusTone
            onDismissed: root.statusMessage = ""
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
