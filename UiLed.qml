import QtQuick

// Status LED: online (breathing ring), busy (amber), off (grey).
Item {
    id: root
    property bool on: false
    property bool busy: false
    implicitWidth: 8; implicitHeight: 8

    Rectangle {
        anchors.centerIn: parent
        width: 8; height: 8; radius: 4
        color: root.busy ? Theme.p2p : root.on ? Theme.signal : Theme.faint
    }
    Rectangle {
        id: ring
        visible: root.on && !root.busy
        anchors.centerIn: parent
        width: 16; height: 16; radius: 8
        color: "transparent"
        border.width: 1
        border.color: Theme.signal
        opacity: 0

        SequentialAnimation on scale {
            running: ring.visible
            loops: Animation.Infinite
            NumberAnimation { from: 0.65; to: 1.2; duration: 1700; easing.type: Easing.OutCubic }
            PauseAnimation { duration: 700 }
        }
        SequentialAnimation on opacity {
            running: ring.visible
            loops: Animation.Infinite
            NumberAnimation { from: 0.5; to: 0; duration: 1700 }
            PauseAnimation { duration: 700 }
        }
    }
}
