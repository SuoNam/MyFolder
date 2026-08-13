#include "forwardmanager.h"
#include "serverconfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <algorithm>

namespace {
constexpr int RequestTimeoutMs = 30000;
}

ForwardManager::ForwardManager(QObject *parent)
    : QObject(parent)
{
    setBaseUrl(MyFolderServerConfig::baseUrl());
}

void ForwardManager::setBaseUrl(const QString &baseUrl)
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

void ForwardManager::setAuthToken(const QString &authToken)
{
    if (m_authToken == authToken) {
        return;
    }
    m_authToken = authToken;
    emit authTokenChanged();
    if (m_authToken.isEmpty() && !m_tasks.isEmpty()) {
        m_tasks.clear();
        emit tasksChanged();
    }
}

void ForwardManager::setCurrentDeviceId(const QString &deviceId)
{
    if (m_currentDeviceId == deviceId) {
        return;
    }
    m_currentDeviceId = deviceId;
    emit currentDeviceIdChanged();
}

void ForwardManager::setCurrentDeviceToken(const QString &deviceToken)
{
    if (m_currentDeviceToken == deviceToken) {
        return;
    }
    m_currentDeviceToken = deviceToken;
    emit currentDeviceTokenChanged();
}

QVariantList ForwardManager::tasks() const
{
    QList<QJsonObject> ordered = m_tasks.values();
    std::sort(ordered.begin(), ordered.end(), [](const QJsonObject &left, const QJsonObject &right) {
        return left.value("updatedAt").toString() > right.value("updatedAt").toString();
    });
    QVariantList result;
    result.reserve(ordered.size());
    for (const QJsonObject &task : ordered) {
        result.append(task.toVariantMap());
    }
    return result;
}

void ForwardManager::refreshTasks()
{
    sendRequest("list", "GET", "/api/v1/forwards");
}

void ForwardManager::queryTask(const QString &forwardId)
{
    if (forwardId.trimmed().isEmpty()) {
        return;
    }
    sendRequest("get", "GET", "/api/v1/forwards/" +
                QString::fromUtf8(QUrl::toPercentEncoding(forwardId)), QJsonObject(), forwardId);
}

void ForwardManager::createForward(const QString &targetDeviceId,
                                   const QString &destinationPath,
                                   bool deleteSource,
                                   const QString &channel,
                                   const QString &relayUploadId,
                                   const QVariantList &files)
{
    const QString normalizedChannel = channel.trimmed().toUpper();
    const QSet<QString> channels{"AUTO", "LAN", "P2P", "RELAY"};
    QString validationError;
    if (m_currentDeviceId.isEmpty() || targetDeviceId.trimmed().isEmpty() ||
        targetDeviceId == m_currentDeviceId || !isValidRelativePath(destinationPath.trimmed()) ||
        !channels.contains(normalizedChannel) || !validateFiles(files, &validationError) ||
        (normalizedChannel == "RELAY" && relayUploadId.trimmed().isEmpty()) ||
        ((normalizedChannel == "LAN" || normalizedChannel == "P2P") &&
         !relayUploadId.trimmed().isEmpty()) || deleteSource) {
        if (validationError.isEmpty()) {
            validationError = tr("Invalid forward task parameters");
        }
        setLastError(validationError);
        emit actionFailed("create", "INVALID_REQUEST", validationError);
        return;
    }

    QJsonArray forwardFiles;
    for (const QVariant &entry : files) {
        const QVariantMap file = entry.toMap();
        forwardFiles.append(QJsonObject{{"path", file.value("path").toString()},
                                        {"size", file.value("size").toLongLong()},
                                        {"sha256", file.value("sha256").toString()}});
    }

    QJsonObject body{{"sourceDeviceId", m_currentDeviceId},
                     {"targetDeviceId", targetDeviceId.trimmed()},
                     {"destinationPath", destinationPath.trimmed()},
                     {"deleteSource", false},
                     {"channel", normalizedChannel},
                     {"files", forwardFiles}};
    if (!relayUploadId.trimmed().isEmpty()) {
        body["relayUploadId"] = relayUploadId.trimmed();
    }
    sendRequest("create", "POST", "/api/v1/forwards", body);
}

void ForwardManager::accept(const QString &forwardId)
{
    lifecycleAction(forwardId, "accept");
}

