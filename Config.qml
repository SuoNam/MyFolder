pragma Singleton
import QtQuick

QtObject {
    readonly property string baseUrl: TransferManager.baseUrl
    readonly property string websocketurl: {
        if (baseUrl.startsWith("https://"))
            return "wss://" + baseUrl.slice(8) + "/device"
        if (baseUrl.startsWith("http://"))
            return "ws://" + baseUrl.slice(7) + "/device"
        return ""
    }
}
