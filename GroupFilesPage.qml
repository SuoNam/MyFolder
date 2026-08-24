import QtQuick

Item {
    id: root
    signal backRequested()
    signal uploadStarted()

    property var group: ({})
    readonly property string groupId: String(group.groupId || "")
    readonly property string groupName: String(group.name || qsTr("群组"))
    readonly property string permission: String(group.myPermission || "READ").toUpperCase()

    function openGroup(value) {
        group = value || ({})
        files.resetAndLoad()
    }

    function refreshContent() {
        if (groupId.length > 0) files.loadDirectory(files.currentPath)
    }

    CloudFilesPage {
        id: files
        anchors.fill: parent
        scopeType: "GROUP"
        scopeId: root.groupId
        scopeName: root.groupName
        permission: root.permission
        showBackButton: true
        onBackRequested: root.backRequested()
        onUploadStarted: root.uploadStarted()
    }
}
