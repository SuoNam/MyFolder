#include "devicemanager.h"

#include "deviceidentifier.h"
#include "serverconfig.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

namespace {
constexpr int RequestTimeoutMs = 30000;
constexpr int HeartbeatIntervalMs = 30000;
}

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    setBaseUrl(MyFolderServerConfig::baseUrl());
    m_deviceId = DeviceIdentifier::getLinuxDeviceId();
    m_deviceName = QSettings().value("device/customName", DeviceIdentifier::getHostName()).toString().trimmed();
    if (m_deviceName.isEmpty()) m_deviceName = DeviceIdentifier::getHostName();
    m_heartbeatTimer.setInterval(HeartbeatIntervalMs);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &DeviceManager::sendHeartbeat);
}

void DeviceManager::updateDeviceName(const QString &deviceName)
{
    const QString normalized = deviceName.trimmed();
    if (normalized.isEmpty() || normalized.size() > 64) {
        emit deviceNameUpdateFinished(false, tr("Device name must contain 1 to 64 characters"));
        return;
    }
    if (!m_registered || m_deviceToken.isEmpty()) {
        emit deviceNameUpdateFinished(false, tr("Device is not registered"));
        return;
    }

    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(m_deviceId));
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/devices/" + encodedId));
    applyRequestDefaults(request);
    request.setRawHeader("X-Device-Token", m_deviceToken.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    beginRequest();
    QNetworkReply *reply = m_network.sendCustomRequest(
        request, "PATCH", QJsonDocument(QJsonObject{{"deviceName", normalized}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, normalized]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        endRequest();
        if (status != 200) {
            const QJsonObject error = parseObject(payload);
            const QString message = error.value("message").toString(
                error.value("code").toString(networkError));
            emit deviceNameUpdateFinished(false, message);
            return;
        }
        m_deviceName = normalized;
        QSettings().setValue("device/customName", normalized);
        emit deviceIdentityChanged();
        refreshDevices();
        emit deviceNameUpdateFinished(true, tr("Device name updated"));
    });
}

void DeviceManager::setBaseUrl(const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    if (m_baseUrl == normalized) {
        return;
    }
    m_baseUrl = normalized;
    emit baseUrlChanged();
}

void DeviceManager::setAuthToken(const QString &authToken)
{
    if (m_authToken == authToken) {
        return;
    }
    m_authToken = authToken;
    emit authTokenChanged();
    if (m_authToken.isEmpty()) {
        stop(false);
    }
}

void DeviceManager::start()
{
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty() || m_deviceId.isEmpty()) {
        setLastError(tr("Missing server address, JWT, or device identity"));
        return;
    }
    m_stopping = false;
    registerDevice();
}

void DeviceManager::stop(bool markOffline)
{
    m_stopping = true;
    m_heartbeatTimer.stop();
    if (markOffline && m_registered && !m_baseUrl.isEmpty() && !m_authToken.isEmpty()) {
        QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/devices/" +
                                     QString::fromUtf8(QUrl::toPercentEncoding(m_deviceId))));
        applyRequestDefaults(request);
        QNetworkReply *reply = m_network.deleteResource(request);
        connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }
    if (m_registered) {
        m_registered = false;
        emit registeredChanged();
    }
}

void DeviceManager::registerDevice()
{
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/devices"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonObject body{{"deviceId", m_deviceId},
                     {"deviceName", m_deviceName},
                     {"deviceType", "PC"},
                     {"os", QSysInfo::prettyProductName()},
                     {"clientVersion", QCoreApplication::applicationVersion()}};
    const QString address = localIpv4Address();
    if (!address.isEmpty()) {
        body["deviceAddress"] = address;
    }

    beginRequest();
    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QJsonObject response = parseObject(reply->readAll());
        const QString networkError = reply->errorString();
        reply->deleteLater();
        endRequest();

        if (m_stopping) {
            return;
        }
        if (status != 201) {
            if (status == 401) {
                emit authenticationRequired();
            }
            setLastError(response.value("code").toString().isEmpty()
                             ? networkError
                             : response.value("code").toString());
            return;
        }

        QString token = response.value("deviceToken").toString();
        if (token.isEmpty()) {
            token = loadStoredToken();
        }
        if (token.isEmpty()) {
            setLastError(tr("Device registration returned no device token"));
            return;
        }
        if (!saveStoredToken(token)) {
            setLastError(tr("Unable to securely persist the device token"));
            return;
        }

        m_deviceToken = token;
        emit credentialsChanged();
        if (!m_registered) {
            m_registered = true;
            emit registeredChanged();
        }
        setLastError(QString());
        m_heartbeatTimer.start();
        emit credentialsReady(m_deviceId, m_deviceToken);
        refreshDevices();
    });
}

