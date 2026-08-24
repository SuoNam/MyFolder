pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root
    property var offer: null
    signal acceptRequested()
    signal laterRequested()
    signal rejectRequested()

    width: 360
    height: 132
    visible: offer !== null
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    modality: Qt.NonModal

    function fileTitle() {
        if (!offer || !offer.files || offer.files.length === 0) return qsTr("新文件")
        if (offer.files.length === 1)
            return String(offer.files[0].path || qsTr("新文件")).split(/[\\/]/).pop()
        return offer.files.length + qsTr(" 个文件")
    }
    function reposition() {
        x = Screen.virtualX + Screen.desktopAvailableWidth - width - 20
        y = Screen.virtualY + Screen.desktopAvailableHeight - height - 20
    }
    onVisibleChanged: if (visible) { reposition(); raise(); requestActivate() }
    Component.onCompleted: reposition()

    Rectangle {
        anchors.fill: parent
        radius: 11
        color: Theme.surface
        border.width: 1
        border.color: Theme.signalEdge

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 13
            spacing: 7
            RowLayout {
                Layout.fillWidth: true
                spacing: 9
                Rectangle {
                    width: 31; height: 31; radius: 8; color: Theme.signalWash
                    Text { anchors.centerIn: parent; text: "↓"; color: Theme.signalDeep; font.pixelSize: 17; font.bold: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 1
                    Text { text: qsTr("收到文件"); font.pixelSize: 14; font.weight: Font.DemiBold; color: Theme.ink }
                    Text { Layout.fillWidth: true; text: root.fileTitle(); font.pixelSize: 12; color: Theme.muted; elide: Text.ElideMiddle }
                }
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("保存到：") + RelayDownloadManager.receiveRoot
                font.family: Theme.dataFont; font.pixelSize: 10; color: Theme.faint; elide: Text.ElideMiddle
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                UiButton { Layout.fillWidth: true; kind: "danger"; text: qsTr("拒绝"); onClicked: root.rejectRequested() }
                UiButton { Layout.fillWidth: true; text: qsTr("稍后"); onClicked: root.laterRequested() }
                UiButton { Layout.fillWidth: true; kind: "primary"; text: qsTr("接收"); onClicked: root.acceptRequested() }
            }
        }
    }
}
