pragma Singleton  // 必须在第一行
import QtQuick

QtObject {
    property string authToken: ""
    property string userName: ""
    property string dataFolder:""
    property bool isLoggedIn: false
    property var devicesList: []
    property string deviceId: ""
    property bool autoAcceptDeviceTransfers: false
}
