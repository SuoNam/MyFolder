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

    property int activePage: 0   // 0 devices, 1 transfers, 2 groups, 3 history, 4 settings
    property var incomingOffer: null

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

    function handleStartupAction(action, filePath) {
        if (!filePath || filePath.length === 0) return
        if (action === "upload-server") {
            serverUploadDialog.openFor(filePath)
        } else if (action === "send-client") {
            activePage = 0
            Qt.callLater(function() { devicesPage.sendFileFromShell(filePath) })
        }
    }

    Platform.SystemTrayIcon {
        visible: true
        icon.source: "qrc:/qt/qml/MyTest/Icons/app.ico"
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

    function activateDeviceSession(deviceId, deviceToken) {
        ForwardManager.currentDeviceId = deviceId
        ForwardManager.currentDeviceToken = deviceToken
        RelayDownloadManager.currentDeviceId = deviceId
        RelayDownloadManager.currentDeviceToken = deviceToken
        WebSocketClient.connectTo(Config.baseUrl, GlobalStatus.authToken,
                                  deviceId, deviceToken)
        DeviceManager.refreshDevices()
        ForwardManager.refreshTasks()
    }

    Component.onCompleted: {
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
        target: DeviceManager
        function onCredentialsReady(deviceId, deviceToken) {
            root.activateDeviceSession(deviceId, deviceToken)
        }
    }

    Connections {
        target: WebSocketClient
        function onDeviceListReceived(devices) { DeviceManager.applyWebSocketDevices(devices) }
        function onForwardEventReceived(action, forwardId, payload) {
            ForwardManager.handleForwardEvent(action, payload)
        }
    }

    Connections {
        target: ForwardManager
        function onTaskUpdated(task) {
            if (task.targetDeviceId !== DeviceManager.deviceId || task.channel !== "RELAY")
                return
            if (task.state === "OFFERED") {
                root.incomingOffer = task
            } else if (task.state === "TRANSFERRING")
                RelayDownloadManager.startTask(task)
            else if (task.state === "COMPLETED")
                RelayDownloadManager.confirmTaskCompleted(task.forwardId)
        }
        function onFinalizationFailed(forwardId, code) {
            RelayDownloadManager.handleFinalizationFailure(forwardId, code)
        }
    }

    ServerUploadDialog {
        id: serverUploadDialog
        onUploadConfirmed: function(filePath, serverPath) {
            var taskId = TransferManager.startFileUpload(filePath, serverPath)
            root.activePage = 1
            root.show(); root.raise(); root.requestActivate()
            if (!taskId || taskId.length === 0)
                console.warn("Unable to create upload task for", filePath)
        }
    }

    IncomingTransferToast {
        offer: root.incomingOffer
        onLaterRequested: root.incomingOffer = null
        onAcceptRequested: {
            if (root.incomingOffer) ForwardManager.accept(root.incomingOffer.forwardId)
            root.incomingOffer = null
        }
    }

    Connections {
        target: RelayDownloadManager
        function onProgressReady(forwardId, verifiedBytes) {
            ForwardManager.reportProgress(forwardId, verifiedBytes)
        }
        function onTaskDownloadCompleted(forwardId, totalBytes) {
            ForwardManager.finishDownload(forwardId, totalBytes)
        }
        function onTaskDownloadFailed(forwardId, code, message) {
            ForwardManager.fail(forwardId, code + ": " + message)
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
                        RailItem { label: qsTr("设备");   count: DeviceManager.devices.length; active: root.activePage === 0; onClicked: root.activePage = 0 }
                        RailItem { label: qsTr("传输");   count: ForwardManager.tasks.length;  active: root.activePage === 1; onClicked: root.activePage = 1 }
                        RailItem { label: qsTr("群组"); active: root.activePage === 2; onClicked: root.activePage = 2 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "RECORDS"; font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.5; color: Theme.faint; Layout.leftMargin: 8; Layout.bottomMargin: 5 }
                        RailItem { label: qsTr("传输记录"); active: root.activePage === 3; onClicked: root.activePage = 3 }
                        RailItem { label: qsTr("设置");     active: root.activePage === 4; onClicked: root.activePage = 4 }
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
                DevicesPage {
                    id: devicesPage
                    onStorageSettingsRequested: Qt.openUrlExternally("file:///" + RelayDownloadManager.receiveRoot.replace(/\\/g, "/"))
                    onServerUploadRequested: function(filePath) { serverUploadDialog.openFor(filePath) }
                }
                TransfersPage {}
                GroupsPage {}
                HistoryPage {}
                SettingsPage { onLogoutRequested: root.logoutRequested() }
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
                        if (root.incomingOffer) ForwardManager.accept(root.incomingOffer.forwardId)
                        root.incomingOffer = null
                        root.activePage = 1
                    }
                }
            }
        }
    }
}
