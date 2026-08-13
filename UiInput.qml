import QtQuick
import QtQuick.Controls

// Text input with focus ring; optional mono for paths and machine values.
TextField {
    id: root
    property bool mono: false

    implicitHeight: 34
    leftPadding: 11; rightPadding: 11
    font.pixelSize: mono ? 11.5 : 13
    font.family: mono ? Theme.dataFont : Theme.uiFont
    color: Theme.ink
    placeholderTextColor: Theme.faint
    selectByMouse: true
    selectionColor: Theme.signalWash
    selectedTextColor: Theme.signalDeep

    background: Rectangle {
        radius: Theme.radiusSm
        color: Theme.surface
        border.width: 1
        border.color: root.activeFocus ? Theme.signal : Theme.line
        // soft focus halo, matching the web app's box-shadow ring
        Rectangle {
            visible: root.activeFocus
            anchors.fill: parent; anchors.margins: -3
            radius: Theme.radiusSm + 3
            color: "transparent"
            border.width: 3
            border.color: Theme.signalWash
            z: -1
        }
    }
}
