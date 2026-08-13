pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    function forwardRow(t) {
        var incoming = t.targetDeviceId === DeviceManager.deviceId
        var total = t.totalBytes || 0
        var moved = t.transferredBytes || 0
        return {
            title: (t.files && t.files.length === 1)
                   ? String(t.files[0].path || "").split(/[\\/]/).pop()
                   : (t.files ? t.files.length + qsTr(" 个文件") : "-"),
            peer: incoming ? t.sourceDeviceId : t.targetDeviceId,
            sizeText: Theme.formatBytes(total),
            channel: t.channel || "RELAY",
            state: t.state,
            progress: total > 0 ? moved / total : 0,
            incoming: incoming,
            serverUpload: false,
            forwardId: t.forwardId
        }
    }

    function uploadRow(t) {
        var title = t.directoryName || "-"
        if (t.files && t.files.length === 1)
            title = String(t.files[0].path || title).split(/[\\/]/).pop()
        return {
            title: title,
            peer: qsTr("服务器 / ") + (t.targetPath || t.parentPath || ""),
            sizeText: Theme.formatBytes(t.totalBytes || 0),
            channel: "RELAY",
            state: t.state,
            progress: t.progress || 0,
            incoming: false,
            serverUpload: true,
            uploadId: t.uploadId
        }
    }

    readonly property var uploadTasks: TransferManager.taskList
    readonly property var activeForwards: ForwardManager.tasks.filter(function(t) {
        return t.state !== "COMPLETED" && t.state !== "CANCELLED" && t.state !== "FAILED"
    })
    readonly property var settledForwards: ForwardManager.tasks.filter(function(t) {
        return t.state === "COMPLETED" || t.state === "FAILED"
    })
    readonly property int activeCount: activeForwards.length + uploadTasks.filter(function(t) {
        return t.state !== "COMPLETED" && t.state !== "CANCELLED" && t.state !== "FAILED"
    }).length

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: qsTr("传输"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
            UiChip {
                visible: root.activeCount > 0
                channel: "LAN"
                text: root.activeCount + qsTr(" 项进行中")
            }
            Item { Layout.fillWidth: true }
            UiButton {
                text: qsTr("刷新")
                enabled: !ForwardManager.busy
                onClicked: {
                    ForwardManager.refreshTasks()
                    for (var i = 0; i < root.uploadTasks.length; ++i) {
                        if (root.uploadTasks[i].uploadId)
                            TransferManager.queryTaskStatus(root.uploadTasks[i].uploadId)
                    }
                }
            }
        }

        ScrollView {
            id: transfersScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: transfersScroll.availableWidth
                spacing: 12

                Rectangle {
                    visible: root.uploadTasks.length > 0
                    Layout.fillWidth: true
                    implicitHeight: uploadCol.implicitHeight
                    radius: Theme.radius
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.line

                    ColumnLayout {
                        id: uploadCol
                        width: parent.width
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.margins: 14
                            Layout.bottomMargin: 10
                            Text { text: qsTr("上传到服务器"); font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink }
                            Item { Layout.fillWidth: true }
                            UiButton { text: qsTr("清除已完成"); kind: "quiet"; onClicked: TransferManager.clearCompletedTasks() }
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }
                        Repeater {
                            model: root.uploadTasks
                            delegate: TransferRow {
                                required property var modelData
                                Layout.fillWidth: true
                                task: root.uploadRow(modelData)
                                onPauseRequested: {
                                    if (modelData.paused) TransferManager.resumeTask(modelData.uploadId)
                                    else TransferManager.pauseTask(modelData.uploadId)
                                }
                                onCancelRequested: TransferManager.cancelTask(modelData.uploadId)
                                onRetryRequested: TransferManager.retryTask(modelData.uploadId)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: forwardCol.implicitHeight
                    radius: Theme.radius
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.line

                    ColumnLayout {
                        id: forwardCol
                        width: parent.width
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.margins: 14
                            Layout.bottomMargin: 10
                            Text { text: qsTr("客户端间发送"); font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink }
                            Item { Layout.fillWidth: true }
                            Text { text: qsTr("自动选择 LAN / P2P / RELAY"); font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.faint }
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

                        Repeater {
                            model: root.activeForwards
                            delegate: TransferRow {
                                required property var modelData
                                Layout.fillWidth: true
                                task: root.forwardRow(modelData)
                                onAcceptRequested: ForwardManager.accept(modelData.forwardId)
                                onCancelRequested: ForwardManager.cancel(modelData.forwardId)
                                onPauseRequested: {
                                    if (task.incoming) RelayDownloadManager.cancelTask(modelData.forwardId)
                                }
                            }
                        }

                        ColumnLayout {
                            visible: root.activeForwards.length === 0
                            Layout.fillWidth: true
                            Layout.margins: 30
                            spacing: 6
                            Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("没有正在进行的客户端传输"); font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.ink2 }
                            Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("在“设备”页选择一台客户端发送文件"); font.pixelSize: 12; color: Theme.muted }
                        }
                    }
                }

                Rectangle {
                    visible: root.settledForwards.length > 0
                    Layout.fillWidth: true
                    implicitHeight: settledCol.implicitHeight
                    radius: Theme.radius
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.line

                    ColumnLayout {
                        id: settledCol
                        width: parent.width
                        spacing: 0
                        Text {
                            Layout.margins: 14
                            Layout.bottomMargin: 10
                            text: qsTr("最近的客户端传输")
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.ink
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }
                        Repeater {
                            model: root.settledForwards
                            delegate: TransferRow {
                                required property var modelData
                                Layout.fillWidth: true
                                task: root.forwardRow(modelData)
                                onRetryRequested: RelayDownloadManager.retryTask(modelData.forwardId)
                            }
                        }
                    }
                }
            }
        }
    }
}
