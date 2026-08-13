import QtQuick

// Slim progress bar tinted by transport channel.
Item {
    id: root
    property real value: 0            // 0..1
    property string channel: "LAN"    // LAN | P2P | RELAY
    property bool failed: false

    implicitHeight: 4

    Rectangle { anchors.fill: parent; radius: 2; color: Theme.lineSoft }
    Rectangle {
        width: Math.max(0, Math.min(1, root.value)) * parent.width
        height: parent.height
        radius: 2
        color: root.failed ? Theme.alert : Theme.channelColor(root.channel)
        Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    }
}
