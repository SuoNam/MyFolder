import QtQuick
import QtQuick.Controls

// Primary / secondary / quiet / danger button, sized for the compact client UI.
Button {
    id: root
    property string kind: "secondary"   // primary | secondary | quiet | danger | dangerPrimary
    property int pixelSize: 12

    implicitHeight: 32
    leftPadding: 13; rightPadding: 13
    font.pixelSize: pixelSize
    font.family: Theme.uiFont
    font.weight: kind === "primary" ? Font.DemiBold : Font.Medium
    hoverEnabled: true

    background: Rectangle {
        radius: Theme.radiusSm
        color: {
            if (!root.enabled) return Theme.sunken
            if (root.kind === "primary") return root.hovered ? Theme.signalDeep : Theme.signal
            if (root.kind === "dangerPrimary") return root.hovered ? "#91352F" : Theme.alert
            if (root.kind === "danger")  return root.hovered ? Theme.alertWash : Theme.surface
            if (root.kind === "quiet")   return "transparent"
            return root.hovered ? Theme.sunken : Theme.surface
        }
        border.width: root.kind === "quiet" ? 0 : 1
        border.color: {
            if (!root.enabled) return Theme.lineSoft
            if (root.kind === "primary") return root.hovered ? Theme.signalDeep : Theme.signal
            if (root.kind === "dangerPrimary") return root.hovered ? "#91352F" : Theme.alert
            if (root.kind === "danger")  return root.hovered ? Theme.alertEdge : Theme.line
            return root.hovered ? "#CBD8D1" : Theme.line
        }
        Behavior on color { ColorAnimation { duration: 120 } }
    }
    contentItem: Text {
        text: root.text
        font: root.font
        color: {
            if (!root.enabled) return Theme.faint
            if (root.kind === "primary" || root.kind === "dangerPrimary") return "#FFFFFF"
            if (root.kind === "danger")  return Theme.alert
            if (root.kind === "quiet" && root.hovered) return Theme.signalDeep
            return Theme.ink2
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
