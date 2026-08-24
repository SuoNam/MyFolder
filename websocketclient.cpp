#include "websocketclient.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QUuid>

namespace {
constexpr int HeartbeatIntervalMs = 25000;
constexpr int HeartbeatAckTimeoutMs = 15000;
constexpr int ConnectTimeoutMs = 20000;
constexpr int MaxReconnectDelayMs = 30000;
}

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);

    m_heartbeatTimer.setInterval(HeartbeatIntervalMs);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        if (!send("heartbeat", QJsonObject())) {
            failCurrentConnection(tr("实时心跳发送失败"));
            return;
        }
        m_heartbeatAckTimer.start();
    });
    m_heartbeatAckTimer.setSingleShot(true);
    m_heartbeatAckTimer.setInterval(HeartbeatAckTimeoutMs);
    connect(&m_heartbeatAckTimer, &QTimer::timeout, this, [this]() {
        failCurrentConnection(tr("实时心跳响应超时"));
    });
    m_connectTimer.setSingleShot(true);
    m_connectTimer.setInterval(ConnectTimeoutMs);
    connect(&m_connectTimer, &QTimer::timeout, this, [this]() {
        failCurrentConnection(tr("实时服务连接超时"));
    });
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::openSocket);
}

void WebSocketClient::connectTo(const QString &baseUrl, const QString &authToken,
                                const QString &deviceId, const QString &deviceToken)
{
    if (baseUrl.trimmed().isEmpty() || authToken.isEmpty() ||
        deviceId.isEmpty() || deviceToken.isEmpty()) {
        setLastError(tr("Missing WebSocket credentials"));
        return;
    }

    m_manualDisconnect = true;
    m_reconnectTimer.stop();
    m_heartbeatTimer.stop();
    m_heartbeatAckTimer.stop();
    m_connectTimer.stop();
    m_webSocket.abort();
    m_baseUrl = baseUrl.trimmed();
    m_authToken = authToken;
    m_deviceId = deviceId;
    m_deviceToken = deviceToken;
    m_reconnectAttempt = 0;
    m_manualDisconnect = false;
    openSocket();
}

void WebSocketClient::disconnectFromServer()
{
    m_manualDisconnect = true;
    m_reconnectTimer.stop();
    m_heartbeatTimer.stop();
    m_heartbeatAckTimer.stop();
    m_connectTimer.stop();
    if (m_webSocket.state() == QAbstractSocket::UnconnectedState) {
        setConnectionState("DISCONNECTED");
    } else {
        m_webSocket.close(QWebSocketProtocol::CloseCodeNormal, "client shutdown");
    }
}

void WebSocketClient::reconnect()
{
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty() || m_deviceId.isEmpty() ||
        m_deviceToken.isEmpty()) {
        setLastError(tr("Missing WebSocket credentials"));
        setConnectionState("FAILED");
        return;
    }
    m_manualDisconnect = true;
    m_reconnectTimer.stop();
    m_heartbeatTimer.stop();
    m_heartbeatAckTimer.stop();
    m_connectTimer.stop();
    m_webSocket.abort();
    m_reconnectAttempt = 0;
    m_manualDisconnect = false;
    openSocket();
}

bool WebSocketClient::send(const QString &action, const QJsonObject &payload)
{
    if (!connected() || action.trimmed().isEmpty()) {
        return false;
    }
    const QJsonObject message{{"action", action.trimmed()},
                              {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                              {"msgId", QUuid::createUuid().toString(QUuid::WithoutBraces)},
                              {"payload", payload}};
    return m_webSocket.sendTextMessage(
               QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact))) > 0;
}

void WebSocketClient::requestDeviceList()
{
    send("device.list", QJsonObject());
}

void WebSocketClient::openSocket()
{
    if (m_manualDisconnect) {
        return;
    }
    if (m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        return;
    }
    const QString socketUrl = websocketUrl(m_baseUrl, m_deviceId);
    if (socketUrl.isEmpty()) {
        setLastError(tr("Invalid WebSocket server URL"));
        return;
    }
    QNetworkRequest request{QUrl(socketUrl)};
    request.setRawHeader("Authorization", m_authToken.toUtf8());
    // QWebSocket/QNetworkAccessManager may reserve or rewrite Authorization
    // during its authentication flow. The dedicated header is passed through
    // unchanged and is preferred by MyFolder Server v1.1.1+.
    request.setRawHeader("X-MyFolder-Authorization", m_authToken.toUtf8());
    request.setRawHeader("X-Device-Token", m_deviceToken.toUtf8());
    setConnectionState("CONNECTING");
    m_connectTimer.start();
    m_webSocket.open(request);
}

