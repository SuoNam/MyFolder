pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    function refreshContent() {
        ForwardManager.refreshTasks()
        for (var i = 0; i < root.activeUploads.length; ++i) {
            if (root.activeUploads[i].uploadId)
                TransferManager.queryTaskStatus(root.activeUploads[i].uploadId)
        }
    }
    signal acceptTask(var task)

    function failureInChinese(reason, fallback) {
        var raw = String(reason || "").trim()
        if (raw.length === 0) return fallback || qsTr("传输失败，未收到具体原因")
        var upper = raw.toUpperCase()
        if (upper.indexOf("HASH") >= 0 || upper.indexOf("SHA-256") >= 0) return qsTr("文件校验失败，文件内容可能已发生变化")
        if (upper.indexOf("TIMEOUT") >= 0 || upper.indexOf("TIMED OUT") >= 0) return qsTr("连接超时，对方设备可能离线或网络不可达")
        if (upper.indexOf("SOURCE_CHANGED") >= 0 || upper.indexOf("LOCAL SOURCE CHANGED") >= 0) return qsTr("源文件在传输过程中发生了变化")
        if (upper.indexOf("UNAUTHORIZED") >= 0 || upper.indexOf("AUTH") >= 0) return qsTr("身份验证失败，请重新登录后重试")
        if (upper.indexOf("PERMISSION") >= 0 || upper.indexOf("ACCESS DENIED") >= 0) return qsTr("没有读取源文件或写入目标目录的权限")
        if (upper.indexOf("STORAGE") >= 0 || upper.indexOf("DISK") >= 0 || upper.indexOf("NO SPACE") >= 0) return qsTr("目标设备存储空间不足或无法写入文件")
        if (upper.indexOf("NETWORK") >= 0 || upper.indexOf("CONNECTION") >= 0 || upper.indexOf("CHANNEL") >= 0) return qsTr("网络连接中断，无法继续传输")
        if (upper.indexOf("NOT FOUND") >= 0 || upper.indexOf("NO SUCH FILE") >= 0) return qsTr("源文件或传输任务已经不存在")
        if (upper.indexOf("REJECT") >= 0) return qsTr("对方设备拒绝接收文件")
        if (upper.indexOf("CANCEL") >= 0) return qsTr("传输已取消")
        if (/^[A-Z0-9_:\- ]+$/.test(raw)) return qsTr("传输失败，错误代码：") + raw
        return raw
    }

    function forwardRow(t) {
        var incoming = t.targetDeviceId === DeviceManager.deviceId
        var total = t.totalBytes || 0
        var moved = t.transferredBytes || 0
        var automatic = incoming && (ForwardManager.isAutoAccepting(t.forwardId)
                     || GlobalStatus.autoAcceptDeviceTransfers
                     || ForwardManager.canAutoAcceptFallback(t))
        if (t.channel === "LAN") {
            var localLan = LanTransferManager.outgoingProgress[t.forwardId]
            if (!incoming && localLan !== undefined)
                moved = Number(localLan)
        } else if (t.channel === "P2P") {
            var localP2p = incoming
                    ? P2pTransferManager.incomingVerifiedProgress[t.forwardId]
                    : P2pTransferManager.outgoingProgress[t.forwardId]
            if (localP2p !== undefined)
                moved = Number(localP2p)
        }
        return {
            title: (t.files && t.files.length === 1)
                   ? String(t.files[0].path || "").split(/[\\/]/).pop()
                   : (t.files ? t.files.length + qsTr(" 个文件") : "-"),
            peer: incoming ? t.sourceDeviceId : t.targetDeviceId,
            sizeText: Theme.formatBytes(total),
            channel: t.channel || "RELAY",
            state: t.state,
            automaticAcceptance: automatic,
            statusText: automatic ? qsTr("正在自动切换通道")
                                  : (!incoming && t.state === "OFFERED" ? qsTr("等待对方接收") : ""),
            progress: total > 0 ? moved / total : 0,
            transferredBytes: moved,
            totalBytes: total,
            incoming: incoming,
            serverUpload: false,
            canRetry: incoming,
            failureReason: t.state === "FAILED" ? root.failureInChinese(t.failureReason, qsTr("传输失败，服务端未返回具体原因")) : "",
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
            transferredBytes: t.uploadedBytes || 0,
            totalBytes: t.totalBytes || 0,
            incoming: false,
            serverUpload: true,
            canRetry: true,
            statusText: t.statusText || "",
            failureReason: t.state === "FAILED" ? root.failureInChinese(t.failureReason, qsTr("上传失败，服务端未返回具体原因")) : "",
            uploadId: t.uploadId
        }
    }

    readonly property var uploadTasks: TransferManager.taskList
    readonly property var activeUploads: uploadTasks.filter(function(t) {
        return t.state !== "COMPLETED" && t.state !== "CANCELLED" && t.state !== "FAILED" && t.state !== "REJECTED"
    })
    readonly property var failedUploads: uploadTasks.filter(function(t) { return t.state === "FAILED" })
    readonly property var activeForwards: ForwardManager.tasks.filter(function(t) {
        return t.state !== "COMPLETED" && t.state !== "CANCELLED" && t.state !== "FAILED" && t.state !== "REJECTED"
    })
    readonly property var failedForwards: ForwardManager.tasks.filter(function(t) { return t.state === "FAILED" })
    readonly property int activeCount: activeForwards.length + activeUploads.length
    readonly property int attentionCount: failedUploads.length + failedForwards.length

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            ColumnLayout {
                spacing: 2
                Text { text: qsTr("正在传输"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
                Text { text: qsTr("这里只显示进行中和需要处理的任务；完成记录统一归档到“传输记录”"); font.pixelSize: 11; color: Theme.muted }
            }
            UiChip {
                visible: root.activeCount > 0
                channel: "LAN"
                text: root.activeCount + qsTr(" 项进行中")
            }
            Item { Layout.fillWidth: true }
            UiButton {
                text: qsTr("刷新")
                enabled: !ForwardManager.busy
                onClicked: root.refreshContent()
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
                    visible: root.activeUploads.length > 0
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
                            Text { text: root.activeUploads.length + qsTr(" 项进行中"); font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.faint }
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }
                        Repeater {
                            model: root.activeUploads
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
                    visible: root.activeForwards.length > 0
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
                                onAcceptRequested: root.acceptTask(modelData)
                                onRejectRequested: ForwardManager.reject(modelData.forwardId)
                                onCancelRequested: ForwardManager.cancel(modelData.forwardId)
                                onPauseRequested: {
                                    if (task.incoming) RelayDownloadManager.cancelTask(modelData.forwardId)
                                }
                            }
                        }

                    }
                }

                Rectangle {
                    visible: root.attentionCount > 0
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
                            text: qsTr("需要处理")
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.ink
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }
                        Repeater {
                            model: root.failedUploads
                            delegate: TransferRow {
                                required property var modelData
                                Layout.fillWidth: true
                                task: root.uploadRow(modelData)
                                onRetryRequested: TransferManager.retryTask(modelData.uploadId)
                                onCancelRequested: TransferManager.cancelTask(modelData.uploadId)
                                onDismissRequested: TransferManager.dismissFailedTask(modelData.uploadId)
                            }
                        }
                        Repeater {
                            model: root.failedForwards
                            delegate: TransferRow {
                                required property var modelData
                                Layout.fillWidth: true
                                task: root.forwardRow(modelData)
                                onRetryRequested: RelayDownloadManager.retryTask(modelData.forwardId)
                                onDismissRequested: ForwardManager.dismissTask(modelData.forwardId)
                            }
                        }
                    }
                }

                Rectangle {
                    visible: root.activeCount === 0 && root.attentionCount === 0
                    Layout.fillWidth: true
                    implicitHeight: 160
                    radius: Theme.radius
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.line
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("当前没有传输任务"); font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink2 }
                        Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("上传文件或从“设备”页发起客户端传输"); font.pixelSize: 11; color: Theme.muted }
                    }
                }
            }
        }
    }
}
