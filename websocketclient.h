#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QWebSocket>
#include <QtQml/qqmlregistration.h>

class WebSocketClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit WebSocketClient(QObject *parent = nullptr);

    bool connected() const { return m_webSocket.state() == QAbstractSocket::ConnectedState; }
    QString connectionState() const { return m_connectionState; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void connectTo(const QString &baseUrl, const QString &authToken,
                               const QString &deviceId, const QString &deviceToken);
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void reconnect();
    Q_INVOKABLE bool send(const QString &action, const QJsonObject &payload = QJsonObject());
    Q_INVOKABLE void requestDeviceList();

signals:
    void connectedChanged();
    void connectionStateChanged();
    void lastErrorChanged();
    void helloAcknowledged(const QJsonObject &device);
    void heartbeatAcknowledged(const QJsonObject &device);
    void deviceListReceived(const QJsonArray &devices);
    void forwardEventReceived(const QString &action, const QString &forwardId,
                              const QJsonObject &payload);
    void protocolError(const QString &message);
    void reconnectExhausted();

private slots:
    void openSocket();
    void onConnected();
    void onTextMessageReceived(const QString &message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    void scheduleReconnect();
    void setConnectionState(const QString &state);
    void setLastError(const QString &error);
    static QString websocketUrl(const QString &baseUrl, const QString &deviceId);

    QWebSocket m_webSocket;
    QTimer m_heartbeatTimer;
    QTimer m_reconnectTimer;
    QString m_baseUrl;
    QString m_authToken;
    QString m_deviceId;
    QString m_deviceToken;
    QString m_connectionState = "DISCONNECTED";
    QString m_lastError;
    int m_reconnectAttempt = 0;
    bool m_manualDisconnect = true;
};

#endif
