import QtQuick
import QtQuick.Layouts

// One rail navigation entry.
Rectangle {
    id: root
    property string label: ""
    property int count: -1
    property bool active: false
    signal clicked()

    Layout.fillWidth: true
    implicitHeight: 32
    radius: Theme.radiusSm
    color: active ? Theme.signalWash : area.containsMouse ? Theme.sunken : "transparent"
    border.width: 1
    border.color: active ? Theme.signalEdge : "transparent"

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 9; anchors.rightMargin: 9
        spacing: 10
        Rectangle {
            width: 5; height: 5; radius: 2.5
            color: root.active ? Theme.signalDeep : Theme.faint
        }
        Text {
            Layout.fillWidth: true
            text: root.label
            font.pixelSize: 13
            font.weight: root.active ? Font.DemiBold : Font.Medium
            color: root.active ? Theme.signalDeep : Theme.ink2
            elide: Text.ElideRight
        }
        Text {
            visible: root.count >= 0
            text: root.count
            font.family: Theme.dataFont
            font.pixelSize: 10
            color: root.active ? Theme.signalDeep : Theme.faint
        }
    }
}
