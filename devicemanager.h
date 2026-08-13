#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class DeviceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceIdentityChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceIdentityChanged)
    Q_PROPERTY(QString deviceToken READ deviceToken NOTIFY credentialsChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool registered READ registered NOTIFY registeredChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit DeviceManager(QObject *parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }
    QString authToken() const { return m_authToken; }
    QString deviceId() const { return m_deviceId; }
    QString deviceName() const { return m_deviceName; }
    QString deviceToken() const { return m_deviceToken; }
    QVariantList devices() const { return m_devices.toVariantList(); }
    bool registered() const { return m_registered; }
    bool busy() const { return m_pendingRequests > 0; }
    QString lastError() const { return m_lastError; }

    void setBaseUrl(const QString &baseUrl);
    void setAuthToken(const QString &authToken);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop(bool markOffline = false);
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void sendHeartbeat();
    Q_INVOKABLE void updateDeviceName(const QString &deviceName);
    Q_INVOKABLE void applyWebSocketDevices(const QJsonArray &devices);

signals:
    void baseUrlChanged();
    void authTokenChanged();
    void deviceIdentityChanged();
    void credentialsChanged();
    void devicesChanged();
    void registeredChanged();
    void busyChanged();
    void lastErrorChanged();
    void credentialsReady(const QString &deviceId, const QString &deviceToken);
    void authenticationRequired();
    void deviceNameUpdateFinished(bool success, const QString &message);

private:
    void registerDevice();
    void setDevices(const QJsonArray &devices);
    void setLastError(const QString &error);
    void beginRequest();
    void endRequest();
    void applyRequestDefaults(QNetworkRequest &request) const;
    QString credentialFilePath() const;
    QString loadStoredToken() const;
    bool saveStoredToken(const QString &token) const;
    static QString localIpv4Address();
    static QJsonObject parseObject(const QByteArray &data);

    QNetworkAccessManager m_network;
    QTimer m_heartbeatTimer;
    QString m_baseUrl;
    QString m_authToken;
    QString m_deviceId;
    QString m_deviceName;
    QString m_deviceToken;
    QJsonArray m_devices;
    QString m_lastError;
    int m_pendingRequests = 0;
    bool m_registered = false;
    bool m_stopping = false;
};

#endif
