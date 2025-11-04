import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
ApplicationWindow {
    width: 1000
    height: 600
    color: "lightblue"
    property string currentSource: "file:///C:/Users/Lenovo/Pictures/test1.jpg"

    Button {
        text: "选择图片"
        onClicked: fileDialog.open()
        width:100;
        height: 50;
    }
    Button{
        x:100;
        text:"ROI识别"
         onClicked:function(){
         }
    }
    Button{
        x:200
        text: "OCR识别"
        onClicked:function(){
        ocr.test(currentSource)
        }
    }

    FileDialog {
        id: fileDialog
        title: "选择图片"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp)"]
        onAccepted: currentSource = fileDialog.selectedFile
    }


    Rectangle {
        x:100;
        y:50;
        width:1000;
        height:550;
           color: "#222"

           Image {
               id: img
               source:  currentSource;
               anchors.centerIn: parent
               fillMode: Image.PreserveAspectFit
               smooth: true
               scale: viewer.scale
               x: viewer.offsetX
               y: viewer.offsetY
           }

           MouseArea {
               id: viewer
               anchors.fill: parent
               property real scale: 1.0
               property real offsetX: 0
               property real offsetY: 0
               property point lastPos: Qt.point(0, 0)
               drag.target: null

               onPressed: function(mouse){lastPos = Qt.point(mouse.x, mouse.y)}

               onPositionChanged: function(mouse){
                   if (mouse.buttons & Qt.LeftButton) {
                       offsetX += mouse.x - lastPos.x
                       offsetY += mouse.y - lastPos.y
                       lastPos = Qt.point(mouse.x, mouse.y)
                   }
               }

               onWheel: function(wheel){
                   var oldScale = scale
                   scale *= (wheel.angleDelta.y > 0 ? 1.1 : 0.9)
                   // 限制缩放范围
                   scale = Math.max(0.1, Math.min(scale, 10.0))
                   // 缩放中心修正
                   offsetX = wheel.x - (wheel.x - offsetX) * (scale / oldScale)
                   offsetY = wheel.y - (wheel.y - offsetY) * (scale / oldScale)
               }
           }
       }

}