void WebSocketClient::onConnected()
{
    m_connectTimer.stop();
    setLastError(QString());
    setConnectionState("CONNECTED");
    emit connectedChanged();
    m_heartbeatAckTimer.stop();
    m_heartbeatTimer.start();
    if (!send("hello", QJsonObject())) {
        failCurrentConnection(tr("实时握手消息发送失败"));
        return;
    }
    requestDeviceList();
}

void WebSocketClient::onTextMessageReceived(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit protocolError(tr("Invalid WebSocket JSON message"));
        return;
    }
    const QJsonObject root = document.object();
    const QString action = root.value("action").toString();
    const QJsonValue payloadValue = root.value("payload");
    if (action == "hello.ack" && payloadValue.isObject()) {
        emit helloAcknowledged(payloadValue.toObject());
    } else if (action == "heartbeat.ack" && payloadValue.isObject()) {
        // A successful handshake alone is not proof that the session is stable.
        // Waiting for a heartbeat round-trip prevents an immediate server-side
        // close from resetting the retry counter forever.
        m_heartbeatAckTimer.stop();
        m_reconnectAttempt = 0;
        emit heartbeatAcknowledged(payloadValue.toObject());
    } else if (action == "device.list" && payloadValue.isArray()) {
        emit deviceListReceived(payloadValue.toArray());
    } else if (action.startsWith("task.forward.") && payloadValue.isObject()) {
        const QJsonObject payload = payloadValue.toObject();
        const QString forwardId = payload.value("forwardId").toString();
        if (forwardId.isEmpty()) {
            emit protocolError(tr("Forward event has no forwardId"));
            return;
        }
        emit forwardEventReceived(action, forwardId, payload);
    }
}

void WebSocketClient::onDisconnected()
{
    m_connectTimer.stop();
    m_heartbeatTimer.stop();
    m_heartbeatAckTimer.stop();
    emit connectedChanged();
    setConnectionState("DISCONNECTED");
    if (!m_manualDisconnect) {
        scheduleReconnect();
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    const QString detail = m_webSocket.errorString();
    if (detail.contains("status code: 401")) {
        setLastError(tr("登录状态正在恢复"));
        emit authenticationRequired();
    } else if (detail.contains("status code: 403")) {
        setLastError(tr("设备凭据已失效，正在重新注册设备"));
        emit deviceCredentialsRejected();
    } else if (detail.contains("status code: 200")) {
        setLastError(tr("实时连接握手失败，请更新客户端后重试"));
    } else {
        setLastError(tr("实时服务暂时无法连接"));
    }
}

void WebSocketClient::failCurrentConnection(const QString &error)
{
    if (m_manualDisconnect) {
        return;
    }
    setLastError(error);
    m_connectTimer.stop();
    m_heartbeatTimer.stop();
    m_heartbeatAckTimer.stop();
    if (m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        m_webSocket.abort();
        setConnectionState("DISCONNECTED");
        scheduleReconnect();
        return;
    }
    setConnectionState("DISCONNECTED");
    scheduleReconnect();
}

void WebSocketClient::scheduleReconnect()
{
    if (m_reconnectTimer.isActive()) {
        return;
    }
    // Keep retrying for the lifetime of the authenticated session. A client
    // can be offline for hours (sleep, Wi-Fi roam, VPN/TUN restart); stopping
    // after a small fixed number of attempts leaves it permanently stale.
    const int exponent = qMin(m_reconnectAttempt, 5);
    const int delay = qMin(MaxReconnectDelayMs, 1000 * (1 << exponent));
    ++m_reconnectAttempt;
    setConnectionState("RECONNECTING");
    m_reconnectTimer.start(delay);
}

void WebSocketClient::setConnectionState(const QString &state)
{
    if (m_connectionState == state) {
        return;
    }
    m_connectionState = state;
    emit connectionStateChanged();
}

void WebSocketClient::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

QString WebSocketClient::websocketUrl(const QString &baseUrl, const QString &deviceId)
{
    QUrl url(baseUrl);
    if (!url.isValid() || url.host().isEmpty()) {
        return QString();
    }
    if (url.scheme() == "https") {
        url.setScheme("wss");
    } else if (url.scheme() == "http") {
        url.setScheme("ws");
    } else if (url.scheme() != "ws" && url.scheme() != "wss") {
        return QString();
    }
    url.setPath("/device");
    QUrlQuery query;
    query.addQueryItem("deviceId", deviceId);
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}
