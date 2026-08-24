pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(620, parent ? parent.width - 40 : 620)
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string mode: "copy"
    property var sourcePaths: []
    property var groups: []
    property var selectedGroup: null
    property var directories: []
    property string currentPath: ""
    property bool loading: false
    property string errorMessage: ""

    signal privateRequested(string mode)
    signal groupConfirmed(string mode, var sourcePaths, string groupId, string targetDirectory)

    function present(action, paths) {
        mode = action === "move" ? "move" : "copy"
        sourcePaths = paths || []
        groups = []
        selectedGroup = null
        directories = []
        currentPath = ""
        loading = true
        errorMessage = ""
        open()
        HttpHandler.loadGroups()
    }

    function cleanPath(path) { return String(path || "").replace(/^\/+|\/+$/g, "") }
    function parentPath(path) {
        var parts = cleanPath(path).split("/").filter(function(part) { return part.length > 0 })
        parts.pop()
        return parts.join("/")
    }
    function chooseGroup(group) {
        selectedGroup = group
        loadDirectory("")
    }
    function loadDirectory(path) {
        if (!selectedGroup) return
        currentPath = cleanPath(path)
        loading = true
        errorMessage = ""
        HttpHandler.listScopedDirectory(currentPath, "GROUP", String(selectedGroup.groupId || ""))
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
            if (response.status !== 200) {
                root.loading = false
                root.errorMessage = response.message || qsTr("加载群组失败")
                return
            }
            var writable = []
            var source = response.data || []
            for (var i = 0; i < source.length; ++i) {
                var permission = String(source[i].myPermission || "").toUpperCase()
                if (permission === "OWNER" || permission === "WRITE") writable.push(source[i])
            }
            root.groups = writable
            root.loading = false
        }
        function onScopedDirectoryListed(scopeType, scopeId, directoryPath, entries, error) {
            if (!root.visible || !root.selectedGroup || scopeType !== "GROUP"
                    || scopeId !== String(root.selectedGroup.groupId || "")
                    || directoryPath !== root.currentPath) return
            root.loading = false
            root.errorMessage = error || ""
            root.directories = (entries || []).filter(function(item) {
                return String(item.type || "").toLowerCase() === "directory"
            })
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            spacing: 10
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    text: root.mode === "copy" ? qsTr("复制到其他文件区") : qsTr("移动到其他文件区")
                    color: Theme.ink; font.pixelSize: 18; font.bold: true
                }
                Text {
                    text: qsTr("已选择 %1 项；群组只显示你拥有写权限的文件区").arg(root.sourcePaths.length)
                    color: Theme.muted; font.pixelSize: 11
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
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 10

            Text { text: qsTr("目标文件区"); color: Theme.ink2; font.pixelSize: 11; font.bold: true }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                Rectangle {
                    width: 180; height: 64; radius: Theme.radius
                    color: privateMouse.containsMouse ? Theme.signalWash : Theme.surface
                    border.width: 1; border.color: privateMouse.containsMouse ? Theme.signal : Theme.line
                    MouseArea {
                        id: privateMouse; anchors.fill: parent; hoverEnabled: true
                        onClicked: { root.privateRequested(root.mode); root.close() }
                    }
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 11; spacing: 10
                        Rectangle { width: 30; height: 30; radius: 8; color: Theme.signalWash; Text { anchors.centerIn: parent; text: "⌂"; color: Theme.signalDeep; font.bold: true } }
                        ColumnLayout { spacing: 2; Text { text: qsTr("我的文件"); color: Theme.ink; font.bold: true; font.pixelSize: 12 } Text { text: qsTr("继续在私人文件区选择目录"); color: Theme.muted; font.pixelSize: 9 } }
                    }
                }
                Repeater {
                    model: root.groups
                    delegate: Rectangle {
                        id: groupCard
                        required property var modelData
                        width: 180; height: 64; radius: Theme.radius
                        property bool picked: root.selectedGroup && String(root.selectedGroup.groupId) === String(modelData.groupId)
                        color: picked || groupMouse.containsMouse ? Theme.signalWash : Theme.surface
                        border.width: 1; border.color: picked ? Theme.signal : Theme.line
                        MouseArea { id: groupMouse; anchors.fill: parent; hoverEnabled: true; onClicked: root.chooseGroup(groupCard.modelData) }
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 11; spacing: 10
                            Rectangle { width: 30; height: 30; radius: 8; color: Theme.signalWash; Text { anchors.centerIn: parent; text: "♧"; color: Theme.signalDeep; font.bold: true } }
                            ColumnLayout { Layout.fillWidth: true; spacing: 2; Text { Layout.fillWidth: true; text: String(groupCard.modelData.name || ""); color: Theme.ink; font.bold: true; font.pixelSize: 12; elide: Text.ElideRight } Text { text: String(groupCard.modelData.myPermission) === "OWNER" ? qsTr("所有者") : qsTr("可读写"); color: Theme.muted; font.pixelSize: 9 } }
                            Text { visible: groupCard.picked; text: "✓"; color: Theme.signalDeep; font.bold: true }
                        }
                    }
                }
            }

            Rectangle {
                visible: root.groups.length === 0 && !root.loading
                Layout.fillWidth: true; implicitHeight: 44; radius: Theme.radiusSm
                color: Theme.sunken; border.width: 1; border.color: Theme.lineSoft
                Text { anchors.centerIn: parent; text: qsTr("没有拥有写权限的群组"); color: Theme.muted; font.pixelSize: 11 }
            }

            ColumnLayout {
                visible: root.selectedGroup !== null
                Layout.fillWidth: true
                spacing: 7
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("群组目标：") + String(root.selectedGroup ? root.selectedGroup.name : "") + " / " + (root.currentPath || qsTr("根目录")); color: Theme.ink2; font.family: Theme.dataFont; font.pixelSize: 10; elide: Text.ElideMiddle }
                    UiButton { text: qsTr("上一级"); enabled: root.currentPath.length > 0 && !root.loading; onClicked: root.loadDirectory(root.parentPath(root.currentPath)) }
                }
                ListView {
                    id: folderList
                    Layout.fillWidth: true; Layout.preferredHeight: 190
                    clip: true; model: root.directories
                    delegate: Rectangle {
                        id: folderRow
                        required property var modelData
                        width: folderList.width; height: 44
                        color: folderMouse.containsMouse ? Theme.signalWash : Theme.surface
                        MouseArea { id: folderMouse; anchors.fill: parent; hoverEnabled: true; onClicked: root.loadDirectory(String(folderRow.modelData.path || folderRow.modelData.name || "")) }
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; Text { text: "▰"; color: Theme.signal } Text { Layout.fillWidth: true; text: String(folderRow.modelData.name || ""); color: Theme.ink; font.pixelSize: 12 } Text { text: "›"; color: Theme.faint; font.pixelSize: 18 } }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.lineSoft }
                    }
                    Text { anchors.centerIn: parent; visible: !root.loading && root.directories.length === 0; text: qsTr("当前文件夹没有子目录，可直接选择这里"); color: Theme.muted; font.pixelSize: 11 }
                    BusyIndicator { anchors.centerIn: parent; visible: root.loading; running: visible; palette.dark: Theme.signal }
                }
            }
            Text { visible: root.errorMessage.length > 0; Layout.fillWidth: true; text: root.errorMessage; color: Theme.alert; wrapMode: Text.WordWrap; font.pixelSize: 11 }
        }

        Rectangle {
            Layout.fillWidth: true; implicitHeight: 58; color: Theme.sunken
            Rectangle { width: parent.width; height: 1; color: Theme.lineSoft }
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                UiButton { text: qsTr("取消"); onClicked: root.close() }
                Item { Layout.fillWidth: true }
                UiButton {
                    kind: "primary"
                    text: root.mode === "copy" ? qsTr("复制到此文件夹") : qsTr("移动到此文件夹")
                    enabled: root.selectedGroup !== null && !root.loading
                    onClicked: {
                        root.groupConfirmed(root.mode, root.sourcePaths,
                                            String(root.selectedGroup.groupId || ""), root.currentPath)
                        root.close()
                    }
                }
            }
        }
    }
}
