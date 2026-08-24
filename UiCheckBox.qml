import QtQuick
import QtQuick.Controls

// Check box that follows the MyFolder surface/brand palette instead of the
// platform-native control style.
CheckBox {
    id: root

    property int boxSize: 17
    property int pixelSize: 11

    implicitHeight: 26
    spacing: 7
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true
    font.family: Theme.uiFont
    font.pixelSize: pixelSize

    indicator: Rectangle {
        implicitWidth: root.boxSize
        implicitHeight: root.boxSize
        x: root.leftPadding
        y: Math.round((root.height - height) / 2)
        radius: 4
        color: !root.enabled ? Theme.sunken
             : root.checkState === Qt.Checked ? Theme.signal
             : root.hovered ? Theme.signalWash : Theme.surface
        border.width: 1
        border.color: !root.enabled ? Theme.lineSoft
                    : root.checkState === Qt.Unchecked ? (root.hovered ? Theme.signalEdge : Theme.line)
                    : Theme.signal

        Text {
            anchors.centerIn: parent
            visible: root.checkState === Qt.Checked
            text: "✓"
            color: "#FFFFFF"
            font.family: Theme.uiFont
            font.pixelSize: 12
            font.bold: true
        }

        Rectangle {
            anchors.centerIn: parent
            visible: root.checkState === Qt.PartiallyChecked
            width: 9
            height: 2
            radius: 1
            color: "#FFFFFF"
        }

        Behavior on color { ColorAnimation { duration: 110 } }
        Behavior on border.color { ColorAnimation { duration: 110 } }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        font: root.font
        color: root.enabled ? Theme.ink2 : Theme.faint
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}