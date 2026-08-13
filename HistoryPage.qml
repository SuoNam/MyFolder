pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// History page: settled forwards as a ledger table.
Item {
    id: root

    readonly property var records: ForwardManager.tasks.filter(function(t) {
        return t.state === "COMPLETED" || t.state === "FAILED" || t.state === "CANCELLED"
    })

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("传输记录"); font.pixelSize: 22; font.bold: true; color: Theme.ink }
            Item { Layout.fillWidth: true }
            UiButton { text: qsTr("刷新"); enabled: !ForwardManager.busy; onClicked: ForwardManager.refreshTasks() }
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
                        readonly property bool incoming: modelData.targetDeviceId === DeviceManager.deviceId
                        readonly property bool ok: modelData.state === "COMPLETED"
                        width: ListView.view.width
                        height: 44

                        RowLayout {
                            anchors.fill: parent
                            spacing: 12
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 9
                                Text {
                                    text: row.incoming ? "↓" : "↑"
                                    font.pixelSize: 12; font.bold: true
                                    color: Theme.ink2
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: (row.modelData.files && row.modelData.files.length === 1)
                                          ? String(row.modelData.files[0].path || "").split(/[\\/]/).pop()
                                          : (row.modelData.files ? row.modelData.files.length + qsTr(" 个文件") : "-")
                                    font.pixelSize: 13; font.weight: Font.Medium
                                    color: Theme.ink
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                Layout.preferredWidth: 128
                                text: row.incoming ? row.modelData.sourceDeviceId : row.modelData.targetDeviceId
                                font.family: Theme.dataFont; font.pixelSize: 11
                                color: Theme.muted
                                elide: Text.ElideRight
                            }
                            UiChip {
                                Layout.preferredWidth: 58
                                channel: row.modelData.channel || "RELAY"
                                text: row.modelData.channel || "RELAY"
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
                                    : row.modelData.state === "FAILED" ? qsTr("失败") : qsTr("已取消")
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
