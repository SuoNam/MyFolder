import QtQuick

CloudFilesPage {
    scopeType: "PRIVATE"
    scopeId: ""
    scopeName: qsTr("我的文件")
    permission: "OWNER"
    showBackButton: false
    function refreshContent() { loadDirectory(currentPath) }
    Component.onCompleted: resetAndLoad()
}
