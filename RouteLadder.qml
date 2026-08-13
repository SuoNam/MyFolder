import QtQuick
import QtQuick.Layouts

// The route ladder — MyFolder's signature element.
// Rungs appear in the order the client tries them (LAN → P2P → RELAY), each
// carrying the reason it was taken or ruled out. `rungs` is a list of:
//   { name: "LAN", state: "live"|"out"|"wait", note: "同网段 · 延迟 1 ms" }
ColumnLayout {
    id: root
    property var rungs: []
    property bool animate: true
    spacing: 0

    Repeater {
        model: root.rungs

        delegate: Item {
            id: rung
            required property var modelData
            required property int index
            readonly property bool live: modelData.state === "live"
            readonly property bool out: modelData.state === "out"
            readonly property color chColor: Theme.channelColor(modelData.name)

            Layout.fillWidth: true
            implicitHeight: 22

            // dashed spine linking this node to the next
            Rectangle {
                visible: rung.index < root.rungs.length - 1
                x: 4; y: 15; width: 1; height: 12
                color: "transparent"
                Column {
                    spacing: 3
                    Repeater {
                        model: 3
                        Rectangle { width: 1; height: 3; color: Theme.line }
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 10

                // node
                Item {
                    Layout.preferredWidth: 10; Layout.preferredHeight: 10
                    Rectangle {
                        anchors.centerIn: parent
                        width: 9; height: 9; radius: 4.5
                        color: rung.live ? rung.chColor : Theme.surface
                        border.width: 1
                        border.color: rung.live ? rung.chColor : Theme.line
                        // halo on the live rung
                        Rectangle {
                            visible: rung.live
                            anchors.centerIn: parent
                            width: 15; height: 15; radius: 7.5
                            color: "transparent"
                            border.width: 3
                            border.color: Theme.channelWash(rung.modelData.name)
                            z: -1
                        }
                        // slash on a ruled-out rung
                        Rectangle {
                            visible: rung.out
                            anchors.centerIn: parent
                            width: 8; height: 1
                            rotation: -45
                            color: Theme.faint
                        }
                    }
                }

                Text {
                    Layout.preferredWidth: 42
                    text: rung.modelData.name
                    font.family: Theme.dataFont
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.9
                    font.strikeout: rung.out
                    color: rung.live ? Theme.ink : Theme.faint
                    opacity: rung.out ? 0.75 : 1
                }

                Text {
                    Layout.fillWidth: true
                    text: rung.modelData.note || ""
                    font.family: Theme.dataFont
                    font.pixelSize: 11
                    color: rung.live ? Theme.ink2 : Theme.muted
                    elide: Text.ElideRight
                }

                // data-in-motion: travelling dashes, only on the live rung
                Item {
                    visible: rung.live
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 2
                    clip: true
                    Row {
                        id: wire
                        spacing: 6
                        Repeater {
                            model: 5
                            Rectangle { width: 5; height: 2; radius: 1; color: rung.chColor }
                        }
                        NumberAnimation on x {
                            running: root.animate && rung.live
                            loops: Animation.Infinite
                            from: -11; to: 0; duration: 700
                        }
                    }
                }

                Text {
                    visible: rung.live
                    text: qsTr("已选用")
                    font.family: Theme.dataFont
                    font.pixelSize: 11
                    color: rung.chColor
                }
            }
        }
    }
}
