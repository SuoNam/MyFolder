import QtQuick

// Channel / status chip. `channel` picks the semantic colour set:
// LAN, P2P, RELAY, or "idle" / "alert" for neutral and error states.
Rectangle {
    id: root
    property string channel: "idle"
    property alias text: label.text

    implicitWidth: label.implicitWidth + 16
    implicitHeight: 21
    radius: 4
    color: channel === "idle"  ? Theme.sunken
         : channel === "alert" ? Theme.alertWash
         : Theme.channelWash(channel)
    border.width: 1
    border.color: channel === "idle"  ? Theme.line
                : channel === "alert" ? Theme.alertEdge
                : Theme.channelEdge(channel)

    Text {
        id: label
        anchors.centerIn: parent
        font.family: Theme.dataFont
        font.pixelSize: 10
        font.bold: true
        font.letterSpacing: 0.9
        color: root.channel === "idle"  ? Theme.muted
             : root.channel === "alert" ? Theme.alert
             : root.channel === "LAN"   ? Theme.signalDeep
             : root.channel === "P2P"   ? "#94590F"
             : "#33507D"
    }
}
