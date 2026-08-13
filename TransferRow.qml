import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property var task: ({})
    signal pauseRequested()
    signal cancelRequested()
    signal retryRequested()
    signal acceptRequested()

    readonly property bool done: task.state === "COMPLETED"
    readonly property bool failed: task.state === "FAILED"
    readonly property bool offered: task.state === "OFFERED"
    readonly property bool serverUpload: task.serverUpload === true

    color: "transparent"
    implicitHeight: body.implicitHeight + 24

    ColumnLayout {
        id: body
        anchors.fill: parent
        anchors.margins: 12
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 11

            Rectangle {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 26
                radius: 6
                color: root.done ? Theme.signalWash : root.failed ? Theme.alertWash : Theme.sunken
                border.width: 1
                border.color: root.done ? "#C8E8D9" : root.failed ? Theme.alertEdge : Theme.line
                Text {
                    anchors.centerIn: parent
                    text: root.done ? "✓" : root.failed ? "!" : root.serverUpload ? "↑" : root.task.incoming ? "↓" : "→"
                    font.pixelSize: 12
                    font.bold: true
                    color: root.done ? Theme.signalDeep : root.failed ? Theme.alert : Theme.ink2
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: root.task.title || "-"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: Theme.ink
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: (root.serverUpload ? qsTr("上传到 ")
                                             : root.task.incoming ? qsTr("接收自 ") : qsTr("发送到 "))
                          + (root.task.peer || "-")
                          + (root.task.sizeText ? " · " + root.task.sizeText : "")
                    font.family: Theme.dataFont
                    font.pixelSize: 11
                    color: root.failed ? Theme.alert : Theme.muted
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                spacing: 2
                visible: !root.done && !root.failed && !root.offered
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.task.rate || ""
                    font.family: Theme.dataFont
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.ink
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.task.eta || ""
                    font.family: Theme.dataFont
                    font.pixelSize: 10
                    color: Theme.faint
                }
            }

            UiChip { visible: root.done; channel: root.task.channel || "LAN"; text: root.serverUpload ? qsTr("已上传") : (root.task.channel || "LAN") }
            UiButton { visible: root.offered; kind: "primary"; implicitHeight: 26; text: qsTr("接收"); onClicked: root.acceptRequested() }
            UiButton { visible: root.failed; implicitHeight: 26; text: qsTr("重试"); onClicked: root.retryRequested() }
        }

        RowLayout {
            visible: !root.done && !root.failed && !root.offered
            Layout.fillWidth: true
            spacing: 10
            UiChip { channel: root.task.channel || "LAN"; text: root.serverUpload ? qsTr("服务器") : (root.task.channel || "LAN") }
            UiProgress {
                Layout.fillWidth: true
                value: root.task.progress || 0
                channel: root.task.channel || "LAN"
            }
            Text {
                Layout.preferredWidth: 34
                horizontalAlignment: Text.AlignRight
                text: Math.round((root.task.progress || 0) * 100) + "%"
                font.family: Theme.dataFont
                font.pixelSize: 11
                color: Theme.muted
            }
            UiButton { kind: "quiet"; implicitHeight: 26; leftPadding: 8; rightPadding: 8; text: "Ⅱ"; onClicked: root.pauseRequested() }
            UiButton { kind: "quiet"; implicitHeight: 26; leftPadding: 8; rightPadding: 8; text: "×"; onClicked: root.cancelRequested() }
        }
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.lineSoft }
}
