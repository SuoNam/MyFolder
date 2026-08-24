pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal logoutRequested()
    property string accountName: ""
    property string accountEmail: ""
    property string displayName: ""
    property string accountMessage: ""
    property var storageUsage: null
    property bool preferenceBusy: false

    function refreshContent() {
        HttpHandler.loadAccountProfile()
        HttpHandler.loadStorageUsage()
    }

    function formatBytes(value) {
        var n = Number(value || 0); var units = ["B", "KiB", "MiB", "GiB", "TiB"]; var i = 0
        while (n >= 1024 && i < units.length - 1) { n /= 1024; i++ }
        return (i === 0 ? n.toFixed(0) : n.toFixed(1)) + " " + units[i]
    }

    Component.onCompleted: root.refreshContent()

    Connections {
        target: HttpHandler
        function onAccountProfileResult(response) {
            if (response.status === 200 && response.data) {
                root.accountName = response.data.account || ""
                root.accountEmail = response.data.email || ""
                root.displayName = response.data.displayName || root.accountName
                GlobalStatus.autoAcceptDeviceTransfers = response.data.autoAcceptDeviceTransfers === true
            } else root.accountMessage = response.message || qsTr("无法读取账号信息")
        }
        function onDisplayNameUpdateResult(response) {
            if (response.status === 200 && response.data) {
                root.displayName = response.data.displayName || root.displayName
                root.accountMessage = qsTr("用户名已保存")
            } else root.accountMessage = response.message || qsTr("保存失败")
        }
        function onStorageUsageResult(response) { if (response.status === 200) root.storageUsage = response.data }
        function onTransferPreferencesUpdateResult(response) {
            root.preferenceBusy = false
            if (response.status === 200 && response.data) {
                GlobalStatus.autoAcceptDeviceTransfers = response.data.autoAcceptDeviceTransfers === true
                root.accountMessage = qsTr("自动接收设置已保存")
            } else {
                root.accountMessage = response.message || qsTr("保存自动接收设置失败")
            }
        }
    }

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        anchors.margins: 16
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: Math.min(760, settingsScroll.availableWidth)
            x: Math.max(0, (settingsScroll.availableWidth - width) / 2)
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("设置"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
                Item { Layout.fillWidth: true }
            }

            SettingsCard {
                title: qsTr("目录")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text { text: qsTr("同步目录"); font.pixelSize: 12; font.weight: Font.Medium; color: Theme.ink2 }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            UiInput {
                                Layout.fillWidth: true
                                mono: true
                                text: GlobalStatus.dataFolder
                                readOnly: true
                            }
                            UiButton {
                                text: qsTr("打开")
                                onClicked: ShellIntegration.openFolder(GlobalStatus.dataFolder)
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text { text: qsTr("收到的文件保存到"); font.pixelSize: 12; font.weight: Font.Medium; color: Theme.ink2 }
                        UiInput {
                            Layout.fillWidth: true
                            mono: true
                            text: RelayDownloadManager.receiveRoot
                            onEditingFinished: {
                                RelayDownloadManager.receiveRoot = text
                                LanTransferManager.receiveRoot = RelayDownloadManager.receiveRoot
                                text = RelayDownloadManager.receiveRoot
                            }
                        }
                        UiButton {
                            text: qsTr("打开文件夹")
                            onClicked: ShellIntegration.openFolder(RelayDownloadManager.receiveRoot)
                        }
                        Text {
                            visible: ShellIntegration.lastError.length > 0
                            Layout.fillWidth: true
                            text: ShellIntegration.lastError
                            color: Theme.alert
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            SettingsCard {
                title: qsTr("传输通道")
                badge: qsTr("建议保持默认")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("按顺序尝试，第一条可用通道会被采用；后续通道用于自动兜底。")
                        font.pixelSize: 12
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                    }
                    RouteLadder {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 520
                        animate: false
                        rungs: [
                            {name: "LAN", state: "live", note: qsTr("同一局域网内直传，速度最快")},
                            {name: "P2P", state: "live", note: qsTr("ICE/STUN 打洞，DTLS 加密传输")},
                            {name: "RELAY", state: "live", note: qsTr("服务器中转兜底")}
                        ]
                    }
                }
            }

            SettingsCard {
                title: qsTr("接收文件")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Text { text: qsTr("自动接收其他设备的文件"); font.pixelSize: 12; font.weight: Font.Medium; color: Theme.ink2 }
                            Text { text: qsTr("此设置跟随账号，并同步到所有桌面客户端"); font.pixelSize: 10; color: Theme.muted }
                        }
                        Switch {
                            checked: GlobalStatus.autoAcceptDeviceTransfers
                            enabled: !root.preferenceBusy
                            Accessible.name: qsTr("自动接收其他设备的文件")
                            onToggled: {
                                root.preferenceBusy = true
                                HttpHandler.updateTransferPreferences(checked)
                            }
                        }
                    }
                }
            }

            SettingsCard {
                title: qsTr("连接")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    InfoRow { label: qsTr("服务器"); value: Config.baseUrl }
                    InfoRow { label: qsTr("状态"); value: WebSocketClient.connectionState }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text { text: qsTr("设备名称"); font.pixelSize: 12; font.weight: Font.Medium; color: Theme.ink2 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 7
                            UiInput { id: deviceNameInput; Layout.fillWidth: true; text: DeviceManager.deviceName; maximumLength: 64 }
                            UiButton {
                                text: qsTr("保存")
                                enabled: !DeviceManager.busy && deviceNameInput.text.trim().length > 0
                                onClicked: DeviceManager.updateDeviceName(deviceNameInput.text)
                            }
                        }
                        Text { visible: deviceNameStatus.text.length > 0; id: deviceNameStatus; font.pixelSize: 11; color: Theme.muted }
                        Connections {
                            target: DeviceManager
                            function onDeviceNameUpdateFinished(success, message) {
                                deviceNameStatus.color = success ? Theme.signalDeep : Theme.alert
                                deviceNameStatus.text = success ? qsTr("设备名称已保存") : message
                            }
                        }
                    }
                    InfoRow { label: qsTr("设备 ID"); value: DeviceManager.deviceId; dim: true }
                }
            }

            SettingsCard {
                visible: root.storageUsage !== null
                title: qsTr("存储空间")
                badge: root.storageUsage && root.storageUsage.systemRole === "SUPER_ADMIN" ? qsTr("超级管理员") : root.storageUsage && root.storageUsage.tier === "PREMIUM" ? qsTr("高级用户") : qsTr("普通用户")
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 8
                    InfoRow { label: qsTr("已使用"); value: root.storageUsage ? root.formatBytes(root.storageUsage.usedBytes) : "—" }
                    InfoRow { label: qsTr("容量上限"); value: root.storageUsage && root.storageUsage.limitBytes !== null ? root.formatBytes(root.storageUsage.limitBytes) : qsTr("不限量") }
                    Text { text: qsTr("私人文件与本人贡献到群组的文件共同计入额度。"); color: Theme.muted; font.pixelSize: 11 }
                }
            }

            SettingsCard {
                title: qsTr("账号")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 9
                    Text { text: qsTr("用户名"); font.pixelSize: 12; font.weight: Font.Medium; color: Theme.ink2 }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7
                        UiInput {
                            id: accountDisplayNameInput
                            Layout.fillWidth: true
                            text: root.displayName
                            maximumLength: 40
                        }
                        UiButton {
                            text: qsTr("保存")
                            enabled: accountDisplayNameInput.text.trim().length >= 2
                            onClicked: HttpHandler.updateDisplayName(accountDisplayNameInput.text)
                        }
                    }
                    InfoRow { label: qsTr("登录账号"); value: root.accountName; dim: true }
                    InfoRow { label: qsTr("邮箱"); value: root.accountEmail; dim: true }
                    Text {
                        visible: root.accountMessage.length > 0
                        text: root.accountMessage
                        color: Theme.muted
                        font.pixelSize: 11
                    }
                }
            }

            SettingsCard {
                title: qsTr("资源管理器右键菜单")
                badge: ShellIntegration.enabledCount === 0
                       ? qsTr("已关闭")
                       : qsTr("已启用 %1 项").arg(ShellIntegration.enabledCount)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("选择文件右键时显示的 MyFolder 功能。关闭全部功能后，MyFolder 入口也会隐藏。")
                        color: Theme.muted
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                    Repeater {
                        model: ShellIntegration.commands
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 58
                            radius: 7
                            color: Theme.sunken
                            border.width: 1
                            border.color: Theme.lineSoft

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 10
                                spacing: 10
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3
                                    Text {
                                        text: modelData.title
                                        color: Theme.ink
                                        font.pixelSize: 12
                                        font.weight: Font.Medium
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.description
                                        color: Theme.muted
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                }
                                Switch {
                                    checked: modelData.enabled
                                    Accessible.name: modelData.title
                                    onToggled: ShellIntegration.setCommandEnabled(modelData.id, checked)
                                }
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("修改后重新打开文件右键菜单即可生效。")
                        color: Theme.faint
                        font.pixelSize: 10
                    }
                }
            }

            SettingsCard {
                title: qsTr("关于")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    InfoRow { label: qsTr("版本"); value: Qt.application.version }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        UiButton { text: qsTr("检查更新") }
                        Item { Layout.fillWidth: true }
                        UiButton {
                            kind: "danger"
                            text: qsTr("退出登录")
                            onClicked: {
                                DeviceManager.stop(true)
                                WebSocketClient.disconnectFromServer()
                                HttpHandler.logout()
                                root.logoutRequested()
                            }
                        }
                    }
                }
            }
        }
    }

    component SettingsCard: Rectangle {
        id: card
        default property alias content: inner.data
        property string title: ""
        property string badge: ""

        Layout.fillWidth: true
        implicitHeight: cardColumn.implicitHeight
        radius: Theme.radius
        color: Theme.surface
        border.width: 1
        border.color: Theme.line

        ColumnLayout {
            id: cardColumn
            width: parent.width
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.topMargin: 12
                Layout.bottomMargin: 11
                Text { text: card.title; font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink }
                Item { Layout.fillWidth: true }
                UiChip { visible: card.badge.length > 0; channel: "LAN"; text: card.badge }
            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

            ColumnLayout {
                id: inner
                Layout.fillWidth: true
                Layout.leftMargin: 14
                Layout.rightMargin: 14
                Layout.topMargin: 12
                Layout.bottomMargin: 14
                spacing: 10
            }
        }
    }

    component InfoRow: RowLayout {
        property string label: ""
        property string value: ""
        property bool dim: false

        Layout.fillWidth: true
        spacing: 12

        Text {
            Layout.preferredWidth: 88
            text: parent.label
            font.family: Theme.dataFont
            font.pixelSize: 10
            font.letterSpacing: 1.3
            color: Theme.faint
        }
        Text {
            Layout.fillWidth: true
            text: parent.value
            font.family: Theme.dataFont
            font.pixelSize: 12
            color: parent.dim ? Theme.muted : Theme.ink
            elide: Text.ElideMiddle
        }
    }
}
