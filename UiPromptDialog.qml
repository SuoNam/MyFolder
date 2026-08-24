import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Branded modal used by file operations. It avoids the native Windows dialog
// palette so prompts remain visually consistent with the rest of MyFolder.
Popup {
    id: root

    property string dialogTitle: ""
    property string description: ""
    property string placeholderText: ""
    property string confirmText: qsTr("确定")
    property string initialText: ""
    property string errorText: ""
    property bool showInput: true
    property bool danger: false

    signal confirmRequested(string value)

    function present(value) {
        initialText = String(value === undefined ? "" : value)
        promptInput.text = initialText
        errorText = ""
        open()
    }

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(420, parent.width - 40)
    implicitHeight: dialogColumn.implicitHeight
    modal: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#520E211A" }

    background: Rectangle {
        radius: 12
        color: Theme.surface
        border.width: 1
        border.color: Theme.line
    }

    onOpened: {
        if (showInput) {
            promptInput.forceActiveFocus()
            promptInput.selectAll()
        }
    }

    contentItem: ColumnLayout {
        id: dialogColumn
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 12
            Layout.topMargin: 14
            Layout.bottomMargin: 13
            spacing: 11

            Rectangle {
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
                radius: 9
                color: root.danger ? Theme.alertWash : Theme.signalWash
                border.width: 1
                border.color: root.danger ? Theme.alertEdge : Theme.signalEdge

                Text {
                    anchors.centerIn: parent
                    text: root.danger ? "!" : "+"
                    color: root.danger ? Theme.alert : Theme.signalDeep
                    font.family: Theme.uiFont
                    font.pixelSize: 17
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: root.dialogTitle
                    color: Theme.ink
                    font.family: Theme.uiFont
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.showInput && root.description.length > 0
                    text: root.description
                    color: Theme.muted
                    font.family: Theme.uiFont
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                }
            }

            UiButton {
                kind: "quiet"
                text: "×"
                implicitWidth: 30
                implicitHeight: 30
                pixelSize: 16
                onClicked: root.close()
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.topMargin: 16
            Layout.bottomMargin: 14
            spacing: 9

            UiInput {
                id: promptInput
                visible: root.showInput
                Layout.fillWidth: true
                placeholderText: root.placeholderText
                onAccepted: {
                    if (root.showInput && text.trim().length === 0) return
                    root.confirmRequested(text)
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.errorText.length > 0
                text: root.errorText
                color: Theme.alert
                font.family: Theme.uiFont
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                visible: !root.showInput && root.description.length > 0
                text: root.description
                color: Theme.ink2
                font.family: Theme.uiFont
                font.pixelSize: 12
                lineHeight: 1.45
                wrapMode: Text.WordWrap
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.lineSoft }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            Layout.topMargin: 12
            Layout.bottomMargin: 14
            spacing: 8

            Item { Layout.fillWidth: true }
            UiButton { text: qsTr("取消"); onClicked: root.close() }
            UiButton {
                kind: root.danger ? "dangerPrimary" : "primary"
                text: root.confirmText
                enabled: !root.showInput || promptInput.text.trim().length > 0
                onClicked: root.confirmRequested(promptInput.text)
            }
        }
    }
}