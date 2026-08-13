pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 820; height: 650; minimumWidth: 700; minimumHeight: 610
    visible: true; title: qsTr("MyFolder")
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"

    property bool isLoading: false
    property string errorMessage: ""
    property string noticeMessage: ""
    property string mode: "login"
    property int codeCooldown: 0
    // Keep the three configured providers visible even when the discovery
    // request is delayed or temporarily fails during application startup.
    property var oauthProviders: [
        { "provider": "nyauth", "configured": true },
        { "provider": "google", "configured": true },
        { "provider": "github", "configured": true }
    ]
    property string workspaceErrorDetails: ""
    property bool workspaceLoaded: false
    property var workspaceWindow: null
    property var workspaceComponent: null
    property string queuedExternalAction: ""
    property string queuedExternalFilePath: ""

    function oauthParam(uri, name) {
        var match = new RegExp("[?&]" + name + "=([^&]+)").exec(uri)
        return match ? decodeURIComponent(match[1]) : ""
    }

    function handleExternalCommand(action, filePath) {
        if (action === "oauth-callback") {
            var code = root.oauthParam(filePath, "code")
            if (code.length > 0) { root.isLoading = true; HttpHandler.exchangeOAuthCode(code) }
            else {
                root.isLoading = false
                root.errorMessage = root.oauthParam(filePath, "oauthErrorDescription") || qsTr("OAuth 登录未完成，请重试")
            }
            root.show(); root.raise(); root.requestActivate()
            return
        }
        if (root.workspaceWindow) {
            root.workspaceWindow.show(); root.workspaceWindow.raise(); root.workspaceWindow.requestActivate()
            if (action !== "activate") root.workspaceWindow.handleStartupAction(action, filePath)
            return
        }
        root.queuedExternalAction = action
        root.queuedExternalFilePath = filePath
        root.show(); root.raise(); root.requestActivate()
    }

    function applyAccessToken(token, userName) {
        GlobalStatus.authToken = token; GlobalStatus.userName = userName
        HttpHandler.authToken = token
        TransferManager.baseUrl = Config.baseUrl; TransferManager.authToken = token
        DeviceManager.baseUrl = Config.baseUrl; DeviceManager.authToken = token
        ForwardManager.baseUrl = Config.baseUrl; ForwardManager.authToken = token
        RelayDownloadManager.baseUrl = Config.baseUrl; RelayDownloadManager.authToken = token
    }

    function finishWorkspaceLoad() {
        if (!root.workspaceComponent || root.workspaceComponent.status === Component.Loading) return
        workspaceLoadWatchdog.stop()
        root.isLoading = false
        if (root.workspaceComponent.status === Component.Error) {
            root.workspaceErrorDetails = root.workspaceComponent.errorString()
            root.errorMessage = qsTr("工作区加载失败，请重试")
            return
        }
        root.workspaceWindow = root.workspaceComponent.createObject()
        if (!root.workspaceWindow) { root.errorMessage = qsTr("无法创建工作区窗口"); return }
        root.workspaceWindow.logoutRequested.connect(root.returnToLogin)
        root.workspaceWindow.show(); root.workspaceLoaded = true
        var nextAction = root.queuedExternalAction.length > 0 ? root.queuedExternalAction : startupAction
        var nextFile = root.queuedExternalAction.length > 0 ? root.queuedExternalFilePath : startupFilePath
        if (nextAction !== "activate" && nextAction !== "oauth-callback") root.workspaceWindow.handleStartupAction(nextAction, nextFile)
        root.queuedExternalAction = ""; root.queuedExternalFilePath = ""
        DeviceManager.start(); sessionRefresh.start(); root.hide()
    }

    function returnToLogin() {
        sessionRefresh.stop()
        root.isLoading = false
        root.errorMessage = ""
        root.noticeMessage = qsTr("已退出登录")
        root.applyAccessToken("", "")
        var oldWorkspace = root.workspaceWindow
        root.workspaceWindow = null
        root.workspaceLoaded = false
        root.workspaceComponent = null
        root.show()
        root.raise()
        root.requestActivate()
        if (oldWorkspace) Qt.callLater(function() { oldWorkspace.destroy() })
    }

    function enterWorkspace(token, userName) {
        root.applyAccessToken(token, userName)
        if (root.workspaceWindow) { root.isLoading = false; return }
        root.isLoading = true; root.errorMessage = ""
        GlobalStatus.dataFolder = DeviceIdentifier.setLocalFolder()
        workspaceLoadWatchdog.restart()
        // MainWindow is compiled into the module and exposed through an inline
        // Component below, so entering the workspace no longer depends on a
        // runtime URL load or a statusChanged timing window.
        root.workspaceComponent = workspaceWindowFactory
        root.finishWorkspaceLoad()
    }

    function acceptSession(dataObj) {
        root.isLoading = false
        if (dataObj.status === 200 && dataObj.data) root.enterWorkspace(dataObj.data.accessToken || dataObj.data.token, dataObj.data.account || accountInput.text.trim())
        else root.errorMessage = dataObj.message || qsTr("登录失败，请重试")
    }

    function switchMode(next) {
        root.mode = next; root.errorMessage = ""; root.noticeMessage = ""
        passwordInput.text = ""; confirmInput.text = ""; codeInput.text = ""
    }

    function requestCode() {
        if (emailInput.text.trim().length === 0) { root.errorMessage = qsTr("请先填写邮箱"); return }
        root.isLoading = true; root.errorMessage = ""
        HttpHandler.sendEmailCode(emailInput.text.trim(), root.mode === "signup" ? "REGISTER" : "RESET_PASSWORD")
    }

    function submit() {
        if (root.isLoading) return
        root.errorMessage = ""; root.noticeMessage = ""
        if (root.mode === "login") {
            if (!accountInput.text.trim() || !passwordInput.text) { root.errorMessage = qsTr("请输入账号或邮箱和密码"); return }
            root.isLoading = true; HttpHandler.login(accountInput.text.trim(), passwordInput.text); return
        }
        if (!emailInput.text.trim() || codeInput.text.length !== 6 || passwordInput.text.length < 8) {
            root.errorMessage = qsTr("请填写邮箱、6 位验证码和至少 8 位密码"); return
        }
        if (passwordInput.text !== confirmInput.text) { root.errorMessage = qsTr("两次输入的密码不一致"); return }
        root.isLoading = true
        if (root.mode === "signup") {
            if (!accountInput.text.trim()) { root.isLoading = false; root.errorMessage = qsTr("请输入账号"); return }
            HttpHandler.signup(accountInput.text.trim(), passwordInput.text, emailInput.text.trim(), codeInput.text)
        } else HttpHandler.resetPassword(emailInput.text.trim(), codeInput.text, passwordInput.text)
    }

    Connections {
        target: HttpHandler
        function onSessionReady(accessToken, account) { root.enterWorkspace(accessToken, account) }
        function onLoginResult(dataObj) { if (dataObj.status !== 200) root.acceptSession(dataObj) }
        function onSignupResult(dataObj) { if (dataObj.status !== 200) root.acceptSession(dataObj) }
        function onRefreshResult(dataObj) {
            if (dataObj.status !== 200) {
                root.isLoading = false
                if (!root.workspaceWindow) root.errorMessage = ""
            }
        }
        function onOAuthExchangeResult(dataObj) { if (dataObj.status !== 200) root.acceptSession(dataObj) }
        function onEmailCodeResult(dataObj) {
            root.isLoading = false
            if (dataObj.status === 200) { root.noticeMessage = qsTr("验证码已发送，10 分钟内有效"); root.codeCooldown = 60; cooldownTimer.start() }
            else root.errorMessage = dataObj.message || qsTr("验证码发送失败")
        }
        function onPasswordResetResult(dataObj) {
            root.isLoading = false
            if (dataObj.status === 200 && root.mode === "forgot") { root.switchMode("login"); root.noticeMessage = qsTr("密码已重置，请重新登录") }
            else if (root.mode === "forgot") root.errorMessage = dataObj.message || qsTr("密码重置失败")
        }
        function onOAuthProvidersResult(dataObj) {
            if (dataObj.status === 200 && dataObj.data && dataObj.data.length > 0)
                root.oauthProviders = dataObj.data
        }
        function onOAuthStartResult(dataObj) {
            if (dataObj.status === 200) {
                // Leave the browser unobstructed while Google/GitHub/Nyauth
                // displays account selection or consent. The custom protocol
                // callback restores this window after the user finishes.
                root.hide()
            } else {
                root.isLoading = false
                root.errorMessage = dataObj.message || qsTr("无法开始第三方登录")
            }
        }
    }

    Timer { id: cooldownTimer; interval: 1000; repeat: true; onTriggered: { root.codeCooldown--; if (root.codeCooldown <= 0) stop() } }
    Timer { id: sessionRefresh; interval: 90 * 60 * 1000; repeat: true; onTriggered: HttpHandler.refreshSession() }
    Component { id: workspaceWindowFactory; MainWindow {} }
    Timer {
        id: workspaceLoadWatchdog
        interval: 12000
        repeat: false
        onTriggered: {
            if (!root.workspaceLoaded && root.isLoading) {
                root.isLoading = false
                root.errorMessage = qsTr("工作区加载超时，请重新打开客户端")
            }
        }
    }

    Component.onCompleted: {
        HttpHandler.loadOAuthProviders()
        if (startupAction === "oauth-callback") root.handleExternalCommand(startupAction, startupFilePath)
        else if (HttpHandler.hasStoredSession) { root.isLoading = true; HttpHandler.refreshStoredSession() }
    }

    Rectangle {
        anchors.fill: parent; radius: 14; color: Theme.surface; border.width: 1; border.color: "#C3D2CB"; clip: true
        ToolButton {
            z: 100
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            implicitWidth: 34
            implicitHeight: 30
            text: "×"
            onClicked: Qt.quit()
            contentItem: Text {
                text: parent.text
                color: parent.hovered ? "#FFFFFF" : "#9DB8AC"
                font.pixelSize: 17
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { radius: 6; color: parent.hovered ? Theme.alert : "transparent" }
        }
        RowLayout {
            anchors.fill: parent; spacing: 0
            Item {
                Layout.fillHeight: true; Layout.preferredWidth: parent.width * 0.55
                MouseArea { anchors.fill: parent; onPressed: root.startSystemMove() }
                ColumnLayout {
                    anchors.centerIn: parent; width: Math.min(parent.width - 78, 350); spacing: 11
                    RowLayout {
                        spacing: 9
                        Image { source:"qrc:/qt/qml/MyTest/Icons/app.png"; sourceSize.width:52; sourceSize.height:52; Layout.preferredWidth:26; Layout.preferredHeight:26; fillMode:Image.PreserveAspectFit; smooth:true }
                        Text { text:"MyFolder"; font.pixelSize:15; font.bold:true; color:Theme.ink }
                        Item { Layout.fillWidth:true }
                        Text { text:"1.1.1"; font.family:Theme.dataFont; font.pixelSize:10; color:Theme.faint }
                    }
                    ColumnLayout {
                        spacing:3; Layout.topMargin:6
                        Text { text:root.mode === "login" ? qsTr("欢迎回来") : root.mode === "signup" ? qsTr("创建账号") : qsTr("找回密码"); font.pixelSize:25; font.bold:true; color:Theme.ink }
                        Text { text:root.mode === "login" ? qsTr("登录一次，这台设备会长期保持会话。") : qsTr("通过邮箱验证码确认身份。"); font.pixelSize:12; color:Theme.muted }
                    }
                    Rectangle {
                        visible:root.errorMessage.length>0 || root.noticeMessage.length>0; Layout.fillWidth:true; implicitHeight:38; radius:6
                        color:root.errorMessage.length>0?Theme.alertWash:Theme.signalWash; border.width:1; border.color:root.errorMessage.length>0?Theme.alertEdge:Theme.signalEdge
                        Text { anchors.fill:parent; anchors.margins:10; text:root.errorMessage || root.noticeMessage; color:root.errorMessage.length>0?Theme.alert:Theme.signalDeep; font.pixelSize:11; elide:Text.ElideRight; verticalAlignment:Text.AlignVCenter }
                    }
                    ColumnLayout {
                        Layout.fillWidth:true; spacing:6
                        Text { visible:root.mode!=="forgot"; text:root.mode==="login"?qsTr("账号或邮箱"):qsTr("账号"); font.pixelSize:12; color:Theme.ink2 }
                        UiInput { id:accountInput; visible:root.mode!=="forgot"; Layout.fillWidth:true; implicitHeight:38; placeholderText:root.mode==="login"?qsTr("账号或邮箱"):qsTr("2–30 个字符") }
                        Text { visible:root.mode!=="login"; text:qsTr("邮箱"); font.pixelSize:12; color:Theme.ink2; Layout.topMargin:4 }
                        UiInput { id:emailInput; visible:root.mode!=="login"; Layout.fillWidth:true; implicitHeight:38; placeholderText:"name@example.com" }
                        Text { visible:root.mode!=="login"; text:qsTr("邮箱验证码"); font.pixelSize:12; color:Theme.ink2; Layout.topMargin:4 }
                        RowLayout {
                            visible:root.mode!=="login"; Layout.fillWidth:true; spacing:7
                            UiInput { id:codeInput; Layout.fillWidth:true; implicitHeight:38; maximumLength:6; placeholderText:qsTr("6 位验证码") }
                            UiButton { implicitWidth:105; implicitHeight:38; text:root.codeCooldown>0?root.codeCooldown+"s":qsTr("获取验证码"); enabled:!root.isLoading&&root.codeCooldown<=0; onClicked:root.requestCode() }
                        }
                        Text { text:root.mode==="forgot"?qsTr("新密码"):qsTr("密码"); font.pixelSize:12; color:Theme.ink2; Layout.topMargin:4 }
                        UiInput { id:passwordInput; Layout.fillWidth:true; implicitHeight:38; echoMode:TextInput.Password; placeholderText:root.mode==="login"?qsTr("请输入密码"):qsTr("至少 8 个字符"); onAccepted:root.submit() }
                        Text { visible:root.mode!=="login"; text:qsTr("确认密码"); font.pixelSize:12; color:Theme.ink2; Layout.topMargin:4 }
                        UiInput { id:confirmInput; visible:root.mode!=="login"; Layout.fillWidth:true; implicitHeight:38; echoMode:TextInput.Password }
                    }
                    UiButton { Layout.fillWidth:true; implicitHeight:40; kind:"primary"; text:root.isLoading?qsTr("正在处理…"):root.mode==="login"?qsTr("登录"):root.mode==="signup"?qsTr("注册并登录"):qsTr("重置密码"); enabled:!root.isLoading; onClicked:root.submit() }
                    RowLayout {
                        Layout.fillWidth:true
                        UiButton { kind:"quiet"; text:root.mode==="login"?qsTr("忘记密码"):qsTr("返回登录"); onClicked:root.switchMode(root.mode==="login"?"forgot":"login") }
                        Item { Layout.fillWidth:true }
                        UiButton { visible:root.mode==="login"; kind:"quiet"; text:qsTr("创建账号"); onClicked:root.switchMode("signup") }
                    }
                    Rectangle { visible:root.mode==="login"; Layout.fillWidth:true; implicitHeight:1; color:Theme.lineSoft; Layout.topMargin:2 }
                    RowLayout {
                        visible:root.mode==="login"; Layout.fillWidth:true; spacing:6
                        Repeater {
                            model:root.oauthProviders
                            UiButton { required property var modelData; Layout.fillWidth:true; text:modelData.provider==="nyauth"?"Nyauth":modelData.provider==="google"?"Google":"GitHub"; enabled:modelData.configured&&!root.isLoading; onClicked:{root.isLoading=true;HttpHandler.startOAuth(modelData.provider)} }
                        }
                    }
                }
            }
            Rectangle {
                Layout.fillHeight:true; Layout.fillWidth:true; color:"#0E211A"
                ColumnLayout {
                    anchors.fill:parent; anchors.margins:36; spacing:16
                    Item { Layout.fillHeight:true }
                    Text { text:"IDENTITY"; font.family:Theme.dataFont; font.pixelSize:10; font.letterSpacing:1.6; color:"#4E7A68" }
                    Text { Layout.fillWidth:true; text:qsTr("一次验证，\n连接每台设备。"); font.pixelSize:28; font.bold:true; lineHeight:1.14; color:"white"; wrapMode:Text.WordWrap }
                    Text { Layout.fillWidth:true; text:qsTr("邮箱用于找回密码和确认敏感操作；Nyauth、Google、GitHub 可以作为独立登录方式。"); font.pixelSize:12; lineHeight:1.5; color:"#9DC0B2"; wrapMode:Text.WordWrap }
                    Rectangle { Layout.fillWidth:true; implicitHeight:1; color:"#1FFFFFFF"; Layout.topMargin:8 }
                    Text { text:qsTr("PC 会话长期有效，但可以在退出登录、密码重置或令牌泄漏时立即撤销。"); Layout.fillWidth:true; font.pixelSize:11; color:"#77A692"; wrapMode:Text.WordWrap }
                }
            }
        }
    }
}