void DeviceManager::refreshDevices()
{
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty()) {
        return;
    }
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/devices"));
    applyRequestDefaults(request);
    beginRequest();
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        endRequest();

        if (status == 200) {
            const QJsonDocument document = QJsonDocument::fromJson(payload);
            if (document.isArray()) {
                setDevices(document.array());
                setLastError(QString());
            } else {
                setLastError(tr("Invalid device list response"));
            }
        } else {
            const QJsonObject error = parseObject(payload);
            if (status == 401) {
                emit authenticationRequired();
            }
            setLastError(error.value("code").toString().isEmpty()
                             ? networkError
                             : error.value("code").toString());
        }
    });
}

void DeviceManager::sendHeartbeat()
{
    if (!m_registered || m_baseUrl.isEmpty() || m_authToken.isEmpty()) {
        return;
    }
    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(m_deviceId));
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/devices/" + encodedId + "/heartbeat"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonObject body{{"clientVersion", QCoreApplication::applicationVersion()}};
    const QString address = localIpv4Address();
    if (!address.isEmpty()) {
        body["deviceAddress"] = address;
    }
    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        reply->deleteLater();
        if (status == 200) {
            setLastError(QString());
            return;
        }
        const QJsonObject error = parseObject(payload);
        if (status == 401) {
            emit authenticationRequired();
        } else if (status == 404) {
            m_registered = false;
            emit registeredChanged();
            registerDevice();
        }
        setLastError(error.value("code").toString());
    });
}

void DeviceManager::applyWebSocketDevices(const QJsonArray &devices)
{
    setDevices(devices);
}

void DeviceManager::setDevices(const QJsonArray &devices)
{
    if (m_devices == devices) {
        return;
    }
    m_devices = devices;
    emit devicesChanged();
}

void DeviceManager::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void DeviceManager::beginRequest()
{
    const bool wasBusy = busy();
    ++m_pendingRequests;
    if (!wasBusy) {
        emit busyChanged();
    }
}

void DeviceManager::endRequest()
{
    const bool wasBusy = busy();
    m_pendingRequests = qMax(0, m_pendingRequests - 1);
    if (wasBusy != busy()) {
        emit busyChanged();
    }
}

void DeviceManager::applyRequestDefaults(QNetworkRequest &request) const
{
    request.setTransferTimeout(RequestTimeoutMs);
    request.setRawHeader("Authorization", m_authToken.toUtf8());
}

QString DeviceManager::credentialFilePath() const
{
    QString directoryPath = QProcessEnvironment::systemEnvironment().value(
        "MYFOLDER_CREDENTIAL_DIR",
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath("credentials"));
    QDir directory(directoryPath);
    if (!directory.exists() && !directory.mkpath(".")) {
        return QString();
    }
    QFile::setPermissions(directory.absolutePath(), QFileDevice::ReadOwner |
                                                       QFileDevice::WriteOwner |
                                                       QFileDevice::ExeOwner);
    const QByteArray key = QCryptographicHash::hash(
        (m_baseUrl + "\n" + m_deviceId).toUtf8(), QCryptographicHash::Sha256).toHex();
    return directory.filePath("device_" + QString::fromLatin1(key) + ".json");
}

QString DeviceManager::loadStoredToken() const
{
    QFile file(credentialFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return parseObject(file.readAll()).value("deviceToken").toString();
}

bool DeviceManager::saveStoredToken(const QString &token) const
{
    const QString path = credentialFilePath();
    if (path.isEmpty()) {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (!file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        file.cancelWriting();
        return false;
    }
    const QJsonObject payload{{"deviceId", m_deviceId}, {"deviceToken", token}};
    if (file.write(QJsonDocument(payload).toJson(QJsonDocument::Compact)) < 0 || !file.commit()) {
        return false;
    }
    return QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QString DeviceManager::localIpv4Address()
{
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        if (!(interface.flags() & QNetworkInterface::IsUp) ||
            !(interface.flags() & QNetworkInterface::IsRunning) ||
            (interface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                return entry.ip().toString();
            }
        }
    }
    return QString();
}

QJsonObject DeviceManager::parseObject(const QByteArray &data)
{
    const QJsonDocument document = QJsonDocument::fromJson(data);
    return document.isObject() ? document.object() : QJsonObject();
}
