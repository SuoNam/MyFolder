pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// History page: settled forwards as a ledger table.
Item {
    id: root

    property var serverUploads: []
    property string loadError: ""

    function refreshContent() {
        ForwardManager.refreshHistory()
        HttpHandler.loadUploadHistory()
    }

    function deviceName(deviceId) {
        for (var i = 0; i < DeviceManager.devices.length; ++i)
            if (String(DeviceManager.devices[i].deviceId || "") === String(deviceId || ""))
                return String(DeviceManager.devices[i].deviceName || deviceId)
        return String(deviceId || qsTr("未知设备"))
    }

    function timestampValue(value) {
        if (typeof value === "number")
            return value < 100000000000 ? value * 1000 : value
        var parsed = Date.parse(String(value || ""))
        return isNaN(parsed) ? 0 : parsed
    }

    Component.onCompleted: root.refreshContent()

    Connections {
        target: HttpHandler
        function onUploadHistoryResult(tasks, error) {
            root.serverUploads = tasks || []
            root.loadError = error || ""
        }
    }

    function forwardRecord(t) {
        var incoming = t.targetDeviceId === DeviceManager.deviceId
        return {
            title: (t.files && t.files.length === 1) ? String(t.files[0].path || "").split(/[\\/]/).pop()
                                                        : (t.files ? t.files.length + qsTr(" 个文件") : "-"),
            direction: incoming ? "↓" : "↑",
            peer: incoming ? qsTr("接收自 ") + root.deviceName(t.sourceDeviceId)
                           : qsTr("发送到 ") + root.deviceName(t.targetDeviceId),
            channel: t.channel || "RELAY",
            channelText: t.channel || "RELAY",
            totalBytes: t.totalBytes || 0,
            state: t.state,
            failureReason: t.failureReason || "",
            updatedAt: t.updatedAt || t.createdAt || ""
        }
    }

    function uploadRecord(t) {
        var title = t.directoryName || String(t.targetPath || "").split(/[\\/]/).pop() || "-"
        if (t.files && t.files.length === 1) title = String(t.files[0].path || title).split(/[\\/]/).pop()
        return {
            title: title,
            direction: "↑",
            peer: qsTr("上传到服务器 / ") + (t.targetPath || t.parentPath || qsTr("根目录")),
            channel: "RELAY",
            channelText: qsTr("服务器"),
            totalBytes: t.totalBytes || 0,
            state: t.state,
            failureReason: t.failureReason || "",
            updatedAt: t.updatedAt || t.createdAt || ""
        }
    }

    readonly property var records: {
        var rows = []
        var forwards = ForwardManager.historyTasks.filter(function(t) {
            return t.state === "COMPLETED" || t.state === "FAILED" || t.state === "CANCELLED" || t.state === "REJECTED"
        })
        var uploads = root.serverUploads.filter(function(t) {
            var target = String(t.targetPath || "")
            var relayStaging = target === "relay" || target.indexOf("relay/") === 0
            return !relayStaging && (t.state === "COMPLETED" || t.state === "FAILED" || t.state === "CANCELLED")
        })
        for (var i = 0; i < forwards.length; ++i) rows.push(root.forwardRecord(forwards[i]))
        for (var j = 0; j < uploads.length; ++j) rows.push(root.uploadRecord(uploads[j]))
        rows.sort(function(a, b) { return root.timestampValue(b.updatedAt) - root.timestampValue(a.updatedAt) })
        return rows
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 2
                Text { text: qsTr("传输记录"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
                Text { text: qsTr("服务器上传与客户端收发统一归档，进行中的任务不会出现在这里"); font.pixelSize: 11; color: Theme.muted }
            }
            Item { Layout.fillWidth: true }
            UiButton { text: qsTr("刷新"); enabled: !ForwardManager.busy; onClicked: root.refreshContent() }
        }

        UiNotice {
            Layout.fillWidth: true
            message: root.loadError
            tone: "error"
            onDismissed: root.loadError = ""
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surface
            border.width: 1; border.color: Theme.line

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 0

                // header row
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 9
                    spacing: 12
                    Text { Layout.fillWidth: true;    text: qsTr("文件");  font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.3; color: Theme.faint }
                    Text { Layout.preferredWidth: 128; text: qsTr("设备"); font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.3; color: Theme.faint }
                    Text { Layout.preferredWidth: 58;  text: qsTr("通道"); font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.3; color: Theme.faint }
                    Text { Layout.preferredWidth: 72;  text: qsTr("大小"); font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.3; color: Theme.faint }
                    Text { Layout.preferredWidth: 76;  text: qsTr("结果"); font.family: Theme.dataFont; font.pixelSize: 10; font.letterSpacing: 1.3; color: Theme.faint }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.line }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.records

                    delegate: Item {
                        id: row
                        required property var modelData
                        readonly property bool ok: modelData.state === "COMPLETED"
                        width: ListView.view.width
                        height: 52

                        RowLayout {
                            anchors.fill: parent
                            spacing: 12
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 9
                                Text {
                                    text: row.modelData.direction
                                    font.pixelSize: 12; font.bold: true
                                    color: Theme.ink2
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: row.modelData.title
                                    font.pixelSize: 13; font.weight: Font.Medium
                                    color: Theme.ink
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                Layout.preferredWidth: 128
                                text: row.modelData.peer
                                font.family: Theme.dataFont; font.pixelSize: 11
                                color: Theme.muted
                                elide: Text.ElideRight
                            }
                            UiChip {
                                Layout.preferredWidth: 58
                                channel: row.modelData.channel || "RELAY"
                                text: row.modelData.channelText
                            }
                            Text {
                                Layout.preferredWidth: 72
                                text: Theme.formatBytes(row.modelData.totalBytes)
                                font.family: Theme.dataFont; font.pixelSize: 11
                                color: Theme.muted
                            }
                            Text {
                                Layout.preferredWidth: 76
                                text: row.ok ? qsTr("校验通过")
                                    : row.modelData.state === "FAILED" ? qsTr("失败")
                                    : row.modelData.state === "REJECTED" ? qsTr("已拒绝") : qsTr("已取消")
                                font.family: Theme.dataFont; font.pixelSize: 11
                                color: row.ok ? Theme.signalDeep
                                     : row.modelData.state === "FAILED" ? Theme.alert : Theme.faint
                            }
                        }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.lineSoft }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: root.records.length === 0
                        text: qsTr("还没有传输记录")
                        font.pixelSize: 13
                        color: Theme.faint
                    }
                }
            }
        }
    }
}