void ForwardManager::startTransfer(const QString &forwardId)
{
    lifecycleAction(forwardId, "start");
}

void ForwardManager::reportProgress(const QString &forwardId, qint64 transferredBytes)
{
    if (transferredBytes < 0) {
        emit actionFailed("progress", "INVALID_REQUEST", tr("Transferred bytes cannot be negative"));
        return;
    }
    lifecycleAction(forwardId, "progress", {{"transferredBytes", transferredBytes}});
}

void ForwardManager::complete(const QString &forwardId)
{
    lifecycleAction(forwardId, "complete");
}

void ForwardManager::finishDownload(const QString &forwardId, qint64 totalBytes)
{
    if (forwardId.trimmed().isEmpty() || totalBytes < 0) {
        emit actionFailed("download-final-progress", "INVALID_REQUEST",
                          tr("Invalid completed download progress"));
        return;
    }
    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(forwardId));
    sendRequest("download-final-progress", "POST",
                "/api/v1/forwards/" + encodedId + "/progress",
                {{"transferredBytes", totalBytes}}, forwardId);
}

void ForwardManager::fail(const QString &forwardId, const QString &reason)
{
    lifecycleAction(forwardId, "fail", {{"reason", reason.trimmed()}});
}

void ForwardManager::cancel(const QString &forwardId)
{
    if (forwardId.trimmed().isEmpty()) {
        return;
    }
    sendRequest("cancel", "DELETE", "/api/v1/forwards/" +
                QString::fromUtf8(QUrl::toPercentEncoding(forwardId)), QJsonObject(), forwardId);
}

void ForwardManager::handleForwardEvent(const QString &action, const QJsonObject &payload)
{
    static const QSet<QString> supported{"task.forward.offer", "task.forward.accepted",
                                         "task.forward.completed", "task.forward.cancelled"};
    if (!supported.contains(action)) {
        return;
    }
    const QString forwardId = payload.value("forwardId").toString();
    if (!forwardId.isEmpty()) {
        queryTask(forwardId);
    }
}

bool ForwardManager::validatePath(const QString &path) const
{
    return isValidRelativePath(path.trimmed());
}

void ForwardManager::lifecycleAction(const QString &forwardId, const QString &action,
                                     const QJsonObject &body)
{
    if (forwardId.trimmed().isEmpty()) {
        return;
    }
    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(forwardId));
    sendRequest(action, "POST", "/api/v1/forwards/" + encodedId + "/" + action,
                body, forwardId);
}

void ForwardManager::sendRequest(const QString &operation, const QByteArray &method,
                                 const QString &path, const QJsonObject &body,
                                 const QString &forwardId)
{
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty() || m_currentDeviceId.isEmpty() ||
        m_currentDeviceToken.isEmpty()) {
        const QString message = tr("Missing server, JWT, or device credentials");
        setLastError(message);
        emit actionFailed(operation, "AUTH_REQUIRED", message);
        return;
    }

    QNetworkRequest request(QUrl(m_baseUrl + path));
    applyRequestDefaults(request);
    QNetworkReply *reply = nullptr;
    if (method == "GET") {
        reply = m_network.get(request);
    } else if (method == "DELETE") {
        reply = m_network.deleteResource(request);
    } else {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
        const QByteArray payload = body.isEmpty()
            ? QByteArray()
            : QJsonDocument(body).toJson(QJsonDocument::Compact);
        reply = m_network.post(request, payload);
    }

    beginRequest();
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, operation, forwardId]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        endRequest();

        const QJsonDocument document = QJsonDocument::fromJson(payload);
        const bool success = status == 200 || status == 201;
        if (!success) {
            handleError(operation, forwardId, status,
                        document.isObject() ? document.object() : QJsonObject(), networkError);
            return;
        }

        setLastError(QString());
        if (operation == "list" && document.isArray()) {
            QMap<QString, QJsonObject> refreshed;
            for (const QJsonValue &value : document.array()) {
                const QJsonObject task = value.toObject();
                const QString id = task.value("forwardId").toString();
                if (!id.isEmpty()) {
                    refreshed.insert(id, task);
                }
            }
            m_tasks = refreshed;
            emit tasksChanged();
            for (const QJsonObject &task : std::as_const(refreshed)) {
                emit taskUpdated(task.toVariantMap());
            }
            return;
        }
        if (!document.isObject()) {
            const QString message = tr("Invalid forward task response");
            setLastError(message);
            emit actionFailed(operation, "INVALID_RESPONSE", message);
            return;
        }
        const QJsonObject task = document.object();
        updateTask(task);
        if (operation == "create") {
            emit forwardCreated(task.value("forwardId").toString());
        } else if (operation == "accept" && task.value("state").toString() == "ACCEPTED") {
            lifecycleAction(task.value("forwardId").toString(), "start");
        } else if (operation == "download-final-progress") {
            lifecycleAction(forwardId, "complete");
        }
    });
}

