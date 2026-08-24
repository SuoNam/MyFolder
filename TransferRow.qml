import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property var task: ({})
    signal pauseRequested()
    signal cancelRequested()
    signal retryRequested()
    signal acceptRequested()
    signal rejectRequested()
    signal dismissRequested()

    readonly property bool done: task.state === "COMPLETED"
    readonly property bool failed: task.state === "FAILED"
    readonly property bool offered: task.state === "OFFERED"
    readonly property bool serverUpload: task.serverUpload === true
    property string sampledTaskKey: ""
    property double sampledBytes: 0
    property double sampledAtMs: 0
    property double bytesPerSecond: 0

    function taskKey() {
        return String(task.forwardId || task.uploadId || task.title || "")
    }

    function currentBytes() {
        return Math.max(0, Number(task.transferredBytes || 0))
    }

    function formatRate(value) {
        if (!isFinite(value) || value <= 0)
            return qsTr("测速中…")
        return Theme.formatBytes(value) + "/s"
    }

    function formatEta(value) {
        if (!isFinite(value) || value <= 0)
            return ""
        var seconds = Math.ceil(value)
        if (seconds < 60)
            return qsTr("约 %1 秒").arg(seconds)
        if (seconds < 3600)
            return qsTr("约 %1 分 %2 秒").arg(Math.floor(seconds / 60)).arg(seconds % 60)
        return qsTr("约 %1 小时 %2 分").arg(Math.floor(seconds / 3600)).arg(Math.floor((seconds % 3600) / 60))
    }

    function resetSpeedSample() {
        sampledTaskKey = taskKey()
        sampledBytes = currentBytes()
        sampledAtMs = Date.now()
        bytesPerSecond = 0
    }

    onTaskChanged: {
        if (taskKey() !== sampledTaskKey)
            resetSpeedSample()
    }

    Component.onCompleted: resetSpeedSample()

    Timer {
        interval: 1000
        repeat: true
        running: root.visible && !root.done && !root.failed && !root.offered
        onTriggered: {
            var now = Date.now()
            var current = root.currentBytes()
            var elapsed = now - root.sampledAtMs
            if (elapsed > 0 && current >= root.sampledBytes) {
                var instant = (current - root.sampledBytes) * 1000 / elapsed
                root.bytesPerSecond = root.bytesPerSecond > 0
                        ? root.bytesPerSecond * 0.55 + instant * 0.45
                        : instant
            } else if (current < root.sampledBytes) {
                root.bytesPerSecond = 0
            }
            root.sampledBytes = current
            root.sampledAtMs = now
        }
    }

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
                Text {
                    visible: String(root.task.statusText || root.task.failureReason || "").length > 0
                    Layout.fillWidth: true
                    text: root.failed
                          ? String(root.task.failureReason || root.task.statusText || "")
                          : String(root.task.statusText || "")
                    font.pixelSize: 10
                    color: root.failed ? Theme.alert : Theme.faint
                    wrapMode: Text.WordWrap
                    maximumLineCount: root.failed ? 3 : 1
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                spacing: 2
                visible: !root.done && !root.failed && !root.offered
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.task.rate || root.formatRate(root.bytesPerSecond)
                    font.family: Theme.dataFont
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.ink
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    text: root.task.eta || root.formatEta((Number(root.task.totalBytes || 0)
                                                          - root.currentBytes()) / root.bytesPerSecond)
                    font.family: Theme.dataFont
                    font.pixelSize: 10
                    color: Theme.faint
                }
            }

            UiChip { visible: root.done; channel: root.task.channel || "LAN"; text: root.serverUpload ? qsTr("已上传") : (root.task.channel || "LAN") }
            UiChip { visible: root.offered && root.task.incoming && root.task.automaticAcceptance === true; channel: root.task.channel || "LAN"; text: qsTr("自动切换") }
            UiButton { visible: root.offered && root.task.incoming && root.task.automaticAcceptance !== true; kind: "danger"; implicitHeight: 26; text: qsTr("拒绝"); onClicked: root.rejectRequested() }
            UiButton { visible: root.offered && root.task.incoming && root.task.automaticAcceptance !== true; kind: "primary"; implicitHeight: 26; text: qsTr("接收"); onClicked: root.acceptRequested() }
            UiButton { visible: root.failed && root.task.canRetry !== false; implicitHeight: 26; text: qsTr("重试"); onClicked: root.retryRequested() }
            UiButton { visible: root.failed; kind: "quiet"; implicitHeight: 26; text: qsTr("移除"); onClicked: root.dismissRequested() }
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
