import QtQuick
import QtQuick.Layouts

// One device card: identity row, route ladder, action strip.
// `device` is a map from DeviceManager.devices (deviceId, deviceName, os,
// deviceAddress, clientVersion, online, lastSeenAt).
Rectangle {
    id: root
    property var device: ({})
    property bool isSelf: false
    property bool selected: false
    signal sendRequested()
    signal detailRequested()
    signal clicked()

    readonly property bool online: isSelf ? WebSocketClient.connected : device.online === true
    readonly property bool isWeb: String(device.deviceType || "").toUpperCase() === "WEB"
    readonly property bool canReceive: !isSelf && !isWeb
    // Route plan for this peer, judged from its address: RFC1918 → assume same
    // LAN and try direct first; public address → hole-punch P2P; offline → RELAY.
    // The transfer layer still probes in LAN → P2P → RELAY order at send time.
    readonly property bool lanLikely: {
        var addr = String(device.deviceAddress || "")
        return addr.indexOf("192.168.") === 0 || addr.indexOf("10.") === 0
            || /^172\.(1[6-9]|2[0-9]|3[01])\./.test(addr)
    }
    readonly property string plannedChannel: !online ? "RELAY" : lanLikely ? "LAN" : "P2P"

    radius: Theme.radius
    color: online ? Theme.surface : Theme.sunken
    border.width: 1
    border.color: selected ? Theme.signal : hoverArea.containsMouse ? "#C9DBD2" : Theme.line

    Rectangle {
        visible: root.selected
        anchors.fill: parent; anchors.margins: -3
        radius: Theme.radius + 3
        color: "transparent"
        border.width: 3; border.color: Theme.signalWash
        z: -1
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // identity
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.bottomMargin: 11
            spacing: 11

            Rectangle {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34
                radius: 8
                color: root.online ? Theme.signalWash : Theme.sunken
                border.width: 1
                border.color: root.online ? "#C8E8D9" : Theme.line
                Text {
                    anchors.centerIn: parent
                    text: {
                        var os = (root.device.os || "").toLowerCase()
                        if (os.indexOf("android") >= 0 || os.indexOf("ios") >= 0) return "📱"
                        if (os.indexOf("mac") >= 0) return "💻"
                        return "🖥"
                    }
                    font.pixelSize: 15
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                RowLayout {
                    spacing: 7
                    UiLed { on: root.online; busy: false }
                    Text {
                        Layout.fillWidth: true
                        text: root.device.deviceName || qsTr("未命名设备")
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: root.online ? Theme.ink : Theme.ink2
                        elide: Text.ElideRight
                    }
                    Rectangle {
                        visible: root.isSelf
                        implicitWidth: selfLabel.implicitWidth + 8
                        implicitHeight: 16
                        radius: 3
                        color: "transparent"
                        border.width: 1; border.color: Theme.line
                        Text {
                            id: selfLabel
                            anchors.centerIn: parent
                            text: qsTr("本机")
                            font.family: Theme.dataFont
                            font.pixelSize: 9; font.bold: true
                            color: Theme.faint
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: (root.device.os || "-") + " · " + (root.device.deviceAddress || "-")
                    font.family: Theme.dataFont
                    font.pixelSize: 10
                    color: Theme.muted
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("客户端 ") + (root.device.clientVersion || "-") + " · "
                          + (root.online ? qsTr("在线") : (root.device.lastSeenAt || qsTr("离线")))
                    font.family: Theme.dataFont
                    font.pixelSize: 10
                    color: Theme.faint
                    elide: Text.ElideRight
                }
            }

            UiChip {
                Layout.alignment: Qt.AlignTop
                channel: root.isWeb ? "idle" : root.online ? root.plannedChannel : "idle"
                text: root.isWeb ? qsTr("控制台") : root.online ? root.plannedChannel : qsTr("离线")
            }
        }

        // route ladder
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 15; Layout.rightMargin: 15
            spacing: 9
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "ROUTE"
                    font.family: Theme.dataFont
                    font.pixelSize: 10
                    font.letterSpacing: 1.5
                    color: Theme.faint
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.isWeb ? qsTr("仅用于管理")
                        : !root.online ? qsTr("先存服务器")
                        : root.plannedChannel === "LAN" ? qsTr("走局域网直传") : qsTr("走 P2P 直连")
                    font.family: Theme.dataFont
                    font.pixelSize: 11
                    color: Theme.ink2
                }
            }
            RouteLadder {
                visible: !root.isWeb
                Layout.fillWidth: true
                animate: root.online
                rungs: !root.online
                    ? [{name: "LAN",   state: "out",  note: qsTr("设备离线")},
                       {name: "P2P",   state: "out",  note: qsTr("设备离线")},
                       {name: "RELAY", state: "live", note: qsTr("文件先存服务器，上线自动下载")}]
                    : root.plannedChannel === "LAN"
                    ? [{name: "LAN",   state: "live", note: qsTr("同网段可直连")},
                       {name: "P2P",   state: "wait", note: qsTr("无需尝试")},
                       {name: "RELAY", state: "wait", note: qsTr("无需尝试")}]
                    : [{name: "LAN",   state: "out",  note: qsTr("不在同一网段")},
                       {name: "P2P",   state: "live", note: qsTr("尝试打洞直连")},
                       {name: "RELAY", state: "wait", note: qsTr("备用通道")}]
            }
            Text {
                visible: root.isWeb
                Layout.fillWidth: true
                text: qsTr("Web 控制台不能作为文件接收端")
                font.pixelSize: 11
                color: Theme.muted
            }
        }

        Item { Layout.fillHeight: true }

        // actions
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 50
            color: Theme.sunken
            Rectangle { width: parent.width; height: 1; color: Theme.lineSoft }
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 7
                UiButton {
                    Layout.fillWidth: true
                    visible: root.canReceive
                    kind: root.online ? "primary" : "secondary"
                    implicitHeight: 28
                    text: root.online ? qsTr("发送文件") : qsTr("发送并等待上线")
                    onClicked: root.sendRequested()
                }
                UiButton {
                    Layout.fillWidth: root.isSelf || !root.canReceive
                    implicitHeight: 28
                    text: root.isSelf ? qsTr("文件保存位置") : qsTr("详情")
                    onClicked: root.detailRequested()
                }
            }
        }
    }
}