void ForwardManager::updateTask(const QJsonObject &task)
{
    const QString forwardId = task.value("forwardId").toString();
    if (forwardId.isEmpty()) {
        return;
    }
    m_tasks[forwardId] = task;
    emit tasksChanged();
    emit taskUpdated(task.toVariantMap());
}

void ForwardManager::removeTask(const QString &forwardId)
{
    if (m_tasks.remove(forwardId) > 0) {
        emit tasksChanged();
    }
}

void ForwardManager::handleError(const QString &operation, const QString &forwardId,
                                 int status, const QJsonObject &error,
                                 const QString &networkError)
{
    const QString code = error.value("code").toString();
    const QString message = error.value("message").toString().isEmpty()
        ? networkError
        : error.value("message").toString();
    setLastError(code.isEmpty() ? message : code);
    if ((operation == "download-final-progress" || operation == "complete") &&
        !forwardId.isEmpty()) {
        emit finalizationFailed(forwardId, code.isEmpty() ? "NETWORK_ERROR" : code);
    }
    if (status == 401) {
        emit authenticationRequired();
    } else if (code == "DEVICE_NOT_FOUND") {
        emit devicesRefreshRequested();
    } else if (code == "FORWARD_NOT_FOUND") {
        removeTask(forwardId);
    } else if ((code == "TASK_STATE_CONFLICT" || code == "FORWARD_INCOMPLETE") &&
               !forwardId.isEmpty() && operation != "get") {
        queryTask(forwardId);
    }
    emit actionFailed(operation, code.isEmpty() ? "NETWORK_ERROR" : code, message);
}

void ForwardManager::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void ForwardManager::beginRequest()
{
    const bool wasBusy = busy();
    ++m_pendingRequests;
    if (!wasBusy) {
        emit busyChanged();
    }
}

void ForwardManager::endRequest()
{
    const bool wasBusy = busy();
    m_pendingRequests = qMax(0, m_pendingRequests - 1);
    if (wasBusy != busy()) {
        emit busyChanged();
    }
}

void ForwardManager::applyRequestDefaults(QNetworkRequest &request) const
{
    request.setTransferTimeout(RequestTimeoutMs);
    request.setRawHeader("Authorization", m_authToken.toUtf8());
    request.setRawHeader("X-Device-Id", m_currentDeviceId.toUtf8());
    request.setRawHeader("X-Device-Token", m_currentDeviceToken.toUtf8());
}

bool ForwardManager::validateFiles(const QVariantList &files, QString *error)
{
    static const QRegularExpression shaPattern("^[0-9a-f]{64}$");
    if (files.isEmpty()) {
        if (error) *error = tr("A forward task needs at least one file");
        return false;
    }
    QSet<QString> paths;
    for (const QVariant &entry : files) {
        const QVariantMap file = entry.toMap();
        const QString path = file.value("path").toString();
        const qint64 size = file.value("size").toLongLong();
        const QString sha256 = file.value("sha256").toString();
        if (!isValidRelativePath(path) || size < 0 || !shaPattern.match(sha256).hasMatch() ||
            paths.contains(path)) {
            if (error) *error = tr("The forward file manifest is invalid");
            return false;
        }
        paths.insert(path);
    }
    return true;
}

bool ForwardManager::isValidRelativePath(const QString &path)
{
    if (path.isEmpty() || path.startsWith('/') || path.endsWith('/') ||
        path.contains('\\') || path.contains("//") || path.contains(QChar::Null)) {
        return false;
    }
    const QStringList parts = path.split('/');
    if (path.size() >= 2 && path.at(0).isLetter() && path.at(1) == ':') {
        return false;
    }
    for (const QString &part : parts) {
        if (part.isEmpty() || part == "." || part == "..") {
            return false;
        }
    }
    return true;
}
