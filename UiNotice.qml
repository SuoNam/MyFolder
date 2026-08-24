pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string message: ""
    // normal = green, warning = amber, error = red
    property string tone: "normal"
    property int timeout: 5000
    property bool autoDismiss: true
    signal dismissed()

    readonly property color wash: tone === "error" ? Theme.alertWash
                                  : tone === "warning" ? Theme.p2pWash : Theme.signalWash
    readonly property color edge: tone === "error" ? Theme.alertEdge
                                  : tone === "warning" ? Theme.p2pEdge : Theme.signalEdge
    readonly property color ink: tone === "error" ? Theme.alert
                                 : tone === "warning" ? Theme.p2p : Theme.signalDeep

    visible: message.length > 0
    implicitHeight: noticeText.implicitHeight + 18
    radius: Theme.radiusSm
    color: wash
    border.width: 1
    border.color: edge

    onMessageChanged: {
        dismissTimer.stop()
        if (autoDismiss && message.length > 0) dismissTimer.restart()
    }

    Timer {
        id: dismissTimer
        interval: root.timeout
        repeat: false
        onTriggered: root.dismissed()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 11
        anchors.rightMargin: 5
        spacing: 8

        Text {
            id: noticeText
            Layout.fillWidth: true
            text: root.message
            color: root.ink
            font.family: Theme.uiFont
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        UiButton {
            kind: "quiet"
            implicitWidth: 28
            implicitHeight: 28
            leftPadding: 0
            rightPadding: 0
            text: "×"
            Accessible.name: qsTr("关闭消息")
            onClicked: {
                dismissTimer.stop()
                root.dismissed()
            }
        }
    }
}
