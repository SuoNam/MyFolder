import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore
ApplicationWindow{

    width:1500
    height:900

    title: qsTr("MyFolder")
    flags:Qt.FramelessWindowHint
    color:'transparent'
    visible: true
    id:root
    property string currentSource
    property var stringList: []
    property var lsList:[]
    property string currentPath;

    function operation(localPath){
        //读取这个文件中的内容 展示出对应的根目录
        stringList=[currentPath,localPath,"/"]
        lsList=command.excel("/home/suo/Desktop/大三上/操作系统/fat12/version_2/ls",stringList).filter(function(item) {
            return item.trim() !== "";
        })
        //TODO::给右侧操作框加qml
        contentLoader.source="operation.qml"
        contentLoader.item.lsList=lsList
        contentLoader.item.localPath=localPath

    }

    property var buttonFuncMap:({
        "fatformat12":fat12.format,
        "operation":operation
    })

    FileDialog {
        id: fileDialog
        title: "选择img镜像"
        nameFilters: ["(*img)"]
        property  string func
        onAccepted:{
        currentSource = fileDialog.selectedFile
        var localPath = Qt.resolvedUrl(currentSource).toString().replace("file://", "")
        buttonFuncMap[func](localPath);
        }
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
                Rectangle{
                    width:parent.width*0.2
                    height:parent.height
                    color: "#4ACE94";
                    Column{
                        anchors{
                            top: parent.top
                            bottom: parent.bottom
                            left: parent.left
                            right: parent.right
                        }
                        Rectangle{
                        width:parent.width
                        height:150
                        color:'transparent'
                        Text{
                            text:"Tools"
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin:20
                            font.bold: true
                            font.pixelSize: 30
                            color:"#EFF2FB"
                        }
                        }
                        Rectangle{
                            width:parent.width
                            height:parent.height-150
                            color:'transparent'
                            //按钮列表
                            ListModel{
                             id:toolsModel
                             ListElement{name:"fat12格式化";func:"fatformat12"}
                             ListElement{name:"对img/硬盘进行操作";func:"operation"}
                             ListElement{name:"创建共享文件夹"}
                            }
                            ListView{
                              anchors.fill: parent
                              model:toolsModel
                              spacing:10
                              delegate:
                                  Rectangle{
                                    color:'transparent'
                                    width:parent.width-20
                                    height:75
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    MyButton{
                                        buttonRadius:20
                                        pixelSize:28
                                        backgroundcolor: "#52dca1"
                                        textcolor:"#EFF2FB"
                                        hovercolor:"#66e6b5"
                                        buttontext:name
                                        onClickedFunc:function(){
                                            fileDialog.open();
                                            fileDialog.func=func
                                        }
                                }
                              }
                            }
                        }

                        }
                    }
                Rectangle{
                    width:parent.width*0.8
                    height:parent.height
                    gradient:
                        Gradient{
                        GradientStop{position: 0.0;color:"#EFF2FB"}
                        GradientStop{position: 1.0;color:"#FFFCFF"}
                    }
                    Column{
                        width:parent.width;
                        height:parent.height;
                        Rectangle{
                            width:parent.width;
                            height:140;
                            color:'transparent'
                            Text{
                                text:"Myfolder"
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin:20
                                font.bold: true
                                font.pixelSize: 30
                            }
                        }
                        Rectangle{
                        //对应qml展示区
                            width:parent.width;
                            height:parent.height-140;
                            color:"transparent"
                            Rectangle{
                                anchors.centerIn: parent
                                width:parent.width-10;
                                height: parent.height;
                                radius:10
                                border.width: 2

                                Loader{
                                    id: contentLoader
                                    anchors.fill: parent

                                }

                            }
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
