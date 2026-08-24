import QtQuick
import QtQuick.Controls

ComboBox {
    id: root
    implicitWidth: 112
    implicitHeight: 34
    leftPadding: 11
    rightPadding: 30
    font.family: Theme.uiFont
    font.pixelSize: 12

    contentItem: Text {
        leftPadding: root.leftPadding
        rightPadding: root.rightPadding
        text: root.displayText
        font: root.font
        color: Theme.ink2
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Text {
        x: root.width - width - 11
        y: Math.round((root.height - height) / 2) - 1
        text: "⌄"
        color: Theme.muted
        font.pixelSize: 14
    }

    background: Rectangle {
        radius: Theme.radiusSm
        color: root.hovered ? Theme.sunken : Theme.surface
        border.width: 1
        border.color: root.activeFocus ? Theme.signal : Theme.line
    }

    delegate: ItemDelegate {
        required property var modelData
        required property int index
        width: root.width
        height: 32
        highlighted: root.highlightedIndex === index
        contentItem: Text {
            text: String(parent.modelData)
            color: parent.highlighted ? Theme.signalDeep : Theme.ink2
            font.family: Theme.uiFont
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle { color: parent.highlighted ? Theme.signalWash : Theme.surface }
    }

    popup: Popup {
        y: root.height + 4
        width: root.width
        implicitHeight: contentItem.implicitHeight + 8
        padding: 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
        }
        background: Rectangle {
            radius: Theme.radiusSm
            color: Theme.surface
            border.width: 1
            border.color: Theme.line
        }
    }
}
