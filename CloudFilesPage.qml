pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Labs

Item {
    id: root

    signal backRequested()
    signal uploadStarted()

    property string scopeType: "PRIVATE"
    property string scopeId: ""
    property string scopeName: qsTr("我的文件")
    property string permission: "OWNER"
    property bool showBackButton: false

    property string currentPath: ""
    property var entries: []
    property var selectedPaths: []
    property bool loading: false
    property bool operationBusy: false
    property string pendingOperation: ""
    property string errorMessage: ""
    property string statusMessage: ""
    property string pendingDownloadPath: ""
    property bool pendingDownloadDirectory: false
    property var renameEntry: ({})
    property string clipboardMode: ""
    property var clipboardEntries: []

    readonly property string normalizedScopeType: scopeType.toUpperCase() === "GROUP" ? "GROUP" : "PRIVATE"
    readonly property bool canWrite: normalizedScopeType !== "GROUP" || permission.toUpperCase() === "OWNER" || permission.toUpperCase() === "WRITE"
    readonly property bool busy: loading || operationBusy
    readonly property int selectedCount: selectedPaths.length

    function resetAndLoad() {
        currentPath = ""
        entries = []
        selectedPaths = []
        clipboardMode = ""
        clipboardEntries = []
        errorMessage = ""
        statusMessage = ""
        loadDirectory("")
    }

    function cleanPath(path) {
        return String(path || "").replace(/^\/+|\/+$/g, "")
    }

    function joinPath(parent, name) {
        var p = cleanPath(parent)
        var n = String(name || "").replace(/^\/+|\/+$/g, "")
        return p.length > 0 ? p + "/" + n : n
    }

    function parentPath(path) {
        var parts = cleanPath(path).split("/").filter(function(part) { return part.length > 0 })
        parts.pop()
        return parts.join("/")
    }

    function validName(name) {
        var clean = String(name || "").trim()
        return clean.length > 0 && clean !== "." && clean !== ".." && clean.indexOf("/") < 0 && clean.indexOf("\\") < 0
    }

    function nameTaken(name, ignorePath) {
        var clean = String(name || "").trim()
        for (var i = 0; i < entries.length; ++i) {
            if (String(entries[i].name || "") === clean && String(entries[i].path || "") !== String(ignorePath || "")) return true
        }
        return false
    }

    function containsName(names, name) {
        var wanted = String(name || "").toLowerCase()
        for (var i = 0; i < names.length; ++i) {
            if (String(names[i] || "").toLowerCase() === wanted) return true
        }
        return false
    }

    function availablePasteName(entry, reservedNames) {
        var original = String(entry.name || "").trim()
        if (!containsName(reservedNames, original)) return original

        var stem = original
        var suffix = ""
        if (!isDirectory(entry)) {
            var dot = original.lastIndexOf(".")
            if (dot > 0) {
                stem = original.substring(0, dot)
                suffix = original.substring(dot)
            }
        }
        var number = 1
        var candidate = ""
        do {
            candidate = stem + " (" + number + ")" + suffix
            number++
        } while (containsName(reservedNames, candidate))
        return candidate
    }

    function clearClipboard() {
        clipboardMode = ""
        clipboardEntries = []
        errorMessage = ""
        statusMessage = qsTr("已取消复制/移动")
    }

    function isDirectory(entry) {
        return String(entry.type || "").toLowerCase() === "directory"
    }

    function isSelected(path) {
        return selectedPaths.indexOf(String(path || "")) >= 0
    }

    function toggleSelection(path) {
        var value = String(path || "")
        var next = selectedPaths.slice(0)
        var index = next.indexOf(value)
        if (index >= 0) next.splice(index, 1)
        else next.push(value)
        selectedPaths = next
    }

    function selectAll() {
        var next = []
        for (var i = 0; i < entries.length; ++i) next.push(String(entries[i].path || ""))
        selectedPaths = next
    }

    function selectedEntryObjects() {
        var out = []
        for (var i = 0; i < entries.length; ++i) {
            if (isSelected(entries[i].path)) out.push(entries[i])
        }
        return out
    }

    function loadDirectory(path) {
        if (normalizedScopeType === "GROUP" && scopeId.length === 0) return
        currentPath = cleanPath(path)
        selectedPaths = []
        loading = true
        errorMessage = ""
        HttpHandler.listScopedDirectory(currentPath, normalizedScopeType, scopeId)
    }

    function startOperation(name) {
        pendingOperation = name
        operationBusy = true
        errorMessage = ""
        statusMessage = ""
    }

    function activateLocalClipboard(mode) {
        var picked = selectedEntryObjects()
        if (picked.length === 0) return
        clipboardEntries = picked
        clipboardMode = mode
        statusMessage = mode === "move" ? qsTr("已剪切 %1 项，请进入目标目录后粘贴").arg(picked.length)
                                              : qsTr("已复制 %1 项，请进入目标目录后粘贴").arg(picked.length)
    }

    function copySelection(mode) {
        if (selectedPaths.length === 0) return
        if (normalizedScopeType === "PRIVATE") {
            groupTransferDialog.present(mode, selectedPaths.slice(0))
            return
        }
        activateLocalClipboard(mode)
    }

    function pasteClipboard() {
        if (!canWrite || clipboardEntries.length === 0 || operationBusy) return
        var operations = []
        var reservedNames = []
        for (var existingIndex = 0; existingIndex < entries.length; ++existingIndex) {
            reservedNames.push(String(entries[existingIndex].name || ""))
        }
        var skippedMoves = 0
        for (var i = 0; i < clipboardEntries.length; ++i) {
            var entry = clipboardEntries[i]
            var source = String(entry.path || "")
            var originalDestination = joinPath(currentPath, entry.name)
            if (isDirectory(entry) && (currentPath === source || currentPath.indexOf(source + "/") === 0)) {
                errorMessage = qsTr("不能把目录复制或移动到它自身内部：") + entry.name
                return
            }
            if (clipboardMode === "move" && originalDestination === source) {
                skippedMoves++
                continue
            }

            var destinationName = availablePasteName(entry, reservedNames)
            reservedNames.push(destinationName)
            operations.push({"from": source, "to": joinPath(currentPath, destinationName)})
        }
        if (operations.length === 0) {
            if (skippedMoves > 0) {
                clipboardMode = ""
                clipboardEntries = []
                errorMessage = ""
                statusMessage = qsTr("所选项目已经位于当前目录")
            }
            return
        }
        startOperation(clipboardMode)
        if (clipboardMode === "move") HttpHandler.moveScopedPaths(operations, normalizedScopeType, scopeId)
        else HttpHandler.copyScopedPaths(operations, normalizedScopeType, scopeId)
    }

    function requestDownload(entry) {
        pendingDownloadPath = String(entry.path || "")
        pendingDownloadDirectory = isDirectory(entry)
        downloadDirectoryDialog.open()
    }

    function formatSize(bytes) {
        var value = Number(bytes || 0)
        if (value < 1024) return value + " B"
        if (value < 1024 * 1024) return (value / 1024).toFixed(1) + " KB"
        if (value < 1024 * 1024 * 1024) return (value / 1024 / 1024).toFixed(1) + " MB"
        return (value / 1024 / 1024 / 1024).toFixed(1) + " GB"
    }

    function formatDate(value) {
        var date = new Date(value)
        return isNaN(date.getTime()) ? "" : Qt.formatDateTime(date, "yyyy-MM-dd HH:mm")
    }

    Labs.FileDialog {
        id: uploadDialog
        title: qsTr("选择要上传的文件")
        fileMode: Labs.FileDialog.OpenFiles
        onAccepted: {
            var selected = 0
            var created = 0
            for (var i = 0; i < files.length; ++i) {
                var localPath = ShellIntegration.localFilePath(files[i])
                if (localPath.length === 0) continue
                selected++
                var taskId = TransferManager.startScopedFileUpload(localPath, root.currentPath,
                                                                   root.normalizedScopeType, root.scopeId)
                if (taskId && taskId.length > 0) created++
            }
            if (created > 0 && created === selected) {
                root.statusMessage = created === 1
                        ? qsTr("上传任务已创建，可在“传输”页查看进度")
                        : qsTr("已创建 %1 个上传任务，可在“传输”页查看进度").arg(created)
                root.errorMessage = ""
                root.uploadStarted()
            } else {
                root.errorMessage = created > 0
                        ? qsTr("已创建 %1 个任务，另有 %2 个文件无法加入").arg(created).arg(selected - created)
                        : qsTr("无法创建上传任务，请检查文件路径和目标目录")
            }
        }
    }

    Labs.FolderDialog {
        id: downloadDirectoryDialog
        title: qsTr("选择下载目录")
        onAccepted: {
            var localFolder = ShellIntegration.localFilePath(folder)
            if (localFolder.length === 0) {
                root.errorMessage = qsTr("无法读取所选下载目录")
                return
            }
            if (root.pendingDownloadDirectory) {
                HttpHandler.downloadScopedDirectory(root.pendingDownloadPath, localFolder,
                                                    root.normalizedScopeType, root.scopeId)
            } else {
                HttpHandler.downloadScoped(root.pendingDownloadPath, localFolder,
                                           root.normalizedScopeType, root.scopeId)
            }
        }
    }

    UiPromptDialog {
        id: newFolderDialog
        dialogTitle: qsTr("新建文件夹")
        description: qsTr("在当前目录创建一个新的文件夹")
        placeholderText: qsTr("文件夹名称")
        confirmText: qsTr("创建")

        onConfirmRequested: function(value) {
            var name = String(value || "").trim()
            if (!root.validName(name)) {
                errorText = qsTr("名称不能为空，也不能包含 / 或 \\")
                return
            }
            if (root.nameTaken(name, "")) {
                errorText = qsTr("当前目录已经存在同名项目")
                return
            }
            close()
            root.startOperation("createFolder")
            HttpHandler.createScopedFolder(root.joinPath(root.currentPath, name), root.normalizedScopeType, root.scopeId)
        }
    }

    GroupTransferDialog {
        id: groupTransferDialog
        onPrivateRequested: function(mode) { root.activateLocalClipboard(mode) }
        onGroupConfirmed: function(mode, sourcePaths, groupId, targetDirectory) {
            var operation = mode === "copy" ? "copyToGroup" : "moveToGroup"
            root.startOperation(operation)
            HttpHandler.transferPrivatePathsToGroup(mode, sourcePaths, groupId, targetDirectory)
        }
    }

    UiPromptDialog {
        id: renameDialog
        dialogTitle: qsTr("重命名")
        description: qsTr("输入这个项目的新名称")
        placeholderText: qsTr("新名称")
        confirmText: qsTr("保存")

        onConfirmRequested: function(value) {
            var name = String(value || "").trim()
            if (!root.validName(name)) {
                errorText = qsTr("名称不能为空，也不能包含 / 或 \\")
                return
            }
            if (root.nameTaken(name, root.renameEntry.path)) {
                errorText = qsTr("当前目录已经存在同名项目")
                return
            }
            var destination = root.joinPath(root.currentPath, name)
            if (destination === root.renameEntry.path) {
                close()
                return
            }
            close()
            root.startOperation("move")
            HttpHandler.moveScopedPaths([{"from": root.renameEntry.path, "to": destination}], root.normalizedScopeType, root.scopeId)
        }
    }

    UiPromptDialog {
        id: deleteDialog
        dialogTitle: qsTr("确认删除")
        description: qsTr("将永久删除选中的 %1 项，文件夹及其内容也会被删除。此操作不可撤销。").arg(root.selectedCount)
        confirmText: qsTr("永久删除")
        showInput: false
        danger: true

        onConfirmRequested: function(value) {
            if (root.selectedPaths.length === 0) {
                close()
                return
            }
            close()
            root.startOperation("delete")
            HttpHandler.deleteScopedPaths(root.selectedPaths, root.normalizedScopeType, root.scopeId)
        }
    }
    Connections {
        target: HttpHandler
        function onScopedDirectoryListed(scopeType, scopeId, directoryPath, result, error) {
            if (scopeType !== root.normalizedScopeType || scopeId !== root.scopeId || directoryPath !== root.currentPath) return
            root.loading = false
            if (!error)
                root.entries = result || []
            root.errorMessage = error || ""
        }
        function onScopedDownloadFinished(scopeType, scopeId, success, message, savedPath) {
            if (scopeType !== root.normalizedScopeType || scopeId !== root.scopeId) return
            root.statusMessage = success ? message + "：" + savedPath : ""
            root.errorMessage = success ? "" : message
        }
        function onScopedOperationFinished(scopeType, scopeId, operation, success, message, results) {
            if (scopeType !== root.normalizedScopeType || scopeId !== root.scopeId || operation !== root.pendingOperation) return
            root.operationBusy = false
            root.pendingOperation = ""
            if (success) {
                root.statusMessage = message || qsTr("操作完成")
                root.errorMessage = ""
                if (operation === "move") {
                    root.clipboardMode = ""
                    root.clipboardEntries = []
                }
                if (operation === "copyToGroup" || operation === "moveToGroup") {
                    root.selectedPaths = []
                }
            } else {
                root.errorMessage = message || qsTr("操作失败")
                root.statusMessage = ""
            }
            root.loadDirectory(root.currentPath)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            UiButton {
                visible: root.showBackButton
                kind: "quiet"
                text: "‹  " + qsTr("返回群组")
                onClicked: root.backRequested()
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: root.scopeName
                        color: Theme.ink
                        font.family: Theme.uiFont
                        font.pixelSize: 22
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    UiChip {
                        visible: root.normalizedScopeType === "GROUP"
                        channel: root.canWrite ? "LAN" : "idle"
                        text: root.canWrite ? qsTr("可读写") : qsTr("只读")
                    }
                    Item { Layout.fillWidth: true }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.normalizedScopeType === "GROUP"
                          ? qsTr("群组共享文件空间")
                          : qsTr("Workspace · 服务器上的个人文件")
                    color: Theme.muted
                    font.family: Theme.uiFont
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            UiButton {
                visible: root.canWrite
                text: qsTr("新建文件夹")
                enabled: !root.busy
                onClicked: newFolderDialog.present("")
            }
            UiButton {
                visible: root.canWrite
                kind: "primary"
                text: qsTr("上传文件")
                enabled: !root.busy
                onClicked: uploadDialog.open()
            }
            UiButton {
                kind: "quiet"
                text: qsTr("刷新")
                enabled: !root.busy
                onClicked: root.loadDirectory(root.currentPath)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.line

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 9
                spacing: 7

                Rectangle {
                    width: 28
                    height: 28
                    radius: 7
                    color: Theme.signalWash
                    border.width: 1
                    border.color: Theme.signalEdge

                    Text {
                        anchors.centerIn: parent
                        text: "⌂"
                        color: Theme.signalDeep
                        font.family: Theme.uiFont
                        font.pixelSize: 15
                        font.bold: true
                    }

                    TapHandler { onTapped: root.loadDirectory("") }
                }

                UiButton {
                    visible: root.currentPath.length > 0
                    kind: "quiet"
                    text: "‹  " + qsTr("上一级")
                    implicitHeight: 28
                    pixelSize: 11
                    onClicked: root.loadDirectory(root.parentPath(root.currentPath))
                }

                Rectangle {
                    visible: root.currentPath.length > 0
                    width: 1
                    height: 20
                    color: Theme.lineSoft
                }

                Text {
                    Layout.fillWidth: true
                    text: root.currentPath.length > 0 ? "/ " + root.currentPath : qsTr("根目录")
                    color: root.currentPath.length > 0 ? Theme.ink2 : Theme.muted
                    font.family: root.currentPath.length > 0 ? Theme.dataFont : Theme.uiFont
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                }

                UiChip {
                    visible: root.clipboardEntries.length > 0
                    channel: "LAN"
                    text: (root.clipboardMode === "move" ? qsTr("待移动") : qsTr("待复制"))
                          + " · " + root.clipboardEntries.length
                }

                UiButton {
                    visible: root.canWrite && root.clipboardEntries.length > 0
                    kind: "primary"
                    text: qsTr("粘贴到此处")
                    implicitHeight: 28
                    pixelSize: 11
                    enabled: !root.busy
                    onClicked: root.pasteClipboard()
                }
                UiButton {
                    visible: root.clipboardEntries.length > 0
                    kind: "quiet"
                    text: qsTr("取消")
                    implicitHeight: 28
                    pixelSize: 11
                    enabled: !root.busy
                    onClicked: root.clearClipboard()
                }
            }
        }

        Rectangle {
            visible: root.selectedCount > 0
            Layout.fillWidth: true
            implicitHeight: 46
            radius: Theme.radius
            color: Theme.signalWash
            border.width: 1
            border.color: Theme.signalEdge

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 7

                Rectangle {
                    width: 26
                    height: 26
                    radius: 7
                    color: Theme.signal
                    Text {
                        anchors.centerIn: parent
                        text: root.selectedCount
                        color: "#FFFFFF"
                        font.family: Theme.dataFont
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
                Text {
                    text: qsTr("已选择项目")
                    color: Theme.signalDeep
                    font.family: Theme.uiFont
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Item { Layout.fillWidth: true }
                UiButton {
                    text: qsTr("下载")
                    implicitHeight: 30
                    pixelSize: 11
                    enabled: root.selectedCount === 1 && !root.busy
                    onClicked: root.requestDownload(root.selectedEntryObjects()[0])
                }
                UiButton {
                    visible: root.canWrite
                    text: qsTr("复制")
                    implicitHeight: 30
                    pixelSize: 11
                    enabled: !root.busy
                    onClicked: root.copySelection("copy")
                }
                UiButton {
                    visible: root.canWrite
                    text: qsTr("移动")
                    implicitHeight: 30
                    pixelSize: 11
                    enabled: !root.busy
                    onClicked: root.copySelection("move")
                }
                UiButton {
                    visible: root.canWrite
                    kind: "danger"
                    text: qsTr("删除")
                    implicitHeight: 30
                    pixelSize: 11
                    enabled: !root.busy
                    onClicked: deleteDialog.present("")
                }
                UiButton {
                    kind: "quiet"
                    text: qsTr("取消选择")
                    implicitHeight: 30
                    pixelSize: 11
                    onClicked: root.selectedPaths = []
                }
            }
        }

        UiNotice {
            Layout.fillWidth: true
            message: root.errorMessage.length > 0 ? root.errorMessage : root.statusMessage
            tone: root.errorMessage.length > 0 ? "error" : "normal"
            onDismissed: { root.errorMessage = ""; root.statusMessage = "" }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 2
            Layout.rightMargin: 2
            spacing: 8

            UiCheckBox {
                id: selectAllBox
                text: root.selectedCount > 0 && root.selectedCount < root.entries.length
                      ? qsTr("已选 %1 项").arg(root.selectedCount)
                      : qsTr("全选")
                tristate: true
                enabled: root.entries.length > 0 && !root.busy
                onClicked: root.selectedCount === root.entries.length ? root.selectedPaths = [] : root.selectAll()
                Binding {
                    target: selectAllBox
                    property: "checkState"
                    value: root.entries.length === 0 || root.selectedCount === 0 ? Qt.Unchecked
                         : root.selectedCount === root.entries.length ? Qt.Checked
                         : Qt.PartiallyChecked
                }
            }

            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("%1 个项目").arg(root.entries.length)
                color: Theme.muted
                font.family: Theme.uiFont
                font.pixelSize: 10
            }
        }

        ListView {
            id: fileList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: root.entries
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: entryRow
                required property var modelData
                readonly property bool directory: root.isDirectory(modelData)
                readonly property string path: String(modelData.path || "")
                width: fileList.width
                height: 62
                radius: Theme.radius
                color: root.isSelected(path) ? Theme.signalWash
                     : rowHover.hovered ? Theme.sunken : Theme.surface
                border.width: 1
                border.color: root.isSelected(path) ? Theme.signalEdge
                            : rowHover.hovered ? "#CBD8D1" : Theme.line

                Behavior on color { ColorAnimation { duration: 100 } }
                HoverHandler { id: rowHover }

                Rectangle {
                    visible: root.isSelected(entryRow.path)
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 3
                    radius: 2
                    color: Theme.signal
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 9
                    spacing: 10

                    UiCheckBox {
                        id: entryCheck
                        Layout.preferredWidth: 18
                        boxSize: 17
                        enabled: !root.busy
                        onClicked: root.toggleSelection(entryRow.path)
                        Binding {
                            target: entryCheck
                            property: "checked"
                            value: root.isSelected(entryRow.path)
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        RowLayout {
                            anchors.fill: parent
                            spacing: 10

                            Rectangle {
                                width: 36
                                height: 36
                                radius: 9
                                color: entryRow.directory ? Theme.signalWash : Theme.sunken
                                border.width: 1
                                border.color: entryRow.directory ? Theme.signalEdge : Theme.line

                                Item {
                                    anchors.centerIn: parent
                                    width: 20
                                    height: 20

                                    Rectangle {
                                        visible: entryRow.directory
                                        x: 1
                                        y: 5
                                        width: 18
                                        height: 13
                                        radius: 3
                                        color: Theme.signal
                                    }
                                    Rectangle {
                                        visible: entryRow.directory
                                        x: 2
                                        y: 2
                                        width: 9
                                        height: 7
                                        radius: 2
                                        color: Theme.signal
                                    }

                                    Rectangle {
                                        visible: !entryRow.directory
                                        x: 3
                                        y: 1
                                        width: 14
                                        height: 18
                                        radius: 2
                                        color: Theme.surface
                                        border.width: 1
                                        border.color: Theme.muted
                                    }
                                    Rectangle {
                                        visible: !entryRow.directory
                                        x: 6
                                        y: 7
                                        width: 8
                                        height: 1
                                        color: Theme.muted
                                    }
                                    Rectangle {
                                        visible: !entryRow.directory
                                        x: 6
                                        y: 11
                                        width: 7
                                        height: 1
                                        color: Theme.faint
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3

                                Text {
                                    Layout.fillWidth: true
                                    text: String(entryRow.modelData.name || entryRow.path)
                                    color: Theme.ink
                                    font.family: Theme.uiFont
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: entryRow.directory
                                          ? qsTr("文件夹")
                                          : root.formatSize(entryRow.modelData.size)
                                            + (root.formatDate(entryRow.modelData.lastModified).length > 0
                                               ? "  ·  " + root.formatDate(entryRow.modelData.lastModified) : "")
                                    color: Theme.muted
                                    font.family: Theme.uiFont
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        TapHandler {
                            enabled: !root.busy
                            acceptedButtons: Qt.LeftButton
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: {
                                if (entryRow.directory) root.loadDirectory(entryRow.path)
                                else root.toggleSelection(entryRow.path)
                            }
                        }
                    }

                    UiButton {
                        visible: root.canWrite
                        text: qsTr("重命名")
                        implicitHeight: 30
                        pixelSize: 11
                        enabled: !root.busy
                        onClicked: {
                            root.renameEntry = entryRow.modelData
                            renameDialog.present(String(entryRow.modelData.name || ""))
                        }
                    }
                    UiButton {
                        text: qsTr("下载")
                        implicitHeight: 30
                        pixelSize: 11
                        enabled: !root.busy
                        onClicked: root.requestDownload(entryRow.modelData)
                    }
                    UiButton {
                        visible: entryRow.directory
                        kind: "primary"
                        text: qsTr("打开")
                        implicitHeight: 30
                        pixelSize: 11
                        enabled: !root.busy
                        onClicked: root.loadDirectory(entryRow.path)
                    }
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                visible: root.busy
                running: visible
                palette.dark: Theme.signal
            }

            ColumnLayout {
                anchors.centerIn: parent
                visible: !root.busy && root.errorMessage.length === 0 && root.entries.length === 0
                spacing: 9

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 14
                    color: Theme.signalWash
                    border.width: 1
                    border.color: Theme.signalEdge

                    Item {
                        anchors.centerIn: parent
                        width: 25
                        height: 22
                        Rectangle { x: 1; y: 6; width: 23; height: 15; radius: 4; color: Theme.signal }
                        Rectangle { x: 3; y: 2; width: 11; height: 8; radius: 3; color: Theme.signal }
                    }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("当前目录为空")
                    color: Theme.ink
                    font.family: Theme.uiFont
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.canWrite ? qsTr("上传文件或新建文件夹开始整理") : qsTr("这里还没有可查看的文件")
                    color: Theme.muted
                    font.family: Theme.uiFont
                    font.pixelSize: 11
                }
            }
        }
    }
}
