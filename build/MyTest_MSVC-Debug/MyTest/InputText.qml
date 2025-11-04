import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: inputText

    property int radius: 10
    property color backgroundColor: "#EFEFF7"
    property int pixelSize: 16
    property alias placeholderText: textfield.placeholderText
    property alias text: textfield.text
    property alias input: textfield
    property alias placeholdertextcolor: textfield.placeholderTextColor
     property alias textcolor: textfield.color
    property alias  type: textfield.echoMode
    implicitWidth:250
    implicitHeight:40
    width:inputText.implicitWidth
    height: inputText.implicitHeight

    property Component leadingIcon: null

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 8
        Loader {
            id: iconLoader
            Layout.preferredWidth: inputText.implicitHeight-15
           Layout.preferredHeight: inputText.implicitHeight-15
            sourceComponent: leadingIcon
            visible: leadingIcon !== null
            Layout.alignment: Qt.AlignVCenter
               }

        TextField {
            id: textfield
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
            font.pixelSize: inputText.pixelSize
            verticalAlignment: Text.AlignVCenter
            background: Rectangle {
                anchors.fill: parent
                color: inputText.backgroundColor
                radius: inputText.radius
            }
        }
    }
}

