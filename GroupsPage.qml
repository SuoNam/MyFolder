pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal groupUploadStarted()

    property var groups: []
    property var selectedGroup: null
    property var groupPendingLeave: null
    property string errorMessage: ""
    readonly property bool showingFiles: selectedGroup !== null

    function openGroup(groupValue) {
        root.selectedGroup = groupValue
        Qt.callLater(function() { groupFiles.openGroup(groupValue) })
    }

    function refreshContent() {
        HttpHandler.loadGroups()
        if (root.selectedGroup) groupFiles.refreshContent()
    }

    function refreshGroupFiles(groupId) {
        if (root.selectedGroup && String(root.selectedGroup.groupId || "") === String(groupId || ""))
            groupFiles.refreshContent()
    }

    Component.onCompleted: HttpHandler.loadGroups()

    Connections {
        target: HttpHandler
        function onGroupsResult(response) {
            if (response.status === 200) {
                root.groups = response.data || []
                root.errorMessage = ""
                if (root.selectedGroup) {
                    for (var i = 0; i < root.groups.length; ++i) {
                        if (root.groups[i].groupId === root.selectedGroup.groupId) {
                            root.selectedGroup = root.groups[i]
                            break
                        }
                    }
                }
            } else {
                root.errorMessage = response.message || qsTr("加载群组失败")
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: root.showingFiles ? 1 : 0

        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        spacing: 3
                        Text { text: qsTr("共享群组"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
                        Text { text: qsTr("邀请其他账号共同维护一个文件空间，可授予只读或读写权限"); font.pixelSize: 12; color: Theme.muted }
                    }
                    Item { Layout.fillWidth: true }
                    UiInput { id: groupName; Layout.preferredWidth: 190; placeholderText: qsTr("群组名称") }
                    UiButton {
                        kind: "primary"
                        text: qsTr("创建")
                        enabled: groupName.text.trim().length >= 2
                        onClicked: {
                            HttpHandler.createGroup(groupName.text)
                            groupName.text = ""
                        }
                    }
                }

                UiNotice {
                    Layout.fillWidth: true
                    message: root.errorMessage
                    tone: "error"
                    onDismissed: root.errorMessage = ""
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 10

                        Repeater {
                            model: root.groups

                            Rectangle {
                                id: groupCard
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: cardColumn.implicitHeight + 24
                                radius: Theme.radius
                                color: Theme.surface
                                border.width: 1
                                border.color: Theme.line

                                ColumnLayout {
                                    id: cardColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 12
                                    spacing: 9

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: groupCard.modelData.name || ""; font.pixelSize: 15; font.bold: true; color: Theme.ink }
                                        Item { Layout.fillWidth: true }
                                        UiChip {
                                            channel: "LAN"
                                            text: groupCard.modelData.myPermission === "OWNER" ? qsTr("所有者")
                                                  : groupCard.modelData.myPermission === "WRITE" ? qsTr("可读写") : qsTr("只读")
                                        }
                                         UiButton {
                                             kind: "primary"
                                             text: qsTr("打开文件区")
                                             onClicked: root.openGroup(groupCard.modelData)
                                         }
                                         UiButton {
                                             visible: groupCard.modelData.myPermission !== "OWNER"
                                             kind: "danger"
                                             text: qsTr("退出群组")
                                             onClicked: {
                                                 root.groupPendingLeave = groupCard.modelData
                                                 leaveDialog.dialogTitle = qsTr("退出“%1”？").arg(groupCard.modelData.name || qsTr("群组"))
                                                 leaveDialog.present("")
                                             }
                                         }
                                    }

                                    Text {
                                        text: (groupCard.modelData.members || []).map(function(member) {
                                            return (member.displayName || member.account) + " · "
                                                    + (member.permission === "OWNER" ? qsTr("所有者")
                                                       : member.permission === "WRITE" ? qsTr("可读写") : qsTr("只读"))
                                        }).join("    ")
                                        color: Theme.muted
                                        font.pixelSize: 11
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }

                                    RowLayout {
                                        visible: groupCard.modelData.myPermission === "OWNER"
                                        Layout.fillWidth: true
                                         UiInput { id: memberEmail; Layout.fillWidth: true; placeholderText: qsTr("输入目标用户邮箱") }
                                         UiComboBox { id: permission; model: [qsTr("可读写"), qsTr("只读")]; implicitWidth: 112 }
                                         UiButton {
                                             text: qsTr("添加成员")
                                             enabled: memberEmail.text.trim().indexOf("@") > 0
                                             onClicked: {
                                                 HttpHandler.addGroupMember(groupCard.modelData.groupId,
                                                                            memberEmail.text,
                                                                            permission.currentIndex === 0 ? "WRITE" : "READ")
                                                 memberEmail.text = ""
                                             }
                                        }
                                    }

                                    Text {
                                        visible: groupCard.modelData.myPermission === "OWNER"
                                        text: qsTr("也可以在 Web 端管理群组成员和文件权限")
                                        color: Theme.faint
                                        font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        GroupFilesPage {
            id: groupFiles
            group: root.selectedGroup || ({})
            onBackRequested: root.selectedGroup = null
            onUploadStarted: root.groupUploadStarted()
        }
    }

    UiPromptDialog {
        id: leaveDialog
        dialogTitle: qsTr("退出群组？")
        description: qsTr("退出后将立即失去群组文件和目录权限；共享文件不会被删除。")
        confirmText: qsTr("确认退出")
        showInput: false
        danger: true
        onConfirmRequested: function(value) {
            if (root.groupPendingLeave) HttpHandler.leaveGroup(root.groupPendingLeave.groupId)
            root.groupPendingLeave = null
            close()
        }
    }
}
