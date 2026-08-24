pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root
    property string forwardId: ""
    property string sizeText: ""
    property string sourceText: ""
    property string fileText: ""
    property string savedPath: ""
    property var shown: ({})

    width: 360
    height: 148
    visible: false
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    function reposition() {
        x = Screen.virtualX + Screen.desktopAvailableWidth - width - 20
        y = Screen.virtualY + Screen.desktopAvailableHeight - height - 20
    }

    function showCompleted(id, totalBytes, source, filePath, destination) {
        if (!id || shown[id] === true) return
        shown[id] = true
        forwardId = id
        sizeText = Theme.formatBytes(totalBytes || 0)
        sourceText = source || qsTr("其他设备")
        fileText = filePath || qsTr("未知文件")
        savedPath = destination || qsTr("接收目录")
        reposition()
        visible = true
        raise()
        autoClose.restart()
    }

    Component.onCompleted: reposition()

    Timer {
        id: autoClose
        interval: 5000
        onTriggered: root.visible = false
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Theme.surface
        border.width: 1
        border.color: Theme.signalEdge

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 11
            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: 7
                color: Theme.signalWash
                Text { anchors.centerIn: parent; text: "✓"; color: Theme.signalDeep; font.pixelSize: 16; font.bold: true }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text { text: qsTr("文件接收完成"); font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.ink }
                Text { Layout.fillWidth: true; text: qsTr("来自：") + root.sourceText; font.pixelSize: 10; color: Theme.muted; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; text: qsTr("文件：") + root.fileText + " · " + root.sizeText; font.pixelSize: 10; color: Theme.muted; elide: Text.ElideMiddle }
                Text { Layout.fillWidth: true; text: qsTr("保存到：") + root.savedPath; font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.faint; elide: Text.ElideMiddle }
            }
            UiButton {
                kind: "quiet"
                implicitWidth: 30
                implicitHeight: 30
                text: "×"
                Accessible.name: qsTr("关闭通知")
                onClicked: { autoClose.stop(); root.visible = false }
            }
        }
    }
}
