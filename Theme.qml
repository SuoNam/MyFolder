pragma Singleton
import QtQuick

// MyFolder "Signal Desk" theme.
// Channel colours are semantic: LAN green / P2P amber / RELAY blue. They encode
// the transport actually used and are never applied decoratively.
QtObject {
    // ink
    readonly property color ink:      "#0E211A"
    readonly property color ink2:     "#3A4F46"
    readonly property color muted:    "#75897F"
    readonly property color faint:    "#9DADA4"

    // surfaces
    readonly property color canvas:   "#EFF3F0"
    readonly property color surface:  "#FFFFFF"
    readonly property color sunken:   "#F6F9F7"
    readonly property color line:     "#DBE5DF"
    readonly property color lineSoft: "#EAF1ED"

    // brand signal
    readonly property color signal:     "#1F9D72"
    readonly property color signalDeep: "#14724F"
    readonly property color signalWash: "#E5F5EE"
    readonly property color signalEdge: "#C2E6D6"

    // channels
    readonly property color p2p:       "#BD7217"
    readonly property color p2pWash:   "#FBF0E0"
    readonly property color p2pEdge:   "#EFD9B4"
    readonly property color relay:     "#48699C"
    readonly property color relayWash: "#E9EFF7"
    readonly property color relayEdge: "#CBD9EC"

    // alert
    readonly property color alert:     "#AD443B"
    readonly property color alertWash: "#FBEBE9"
    readonly property color alertEdge: "#ECCEC9"

    readonly property int radius:   8
    readonly property int radiusSm: 5

    readonly property string uiFont:   Qt.platform.os === "windows" ? "Microsoft YaHei UI" : "PingFang SC"
    readonly property string dataFont: Qt.platform.os === "windows" ? "Cascadia Mono" : "Menlo"

    function channelColor(ch) {
        if (ch === "LAN") return signal
        if (ch === "P2P") return p2p
        return relay
    }
    function channelWash(ch) {
        if (ch === "LAN") return signalWash
        if (ch === "P2P") return p2pWash
        return relayWash
    }
    function channelEdge(ch) {
        if (ch === "LAN") return signalEdge
        if (ch === "P2P") return p2pEdge
        return relayEdge
    }

    function formatBytes(bytes) {
        if (bytes === undefined || bytes === null || bytes < 0) return "-"
        if (bytes < 1024) return bytes + " B"
        var units = ["KB", "MB", "GB", "TB"]
        var v = bytes
        for (var i = 0; i < units.length; ++i) {
            v = v / 1024
            if (v < 1024 || i === units.length - 1)
                return (v >= 100 ? Math.round(v) : v.toFixed(1)) + " " + units[i]
        }
        return "-"
    }
}
