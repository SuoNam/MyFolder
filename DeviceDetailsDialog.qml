pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    modal: true
    anchors.centerIn: parent
    width: 430
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var device: ({})

    function openFor(value) {
        device = value || ({})
        open()
    }

    background: Rectangle {
        radius: 11
        color: Theme.surface
        border.width: 1
        border.color: "#C3D2CB"
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            Text {
                Layout.fillWidth: true
                text: root.device.deviceName || qsTr("未命名设备")
                font.pixelSize: 16
                font.bold: true
                color: Theme.ink
                elide: Text.ElideRight
            }
            UiChip {
                channel: root.device.online ? "LAN" : "idle"
                text: root.device.online ? qsTr("在线") : qsTr("离线")
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

        GridLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            columns: 2
            columnSpacing: 16
            rowSpacing: 11

            Repeater {
                model: [
                    { label: qsTr("设备 ID"), value: root.device.deviceId || "-" },
                    { label: qsTr("设备类型"), value: root.device.deviceType || "-" },
                    { label: qsTr("操作系统"), value: root.device.os || "-" },
                    { label: qsTr("设备地址"), value: root.device.deviceAddress || "-" },
                    { label: qsTr("监听端口"), value: root.device.listenPort || "-" },
                    { label: qsTr("客户端版本"), value: root.device.clientVersion || "-" },
                    { label: qsTr("最后在线"), value: root.device.lastSeenAt || "-" }
                ]
                delegate: Item {
                    id: detailRow
                    required property var modelData
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    implicitHeight: 22
                    RowLayout {
                        anchors.fill: parent
                        Text {
                            Layout.preferredWidth: 76
                            text: detailRow.modelData.label
                            font.pixelSize: 11
                            color: Theme.faint
                        }
                        Text {
                            Layout.fillWidth: true
                            text: detailRow.modelData.value
                            font.family: Theme.dataFont
                            font.pixelSize: 11
                            color: Theme.ink2
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 52
            color: Theme.sunken
            Rectangle { width: parent.width; height: 1; color: Theme.lineSoft }
            UiButton {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("关闭")
                onClicked: root.close()
            }
        }
    }
}
