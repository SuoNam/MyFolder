import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow{

    width:1000
    height:600

    title: qsTr("MyFolder")
    flags:Qt.FramelessWindowHint
    color:'transparent'
    visible: true

    Rectangle{
        id:maincontainer
        anchors.fill: parent
        width:1000
        height:600
        color:"#F9F8FD"

        Row{
            anchors {
                   top: parent.top
                   bottom: parent.bottom
                   left: parent.left
                   right: parent.right

                   // topMargin: 6
                   // bottomMargin: 6
                   // leftMargin: 10
                   // rightMargin: 10
               }

            Column{
                width: parent.width * 0.2
                height: parent.height
                Rectangle{
                    anchors.fill: parent
                    color: "#4ACE94";
                }
            }
            Column{
                width: parent.width * 0.8
                height: parent.height
                Rectangle{
                    anchors.fill: parent
                    gradient:
                        Gradient{
                        GradientStop{position: 0.0;color:"#EFF2FB"}
                        GradientStop{position: 1.0;color:"#FFFCFF"}
                    }
                }

            }
        }




    }




    Button{
        id:closeButton
        anchors.right: maincontainer.right
        anchors.top: maincontainer.top
        anchors.rightMargin: 0
        anchors.topMargin: 0
        width: 25;height:25
        contentItem: Text {
                text:"×"
                font.pixelSize: 16
                color: "black"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

        background: Rectangle {
               anchors.fill: parent
               color: closeButton.pressed ? "#D9534F" : closeButton.hovered ? "#FA5959" : "#E0E0E0"
               radius: 5
           }
        onClicked: Qt.quit()
    }
}
