import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    width:349
    height:425
    visible: true
    title: qsTr("MyFolder")
    flags:Qt.FramelessWindowHint
    color:'transparent'
    id:root

    Rectangle{
        id:maincontainer
        anchors.fill:parent;
        radius:10
        gradient:
            Gradient{
            GradientStop{position: 0.0;color:"#EFF2FB"}
            GradientStop{position: 1.0;color:"#FFFCFF"}
        }
        MouseArea {
                       anchors.fill: parent
                       property real clickX
                       property real clickY

                       onPressed: function(mouse){
                           clickX = mouse.x
                           clickY = mouse.y
                       }
                       onPositionChanged:function(mouse) {
                           root.x += mouse.x - clickX
                           root.y += mouse.y - clickY
                       }
                   }
        ColumnLayout{
            anchors.fill: parent
            ColumnLayout{
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 35
                Text{
                    text:"用于多设备文件夹同步"
                    font.pixelSize: 17
                    color:"#A9A9AC"
                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: 250
                    Layout.preferredHeight: 20
                }
                Text{
                    text:"MyFolder"
                    font.pixelSize: 25
                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: 250
                    Layout.preferredHeight: 25
                    font.bold: true
                }
            }
            ColumnLayout{
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 15
                Layout.topMargin: 15
                spacing: 15
                InputText {
                    placeholderText:"请输入用户名"
                     leadingIcon:Image{
                     source: "Icons/user.svg"
                     }
                    pixelSize: 16
                    placeholdertextcolor: "#97989D"
                    textcolor: "#1F2022"
                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: 250
                    Layout.preferredHeight: 45
                    radius:20
                }
                InputText {
                    placeholderText:"请输入密码"
                    type:TextInput.Password
                    leadingIcon:Image{
                    source: "Icons/password.svg"
                    }
                    pixelSize: 16
                     placeholdertextcolor:"#97989D"
                     textcolor: "#1F2022"
                    Layout.alignment: Qt.AlignLeft
                    Layout.preferredWidth: 250
                    Layout.preferredHeight: 45
                    radius:20
                }
            }
            ColumnLayout{
                Layout.alignment: Qt.AlignHCenter
                spacing: 10
                Layout.bottomMargin: 60
                MyButton{
                    //用户名和密码没有被输入的时候变灰
                    //点击确认后变为转圈
                    buttontext:"登录"
                    backgroundcolor: "#4ACE94"
                    pixelSize:17
                    textcolor: "#E7FFFD"
                    hovercolor:"#3CB87F"
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 250
                    onClickedFunc:function() {
                        //发送用户名和密码
                        //等待服务器验证期间变为转圈
                        var comp = Qt.createComponent("mainWindow.qml");
                        if (comp.status === Component.Ready) {
                            var newW = comp.createObject();
                            newW.show();
                            root.close();
                        } else {
                            console.log("加载失败:", comp.errorString());
                        }
                    }
                }
                MyButton{
                    //等待服务器验证期间变为灰色
                    buttontext:"注册"
                    backgroundcolor: "#4ACE94"
                    pixelSize:17
                    textcolor: "#E7FFFD"
                    hovercolor:"#3CB87F"
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: 250
                }
            }



        }
    }
    Button{
        id:closeButton
        anchors.right: maincontainer.right
        anchors.top: maincontainer.top
        anchors.rightMargin: 5
        anchors.topMargin: 5
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
