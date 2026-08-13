import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var groups: []
    property string errorMessage: ""
    Component.onCompleted: HttpHandler.loadGroups()
    Connections {
        target: HttpHandler
        function onGroupsResult(response) {
            if (response.status === 200) { root.groups = response.data || []; root.errorMessage = "" }
            else root.errorMessage = response.message || qsTr("无法读取群组")
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 14
        RowLayout { Layout.fillWidth: true
            ColumnLayout { spacing: 3; Text { text: qsTr("共享群组"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
                Text { text: qsTr("多人共享文件空间，群主负责成员与目录权限。"); font.pixelSize: 12; color: Theme.muted } }
            Item { Layout.fillWidth: true }
            UiInput { id: groupName; Layout.preferredWidth: 190; placeholderText: qsTr("新群组名称") }
            UiButton { kind: "primary"; text: qsTr("创建"); enabled: groupName.text.trim().length >= 2; onClicked: { HttpHandler.createGroup(groupName.text); groupName.text = "" } }
        }
        Text { visible: root.errorMessage.length > 0; text: root.errorMessage; color: Theme.alert; font.pixelSize: 11 }
        ScrollView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            ColumnLayout { width: parent.width; spacing: 10
                Repeater { model: root.groups
                    Rectangle { required property var modelData; Layout.fillWidth: true; implicitHeight: cardColumn.implicitHeight + 24; radius: Theme.radius; color: Theme.surface; border.width: 1; border.color: Theme.line
                        ColumnLayout { id: cardColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 12; spacing: 9
                            RowLayout { Layout.fillWidth: true; Text { text: modelData.name || ""; font.pixelSize: 15; font.bold: true; color: Theme.ink }
                                Item { Layout.fillWidth: true } UiChip { channel: "LAN"; text: modelData.myPermission === "OWNER" ? qsTr("群主") : modelData.myPermission === "WRITE" ? qsTr("可读写") : qsTr("只读") } }
                            Text { text: (modelData.members || []).map(function(m){ return (m.displayName || m.account) + " · " + (m.permission === "OWNER" ? qsTr("群主") : m.permission === "WRITE" ? qsTr("可读写") : qsTr("只读")) }).join("    "); color: Theme.muted; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
                            RowLayout { visible: modelData.myPermission === "OWNER"; Layout.fillWidth: true
                                UiInput { id: memberAccount; Layout.fillWidth: true; placeholderText: qsTr("输入登录账号") }
                                ComboBox { id: permission; model: [qsTr("可读写"), qsTr("只读")]; implicitWidth: 100 }
                                UiButton { text: qsTr("添加成员"); enabled: memberAccount.text.trim().length > 0; onClicked: { HttpHandler.addGroupMember(modelData.groupId, memberAccount.text, permission.currentIndex === 0 ? "WRITE" : "READ"); memberAccount.text = "" } }
                            }
                            Text { visible: modelData.myPermission === "OWNER"; text: qsTr("更细的文件夹权限可在 Web 文件页的“当前目录权限”中设置。"); color: Theme.faint; font.pixelSize: 10 }
                        }
                    }
                }
            }
        }
    }
}
