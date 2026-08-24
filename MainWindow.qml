pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

// Workspace window: compact rail on the left, pages on the right.
ApplicationWindow {
    id: root
    signal logoutRequested()
    width: 1060
    height: 690
    minimumWidth: 860
    minimumHeight: 560
    visible: false
    title: qsTr("MyFolder")
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"

    property int activePage: 0   // 0 files, 1 devices, 2 transfers, 3 groups, 4 history, 5 settings
    property var incomingOffer: null

    function refreshVisibleData() {
        if (!GlobalStatus.authToken.length) return
        switch (root.activePage) {
        case 0: serverFilesPage.refreshContent(); break
        case 1: devicesPage.refreshContent(); break
        case 2: transfersPage.refreshContent(); break
        case 3: groupsPage.refreshContent(); break
        case 4: historyPage.refreshContent(); break
        case 5: settingsPage.refreshContent(); break
        }
    }

    function showTransferCompleted(forwardId, totalBytes) {
        var task = null
        for (var i = 0; i < ForwardManager.tasks.length; ++i) {
            if (String(ForwardManager.tasks[i].forwardId || "") === String(forwardId || "")) {
                task = ForwardManager.tasks[i]
                break
            }
        }
        var source = task ? String(task.sourceDeviceId || "") : ""
        for (var d = 0; d < DeviceManager.devices.length; ++d) {
            if (String(DeviceManager.devices[d].deviceId || "") === source) {
                source = String(DeviceManager.devices[d].deviceName || source)
                break
            }
        }
        var files = task && task.files ? task.files : []
        var filePath = files.length === 1 ? String(files[0].path || "")
                                           : qsTr("%1 个文件").arg(files.length)
        var destination = RelayDownloadManager.receiveRoot
        var relative = task ? String(task.destinationPath || "") : ""
        if (relative.length > 0) destination += "/" + relative
        if (files.length === 1 && filePath.length > 0) destination += "/" + filePath
        completionToast.showCompleted(forwardId, totalBytes, source, filePath, destination)
    }

    onActivePageChanged: Qt.callLater(root.refreshVisibleData)

    function acceptIncomingTask(task, automatic) {
        if (!task || !task.forwardId) return
        if (automatic)
            ForwardManager.markAutoAccepting(task.forwardId)
        ForwardManager.rememberAcceptedTransfer(task)
        if (task.channel === "LAN")
            LanTransferManager.prepareIncoming(task)
        else if (task.channel === "P2P")
            P2pTransferManager.prepareIncoming(task)
        ForwardManager.accept(task.forwardId)
    }

    function canAutoAcceptFallback(task) {
        return ForwardManager.canAutoAcceptFallback(task)
    }

    function forgetAcceptedTransfer(task) {
        ForwardManager.forgetAcceptedTransfer(task)
    }

    function incomingOfferTitle(task) {
        if (!task || !task.files || task.files.length === 0) return qsTr("新文件")
        if (task.files.length === 1)
            return String(task.files[0].path || qsTr("新文件")).split(/[\\/]/).pop()
        return task.files.length + qsTr(" 个文件")
    }

    function connectionLabel() {
        switch (WebSocketClient.connectionState) {
        case "CONNECTED": return qsTr("实时连接正常\n") + DeviceManager.deviceName
        case "CONNECTING": return qsTr("正在连接实时服务…")
        case "RECONNECTING": return qsTr("实时服务重连中…")
        case "FAILED": return qsTr("实时连接失败\n点击重试")
        default: return qsTr("实时服务未连接")
        }
    }

    function handleStartupAction(action, payload) {
        var filePaths = []
        if (payload !== undefined && payload !== null) {
            if (typeof payload === "string") {
                var text = String(payload).trim()
                if (text.charAt(0) === "[") {
                    try { filePaths = JSON.parse(text) }
                    catch (error) { filePaths = [text] }
                } else if (text.length > 0) filePaths = [text]
            } else if (payload.length !== undefined) {
                for (var index = 0; index < Number(payload.length); ++index)
                    filePaths.push(String(payload[index] || ""))
            }
        }
        filePaths = filePaths.filter(function(path) {
            return String(path).trim().length > 0
        })
        if (filePaths.length === 0) return
        root.show(); root.raise(); root.requestActivate()
        if (action === "upload-server") {
            serverUploadDialog.openForPaths(filePaths)
        } else if (action === "send-client") {
            activePage = 1
            Qt.callLater(function() { devicesPage.sendFilesFromShell(filePaths) })
        }
    }

    Platform.SystemTrayIcon {
        visible: true
        icon.source: "qrc:/qt/qml/MyTest/Icons/app.png"
        tooltip: qsTr("MyFolder")
        onActivated: function(reason) {
            if (reason === Platform.SystemTrayIcon.Trigger ||
                    reason === Platform.SystemTrayIcon.DoubleClick) {
                root.show()
                root.raise()
                root.requestActivate()
            }
        }
        menu: Platform.Menu {
            Platform.MenuItem {
                text: qsTr("打开 MyFolder")
                onTriggered: {
                    root.show()
                    root.raise()
                    root.requestActivate()
                }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: qsTr("退出")
                onTriggered: Qt.quit()
            }
        }
    }

    function refreshRealtimeSession() {
        if (GlobalStatus.authToken.length > 0)
            DeviceManager.restartSession()
    }

    function prepareForNetworkRecovery() {
        WebSocketClient.disconnectFromServer()
        DeviceManager.stop(false)
    }

    function activateDeviceSession(deviceId, deviceToken) {
        ForwardManager.currentDeviceId = deviceId
        ForwardManager.currentDeviceToken = deviceToken
        RelayDownloadManager.currentDeviceId = deviceId
        RelayDownloadManager.currentDeviceToken = deviceToken
        LanTransferManager.currentDeviceId = deviceId
        LanTransferManager.currentDeviceToken = deviceToken
        LanTransferManager.receiveRoot = RelayDownloadManager.receiveRoot
        P2pTransferManager.currentDeviceId = deviceId
        P2pTransferManager.currentDeviceToken = deviceToken
        P2pTransferManager.receiveRoot = RelayDownloadManager.receiveRoot
        WebSocketClient.connectTo(Config.baseUrl, GlobalStatus.authToken,
                                  deviceId, deviceToken)
        DeviceManager.refreshDevices()
        ForwardManager.refreshTasks()
        root.refreshVisibleData()
    }

    Component.onCompleted: {
        HttpHandler.loadAccountProfile()
        if (DeviceManager.registered && DeviceManager.deviceToken.length > 0)
            activateDeviceSession(DeviceManager.deviceId, DeviceManager.deviceToken)
    }

    // A logout removes the device server-side. When the same process logs in
    // again, repair both registration and the realtime channel automatically.
    Timer {
        interval: 5000
        repeat: true
        running: true
        triggeredOnStart: true
        onTriggered: {
            if (!DeviceManager.registered) {
                if (!DeviceManager.busy && GlobalStatus.authToken.length > 0)
                    DeviceManager.start()
                return
            }
            if (WebSocketClient.connectionState === "DISCONNECTED" ||
                    WebSocketClient.connectionState === "FAILED")
                WebSocketClient.reconnect()
        }
    }

    Connections {
        target: HttpHandler
        function onAccountProfileResult(response) {
            if (response.status === 200 && response.data)
                GlobalStatus.autoAcceptDeviceTransfers = response.data.autoAcceptDeviceTransfers === true
        }
        function onTransferPreferencesUpdateResult(response) {
            if (response.status === 200 && response.data)
                GlobalStatus.autoAcceptDeviceTransfers = response.data.autoAcceptDeviceTransfers === true
        }
    }

    Connections {
        target: DeviceManager
        function onCredentialsReady(deviceId, deviceToken) {
            root.activateDeviceSession(deviceId, deviceToken)
        }
    }

    Connections {
        target: WebSocketClient
        function onDeviceListReceived(devices) { DeviceManager.applyWebSocketDevices(devices) }
        function onDeviceCredentialsRejected() {
            DeviceManager.stop(false)
            if (!DeviceManager.busy && GlobalStatus.authToken.length > 0)
                DeviceManager.start()
        }
        function onForwardEventReceived(action, forwardId, payload) {
            if (action === "task.forward.signal") {
                P2pTransferManager.handleSignal(payload)
                return
            }
            ForwardManager.handleForwardEvent(action, payload)
        }
    }

    Connections {
        target: ForwardManager
        function onTaskUpdated(task) {
            if (task.channel === "LAN") {
                if (task.targetDeviceId === DeviceManager.deviceId
                        && (task.state === "ACCEPTED" || task.state === "TRANSFERRING"))
                    LanTransferManager.prepareIncoming(task)
                LanTransferManager.handleTaskUpdated(task)
            }
            if (task.channel === "P2P") {
                if (task.targetDeviceId === DeviceManager.deviceId
                        && (task.state === "ACCEPTED" || task.state === "TRANSFERRING"))
                    P2pTransferManager.prepareIncoming(task)
                P2pTransferManager.handleTaskUpdated(task)
            }
            if (task.targetDeviceId !== DeviceManager.deviceId) return
            if (task.state === "OFFERED") {
                var automatic = root.canAutoAcceptFallback(task)
                             || GlobalStatus.autoAcceptDeviceTransfers
                if (automatic) {
                    if (!ForwardManager.isAutoAccepting(task.forwardId))
                        root.acceptIncomingTask(task, true)
                } else {
                    root.incomingOffer = task
                }
            }
            else if (task.channel === "RELAY" && task.state === "TRANSFERRING") {
                if (!RelayDownloadManager.startTask(task))
                    ForwardManager.fail(task.forwardId,
                                        RelayDownloadManager.lastError || qsTr("服务器中转任务无法开始"))
            }
            else if (task.state === "COMPLETED") {
                root.forgetAcceptedTransfer(task)
                if (task.channel === "RELAY")
                    RelayDownloadManager.confirmTaskCompleted(task.forwardId)
            }
            else if (task.state === "REJECTED") {
                root.forgetAcceptedTransfer(task)
                if (root.incomingOffer && root.incomingOffer.forwardId === task.forwardId)
                    root.incomingOffer = null
            }
            else if (task.channel === "RELAY" && task.state === "FAILED")
                root.forgetAcceptedTransfer(task)
        }
        function onFinalizationFailed(forwardId, code) {
            RelayDownloadManager.handleFinalizationFailure(forwardId, code)
        }
        function onTasksReconciled(activeTaskIds) {
            LanTransferManager.reconcileTasks(activeTaskIds)
            P2pTransferManager.reconcileTasks(activeTaskIds)
        }
    }

    Connections {
        target: TransferManager
        function onTaskStatusChanged(uploadId, status, errorMessage) {
            if (status !== "COMPLETED") return
            var tasks = TransferManager.taskList
            for (var i = 0; i < tasks.length; ++i) {
                if (String(tasks[i].uploadId || "") !== String(uploadId || "")) continue
                if (String(tasks[i].scopeType || "").toUpperCase() === "GROUP")
                    groupsPage.refreshGroupFiles(tasks[i].scopeId)
                else
                    serverFilesPage.refreshContent()
                break
            }
            if (root.activePage === 5) settingsPage.refreshContent()
        }
    }

    ServerUploadDialog {
        id: serverUploadDialog
        onUploadConfirmed: function(filePaths, serverPath, scopeType, scopeId) {
            var created = 0
            var expected = 0
            var selectedFiles = []
            for (var i = 0; i < filePaths.length; ++i) {
                var filePath = String(filePaths[i] || "")
                if (LanTransferManager.isDirectory(filePath)) {
                    expected++
                    var folderTaskId = TransferManager.startScopedFolderUpload(
                                filePath, serverPath, scopeType, scopeId)
                    if (folderTaskId && folderTaskId.length > 0) created++
                } else selectedFiles.push(filePath)
            }
            if (selectedFiles.length > 0) {
                expected++
                var fileTaskId = TransferManager.startScopedFilesUpload(
                            selectedFiles, serverPath, scopeType, scopeId)
                if (fileTaskId && fileTaskId.length > 0) created++
            }
            root.activePage = 2
            root.show(); root.raise(); root.requestActivate()
            if (created !== expected)
                console.warn("Unable to create all upload tasks", created, expected)
        }
    }

    IncomingTransferToast {
        offer: root.incomingOffer
        onLaterRequested: root.incomingOffer = null
        onAcceptRequested: {
            if (root.incomingOffer) root.acceptIncomingTask(root.incomingOffer, false)
            root.incomingOffer = null
        }
        onRejectRequested: {
            if (root.incomingOffer) ForwardManager.reject(root.incomingOffer.forwardId)
            root.incomingOffer = null
        }
    }

    TransferCompletionToast { id: completionToast }

    Connections {
        target: RelayDownloadManager
        function onProgressReady(forwardId, verifiedBytes) {
            ForwardManager.reportProgress(forwardId, verifiedBytes)
        }
        function onTaskDownloadCompleted(forwardId, totalBytes) {
            root.showTransferCompleted(forwardId, totalBytes)
            ForwardManager.finishDownload(forwardId, totalBytes)
        }
        function onTaskDownloadFailed(forwardId, code, message) {
            ForwardManager.fail(forwardId, code + ": " + message)
        }
    }

    Connections {
        target: LanTransferManager
        function onIncomingProgress(forwardId, verifiedBytes) {
            ForwardManager.reportProgress(forwardId, verifiedBytes)
        }
        function onIncomingCompleted(forwardId, totalBytes) {
            root.showTransferCompleted(forwardId, totalBytes)
            ForwardManager.finishDownload(forwardId, totalBytes)
        }
        function onIncomingFailed(forwardId, code, message) {
            ForwardManager.fail(forwardId, code + ": " + message)
        }
        function onOutgoingFallbackRequested(forwardId, targetDeviceId, destinationPath, localFilePath, directory, reason) {
            if (forwardId && forwardId.length > 0)
                ForwardManager.hideSupersededTask(forwardId)
            if (forwardId && forwardId.length > 0)
                ForwardManager.fail(forwardId, "LAN_FALLBACK: " + reason)
            devicesPage.fallbackFromLan(targetDeviceId, destinationPath, localFilePath, directory, reason)
        }
    }

    // WebSocket events are deliberately low-latency, but they are not a
    // durable task store. Periodically reconcile both lists so a sleep,
    // network roam, VPN/TUN restart, or missed event cannot leave stale rows.
    Timer {
        interval: 60000
        repeat: true
        running: true
        onTriggered: {
            if (GlobalStatus.authToken.length === 0) return
            if (!DeviceManager.busy) DeviceManager.refreshDevices()
            if (!ForwardManager.busy) ForwardManager.refreshTasks()
        }
    }

    Connections {
        target: P2pTransferManager
        function onIncomingProgress(forwardId, verifiedBytes) {
            ForwardManager.reportProgress(forwardId, verifiedBytes)
        }
        function onIncomingCompleted(forwardId, totalBytes) {
            root.showTransferCompleted(forwardId, totalBytes)
            ForwardManager.finishDownload(forwardId, totalBytes)
        }
        function onIncomingFailed(forwardId, code, message) {
            ForwardManager.fail(forwardId, code + ": " + message)
        }
        function onOutgoingFallbackRequested(forwardId, targetDeviceId, destinationPath, localFilePath, directory, reason) {
            if (forwardId && forwardId.length > 0)
                ForwardManager.hideSupersededTask(forwardId)
            if (forwardId && forwardId.length > 0)
                ForwardManager.fail(forwardId, "P2P_FALLBACK: " + reason)
            devicesPage.fallbackToRelay(targetDeviceId, destinationPath, localFilePath, directory, reason)
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: Theme.canvas
        border.width: 1
        border.color: "#C3D2CB"
        clip: true

        // ---- title bar -----------------------------------------------------
        Rectangle {
            id: titleBar
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            height: 42
            color: Theme.surface
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
            MouseArea { anchors.fill: parent; onPressed: root.startSystemMove() }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14; anchors.rightMargin: 6
                spacing: 9
                Image {
                    source: "qrc:/qt/qml/MyTest/Icons/app.png"
                    sourceSize.width: 44
                    sourceSize.height: 44
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
                Text { text: qsTr("MyFolder"); font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.ink }
                Item { Layout.fillWidth: true }
                RowLayout {
                    spacing: 1
                    ToolButton {
                        text: "–"; onClicked: root.showMinimized()
                        implicitWidth: 32; implicitHeight: 28
                        contentItem: Text { text: parent.text; color: Theme.muted; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { radius: 5; color: parent.hovered ? Theme.sunken : "transparent" }
                    }
                    ToolButton {
                        text: "□"; onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                        implicitWidth: 32; implicitHeight: 28
                        contentItem: Text { text: parent.text; color: Theme.muted; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { radius: 5; color: parent.hovered ? Theme.sunken : "transparent" }
                    }
                    ToolButton {
                        text: "×"; onClicked: root.hide()   // stays in tray; transfers keep running
                        implicitWidth: 32; implicitHeight: 28
                        contentItem: Text { text: parent.text; color: parent.hovered ? "#FFFFFF" : Theme.muted; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { radius: 5; color: parent.hovered ? Theme.alert : "transparent" }
                    }
                }
            }
        }

        // ---- body ----------------------------------------------------------
        RowLayout {
            anchors.top: titleBar.bottom
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            spacing: 0

            // rail
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 186
                color: Theme.surface
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    anchors.topMargin: 12
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "WORKSPACE"; font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.5; color: Theme.faint; Layout.leftMargin: 8; Layout.bottomMargin: 5 }
                        RailItem { label: qsTr("文件"); active: root.activePage === 0; onClicked: root.activePage = 0 }
                        RailItem { label: qsTr("设备"); count: devicesPage.visibleDevices.length; active: root.activePage === 1; onClicked: root.activePage = 1 }
                        RailItem { label: qsTr("传输"); count: transfersPage.activeCount + transfersPage.attentionCount; active: root.activePage === 2; onClicked: root.activePage = 2 }
                        RailItem { label: qsTr("群组"); active: root.activePage === 3; onClicked: root.activePage = 3 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "RECORDS"; font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.5; color: Theme.faint; Layout.leftMargin: 8; Layout.bottomMargin: 5 }
                        RailItem { label: qsTr("传输记录"); active: root.activePage === 4; onClicked: root.activePage = 4 }
                        RailItem { label: qsTr("设置"); active: root.activePage === 5; onClicked: root.activePage = 5 }
                    }

                    Item { Layout.fillHeight: true }

                    // local agent strip: things only the client knows
                    Rectangle {
                        id: connectionStrip
                        Layout.fillWidth: true
                        implicitHeight: 46
                        radius: Theme.radiusSm
                        color: Theme.sunken
                        border.width: 1; border.color: Theme.line
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 9
                            spacing: 9
                            UiLed { on: WebSocketClient.connected }
                            Text {
                                Layout.fillWidth: true
                                text: root.connectionLabel()
                                font.family: Theme.dataFont
                                font.pixelSize: 10
                                lineHeight: 1.4
                                color: Theme.muted
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: WebSocketClient.connectionState === "FAILED"
                                         ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: if (WebSocketClient.connectionState === "FAILED")
                                           WebSocketClient.reconnect()
                        }
                        ToolTip.visible: connectionHover.containsMouse && WebSocketClient.lastError.length > 0
                        ToolTip.text: WebSocketClient.lastError
                        MouseArea {
                            id: connectionHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 45
                        radius: Theme.radiusSm
                        color: Theme.sunken
                        border.width: 1; border.color: Theme.line
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 9
                            spacing: 9
                            Rectangle {
                                width: 27; height: 27; radius: 7
                                gradient: Gradient {
                                    GradientStop { position: 0; color: "#2BB083" }
                                    GradientStop { position: 1; color: "#14724F" }
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: (GlobalStatus.userName || "?").substring(0, 2).toUpperCase()
                                    color: "#FFFFFF"; font.pixelSize: 10; font.bold: true
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text { text: GlobalStatus.userName; font.pixelSize: 12; font.weight: Font.DemiBold; color: Theme.ink; elide: Text.ElideRight; Layout.fillWidth: true }
                                Text { text: GlobalStatus.dataFolder; font.family: Theme.dataFont; font.pixelSize: 9; color: Theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }

            // pages
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.activePage
                ServerFilesPage {
                    id: serverFilesPage
                    onUploadStarted: root.activePage = 2
                }
                DevicesPage {
                    id: devicesPage
                    onStorageSettingsRequested: {
                        if (!ShellIntegration.openFolder(RelayDownloadManager.receiveRoot))
                            root.activePage = 5
                    }
                    onServerUploadRequested: function(filePaths) { serverUploadDialog.openForPaths(filePaths) }
                }
                TransfersPage {
                    id: transfersPage
                    onAcceptTask: function(task) { root.acceptIncomingTask(task, false) }
                }
                GroupsPage {
                    id: groupsPage
                    onGroupUploadStarted: root.activePage = 2
                }
                HistoryPage { id: historyPage }
                SettingsPage {
                    id: settingsPage
                    onLogoutRequested: root.logoutRequested()
                }
            }
        }
    }

    Rectangle {
        visible: false
        z: 1000
        width: Math.min(360, root.width - 40)
        implicitHeight: incomingColumn.implicitHeight + 28
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        radius: 10
        color: Theme.surface
        border.width: 1
        border.color: Theme.signalEdge

        ColumnLayout {
            id: incomingColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 9

            RowLayout {
                Layout.fillWidth: true
                spacing: 9
                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    radius: 7
                    color: Theme.signalWash
                    Text { anchors.centerIn: parent; text: "↓"; color: Theme.signalDeep; font.pixelSize: 16; font.bold: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: qsTr("收到文件"); font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink }
                    Text {
                        Layout.fillWidth: true
                        text: root.incomingOfferTitle(root.incomingOffer)
                        font.pixelSize: 12
                        color: Theme.muted
                        elide: Text.ElideMiddle
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("保存到：") + (root.incomingOffer ? root.incomingOffer.destinationPath || "MyFolder" : "MyFolder")
                font.family: Theme.dataFont
                font.pixelSize: 10
                color: Theme.faint
                elide: Text.ElideMiddle
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                UiButton {
                    Layout.fillWidth: true
                    text: qsTr("稍后")
                    onClicked: root.incomingOffer = null
                }
                UiButton {
                    Layout.fillWidth: true
                    kind: "primary"
                    text: qsTr("接收")
                    onClicked: {
                        if (root.incomingOffer) root.acceptIncomingTask(root.incomingOffer)
                        root.incomingOffer = null
                        root.activePage = 2
                    }
                }
            }
        }
    }
}
