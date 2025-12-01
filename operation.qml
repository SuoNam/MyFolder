import QtQuick 2.15
import QtQuick.Controls
Rectangle{
    anchors.fill: parent
    anchors.margins: 10
    property string localPath;
    property var lsList: []
    property var stringlist:[]

    // Component.onCompleted: {

    // }
    Dialog {
        id: myDialog
        title: "新建"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string type: ""
        property string userInput : ""
        property  string func:""
        contentItem: Column {
            spacing: 15
            padding: 20

            TextField {
                id: inputField
                placeholderText: "请输入"+myDialog.type+"名字"
                text: myDialog.userInput
                onTextChanged: myDialog.userInput = text
                width: 250
            }
        }

        onAccepted: {
            stringlist=[localPath,userInput]
            console.log(stringlist)
            if (func=="touch") command.excel("/home/suo/Desktop/大三上/操作系统/fat12/version_2/touch",stringlist)
            if (func=="mkdir") command.excel("/home/suo/Desktop/大三上/操作系统/fat12/version_2/mkdir",stringlist)
            stringlist=[localPath,"/"]
            lsList=command.excel("/home/suo/Desktop/大三上/操作系统/fat12/version_2/ls",stringlist).filter(function(item) {
                return item.trim() !== "";
            })
        }
    }
    function add(name,func,placeholderText){
        myDialog.title=name
        myDialog.type=placeholderText
        myDialog.func=func
        myDialog.open()
    }

    ListModel{
        id:buttonlist
        ListElement{name:"新建文件";func:"touch";placeholderText:"文件"}
        ListElement{name:"新建文件夹";func:"mkdir";placeholderText:"文件夹"}
    }
    Column{
        anchors {
               top: parent.top
               bottom: parent.bottom
               left: parent.left
               right: parent.right
           }
        Rectangle{
            width:parent.width-20
            height:parent.height*0.1
                ListView{
                    anchors.fill:parent
                    model:buttonlist
                    spacing:10
                    orientation: ListView.Horizontal
                    delegate:Rectangle{
                        color:'transparent'
                        width:200
                        height:parent.height
                        anchors.verticalCenter:parent.verticalCenter
                        MyButton{
                            buttonRadius:20
                            pixelSize:28
                            backgroundcolor: "#52dca1"
                            textcolor:"#EFF2FB"
                            hovercolor:"#66e6b5"
                            buttontext:name
                            onClickedFunc:function(){
                                add(name,func,placeholderText)
                            }
                    }
                  }
                }
        }
        Rectangle{
            id:folderRec
            width:parent.width
            height:parent.height*0.6
            ListView {
                anchors.fill: parent
                model: lsList

                delegate: Rectangle {
                    width: parent.width
                    height: 60

                    Rectangle {
                        anchors.top: parent.top
                        width: parent.width
                        height: 1
                        color: "black"
                    }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        Image {
                            source: "Icons/file.svg"
                            width: 50
                            height: 50
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            height: parent.height-10
                            width: parent.width - 120
                            anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    text: modelData
                                    font.pixelSize: 20
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                }

                        }
                        Button{
                            width:60
                            height: 20
                            text:"删除"
                            anchors.verticalCenter: parent.verticalCenter
                            onClicked:{
                                stringlist=[localPath,modelData]
                                command.excel("/home/suo/Desktop/大三上/操作系统/fat12/version_2/rm",stringlist)
                                stringlist=[localPath,"/"]
                                lsList=command.excel("/home/suo/Desktop/大三上/操作系统/fat12/version_2/ls",stringlist).filter(function(item) {
                                    return item.trim() !== "";
                                })
                            }
                        }
                    }
                }
            }

        }

    }

}

