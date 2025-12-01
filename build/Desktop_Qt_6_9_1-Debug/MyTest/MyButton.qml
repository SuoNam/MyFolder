import QtQuick

Item {
    property  color  backgroundcolor
    property  string buttontext
    property color textcolor
    property color  hovercolor
    property int pixelSize
    property var onClickedFunc
    property int buttonRadius

    width:parent.width
    height:parent.height
    id:mybtn
    Rectangle{
        border.width: 0
        radius:buttonRadius ? buttonRadius:20
        width:parent.width;height:parent.height
        color:mouseArea.containsMouse ? hovercolor:parent.backgroundcolor
        anchors.horizontalCenter: parent.horizontalCenter
        Text{
         text:mybtn.buttontext
         font.pixelSize:mybtn.pixelSize
         anchors.centerIn: parent
         color:mybtn.textcolor
        }
    MouseArea{
         id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked:{
            if (mybtn.onClickedFunc) {
                mybtn.onClickedFunc();
            }
        }
    }
    }


}
