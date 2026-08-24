pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(500, parent ? parent.width - 40 : 500)
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var localFilePaths: []
    property string currentPath: ""
    property var directories: []
    property var groups: []
    property var selectedGroup: null
    property string targetScopeType: "PRIVATE"
    property string targetScopeId: ""
    property bool choosingGroup: false
    property bool browsing: false
    property bool loading: false
    property string errorMessage: ""
    signal uploadConfirmed(var filePaths, string serverPath, string scopeType, string scopeId)

    function fileName() {
        if (localFilePaths.length === 0) return qsTr("未选择文件")
        if (localFilePaths.length === 1) return String(localFilePaths[0]).split(/[\\/]/).pop()
        return qsTr("已选择 %1 个文件").arg(localFilePaths.length)
    }
    function openFor(filePath) {
        openForPaths([filePath])
    }
    function openForPaths(filePaths) {
        localFilePaths = filePaths || []
        currentPath = ""
        directories = []
        groups = []
        selectedGroup = null
        targetScopeType = "PRIVATE"
        targetScopeId = ""
        choosingGroup = false
        browsing = false
        loading = true
        errorMessage = ""
        open()
        HttpHandler.loadGroups()
    }
    function loadDirectory(path) {
        currentPath = path
        loading = true
        errorMessage = ""
        HttpHandler.listScopedDirectory(path, targetScopeType, targetScopeId)
    }
    function enterDirectory(entry) {
        var path = String(entry.path || entry.name || "")
        loadDirectory(path.replace(/^\/+|\/+$/g, ""))
    }
    function parentPath(path) {
        var parts = String(path).split("/").filter(function(p) { return p.length > 0 })
        parts.pop()
        return parts.join("/")
    }

    Overlay.modal: Rectangle { color: "#520E211A" }
    background: Rectangle {
        radius: 12
        color: Theme.surface
        border.width: 1
        border.color: "#C3D2CB"
    }

    Connections {
        target: HttpHandler
        function onGroupsResult(response) {
            if (!root.visible) return
            root.loading = false
            if (response.status !== 200) return
            var writable = []
            var source = response.data || []
            for (var i = 0; i < source.length; ++i) {
                var permission = String(source[i].myPermission || "").toUpperCase()
                if (permission === "OWNER" || permission === "WRITE") writable.push(source[i])
            }
            root.groups = writable
        }
        function onScopedDirectoryListed(scopeType, scopeId, directoryPath, entries, error) {
            if (!root.visible || scopeType !== root.targetScopeType || scopeId !== root.targetScopeId
                    || directoryPath !== root.currentPath) return
            root.loading = false
            root.errorMessage = error
            root.directories = entries.filter(function(item) {
                var type = String(item.type || "").toLowerCase()
                return type === "directory" || type === "folder"
            })
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18; Layout.rightMargin: 12
            Layout.topMargin: 15; Layout.bottomMargin: 14
            spacing: 10
            Rectangle {
                width: 34; height: 34; radius: 9; color: Theme.signalWash
                Text { anchors.centerIn: parent; text: "↑"; font.pixelSize: 17; font.bold: true; color: Theme.signalDeep }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 2
                Text { text: qsTr("发送到 MyFolder 服务器"); font.pixelSize: 15; font.bold: true; color: Theme.ink }
                Text {
                    Layout.fillWidth: true; text: root.fileName(); elide: Text.ElideMiddle
                    font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.muted
                }
            }
            ToolButton {
                text: "×"; implicitWidth: 30; implicitHeight: 30
                onClicked: root.close()
                contentItem: Text { text: parent.text; color: Theme.muted; font.pixelSize: 17; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { radius: 6; color: parent.hovered ? Theme.sunken : "transparent" }
            }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

        ColumnLayout {
            visible: !root.browsing
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 10
            Text { text: qsTr("选择文件在服务器上的保存位置"); color: Theme.ink2; font.pixelSize: 12 }

            DestinationOption {
                title: qsTr("发送到默认位置")
                description: qsTr("保存至服务器默认目录  /inbox")
                symbol: "⌂"
                onChosen: { root.uploadConfirmed(root.localFilePaths, "inbox", "PRIVATE", ""); root.close() }
            }
            DestinationOption {
                title: qsTr("选择指定位置")
                description: qsTr("逐级浏览服务器文件夹")
                symbol: "▣"
                onChosen: {
                    root.targetScopeType = "PRIVATE"; root.targetScopeId = ""
                    root.selectedGroup = null; root.choosingGroup = false
                    root.browsing = true; root.loadDirectory("")
                }
            }
            DestinationOption {
                title: qsTr("上传至群组")
                description: root.groups.length > 0
                             ? qsTr("选择有写权限的群组及目标文件夹")
                             : qsTr("当前没有可写群组")
                symbol: "♧"
                enabled: root.groups.length > 0
                opacity: enabled ? 1 : 0.55
                onChosen: {
                    root.targetScopeType = "GROUP"; root.targetScopeId = ""
                    root.selectedGroup = null; root.choosingGroup = true
                    root.currentPath = ""; root.directories = []; root.browsing = true
                }
            }
        }

        ColumnLayout {
            visible: root.browsing
            Layout.fillWidth: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true; implicitHeight: 48; color: Theme.sunken
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 14; spacing: 8
                    UiButton {
                        text: qsTr("上一级"); enabled: root.selectedGroup !== null && root.currentPath.length > 0 && !root.loading
                        onClicked: root.loadDirectory(root.parentPath(root.currentPath))
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.selectedGroup
                              ? String(root.selectedGroup.name || "") + " / " + (root.currentPath || qsTr("根目录"))
                              : (root.choosingGroup ? qsTr("请选择目标群组")
                                                   : (root.currentPath.length > 0 ? "/" + root.currentPath : qsTr("服务器根目录")))
                        color: Theme.ink2; font.family: Theme.dataFont; font.pixelSize: 11; elide: Text.ElideMiddle
                    }
                }
            }

            ListView {
                visible: root.choosingGroup && root.selectedGroup === null
                Layout.fillWidth: true; Layout.preferredHeight: 252
                clip: true; model: root.groups
                delegate: Rectangle {
                    id: groupRow
                    required property var modelData
                    width: ListView.view.width; height: 54
                    color: groupMouse.containsMouse ? Theme.signalWash : Theme.surface
                    MouseArea {
                        id: groupMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: {
                            root.selectedGroup = groupRow.modelData
                            root.targetScopeId = String(groupRow.modelData.groupId || "")
                            root.loadDirectory("")
                        }
                    }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 11
                        Rectangle { width: 30; height: 30; radius: 7; color: Theme.signalWash; Text { anchors.centerIn: parent; text: "♧"; color: Theme.signalDeep; font.bold: true } }
                        ColumnLayout { Layout.fillWidth: true; spacing: 2; Text { text: String(groupRow.modelData.name || ""); color: Theme.ink; font.pixelSize: 13; font.bold: true } Text { text: String(groupRow.modelData.myPermission) === "OWNER" ? qsTr("所有者") : qsTr("可读写"); color: Theme.muted; font.pixelSize: 10 } }
                        Text { text: "›"; font.pixelSize: 20; color: Theme.faint }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.lineSoft }
                }
            }

            ListView {
                id: directoryList
                visible: !root.choosingGroup || root.selectedGroup !== null
                Layout.fillWidth: true; Layout.preferredHeight: 252
                clip: true; model: root.directories
                delegate: Rectangle {
                    id: folderRow
                    required property var modelData
                    width: directoryList.width; height: 52
                    color: folderMouse.containsMouse ? Theme.signalWash : Theme.surface
                    MouseArea { id: folderMouse; anchors.fill: parent; hoverEnabled: true; onClicked: root.enterDirectory(folderRow.modelData) }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 11
                        Rectangle {
                            width: 30; height: 30; radius: 7; color: folderMouse.containsMouse ? Theme.surface : Theme.sunken
                            border.width: 1; border.color: Theme.line
                            Text { anchors.centerIn: parent; text: "▰"; color: Theme.signal; font.pixelSize: 15 }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: String(folderRow.modelData.name || folderRow.modelData.path || "")
                            font.pixelSize: 13; font.weight: Font.Medium; color: Theme.ink; elide: Text.ElideRight
                        }
                        Text { text: "›"; font.pixelSize: 20; color: Theme.faint }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.lineSoft }
                }
                Text { anchors.centerIn: parent; visible: !root.loading && root.directories.length === 0; text: qsTr("此目录没有子文件夹"); color: Theme.muted; font.pixelSize: 12 }
                BusyIndicator { anchors.centerIn: parent; visible: root.loading; running: visible; palette.dark: Theme.signal }
            }

            Text {
                visible: root.errorMessage.length > 0; text: root.errorMessage
                color: Theme.alert; wrapMode: Text.WordWrap
                Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; Layout.bottomMargin: 8
            }

            Rectangle {
                Layout.fillWidth: true; implicitHeight: 58; color: Theme.sunken
                Rectangle { width: parent.width; height: 1; color: Theme.lineSoft }
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 8
                    UiButton {
                        text: qsTr("返回")
                        onClicked: {
                            if (root.choosingGroup && root.selectedGroup) {
                                root.selectedGroup = null; root.targetScopeId = ""
                                root.currentPath = ""; root.directories = []
                            } else root.browsing = false
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.selectedGroup
                              ? qsTr("当前：") + String(root.selectedGroup.name || "") + " / " + (root.currentPath || qsTr("根目录"))
                              : qsTr("当前：") + (root.currentPath.length > 0 ? "/" + root.currentPath : "/")
                        font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.muted; elide: Text.ElideMiddle
                    }
                    UiButton {
                        kind: "primary"; text: qsTr("上传到此文件夹")
                        enabled: !root.loading && (!root.choosingGroup || root.selectedGroup !== null)
                        onClicked: {
                            root.uploadConfirmed(root.localFilePaths, root.currentPath,
                                                 root.targetScopeType, root.targetScopeId)
                            root.close()
                        }
                    }
                }
            }
        }
    }

    component DestinationOption: Rectangle {
        id: option
        property string title: ""
        property string description: ""
        property string symbol: ""
        signal chosen()
        Layout.fillWidth: true; implicitHeight: 66
        radius: Theme.radius; color: optionMouse.containsMouse ? Theme.signalWash : Theme.sunken
        border.width: 1; border.color: optionMouse.containsMouse ? Theme.signalEdge : Theme.line
        MouseArea { id: optionMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: option.chosen() }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 13; anchors.rightMargin: 14; spacing: 11
            Rectangle {
                width: 36; height: 36; radius: 9; color: Theme.surface; border.width: 1; border.color: Theme.line
                Text { anchors.centerIn: parent; text: option.symbol; color: Theme.signalDeep; font.pixelSize: 17; font.bold: true }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                Text { text: option.title; color: Theme.ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                Text { text: option.description; color: Theme.muted; font.pixelSize: 11 }
            }
            Text { text: "›"; font.pixelSize: 21; color: Theme.faint }
        }
    }
}
