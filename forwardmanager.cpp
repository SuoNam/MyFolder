#include "forwardmanager.h"
#include "serverconfig.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <algorithm>

namespace {
constexpr int RequestTimeoutMs = 30000;
constexpr qint64 AcceptedTransferTtlMs = 30 * 60 * 1000;
}

ForwardManager::ForwardManager(QObject *parent)
    : QObject(parent)
{
    setBaseUrl(MyFolderServerConfig::baseUrl());
    loadAcceptedTransfers();
    loadDismissedTasks();
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
    if (m_authToken.isEmpty()) {
        ++m_latestListRequestSequence;
        m_progressInFlight.clear();
        m_pendingProgress.clear();
        m_pendingFinalProgress.clear();
        m_autoAcceptingTaskIds.clear();
        m_taskMutationRevisions.clear();
    }
    if (m_authToken.isEmpty() && !m_tasks.isEmpty()) {
        m_tasks.clear();
        emit tasksChanged();
    }
    if (m_authToken.isEmpty() && !m_historyTasks.isEmpty()) {
        m_historyTasks.clear();
        emit historyTasksChanged();
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

void ForwardManager::refreshHistory()
{
    sendRequest("history", "GET", "/api/v1/forwards/history");
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
    createForwardWithDirectories(targetDeviceId, destinationPath, deleteSource, channel,
                                 relayUploadId, files, {});
}

void ForwardManager::createForwardWithDirectories(const QString &targetDeviceId,
                                                   const QString &destinationPath,
                                                   bool deleteSource,
                                                   const QString &channel,
                                                   const QString &relayUploadId,
                                                   const QVariantList &files,
                                                   const QVariantList &directories)
{
    const QString normalizedChannel = channel.trimmed().toUpper();
    const QSet<QString> channels{"AUTO", "LAN", "P2P", "RELAY"};
    QString validationError;
    if (m_currentDeviceId.isEmpty() || targetDeviceId.trimmed().isEmpty() ||
        targetDeviceId == m_currentDeviceId || !isValidRelativePath(destinationPath.trimmed()) ||
        !channels.contains(normalizedChannel) || !validateFiles(files, &validationError) ||
        !validateDirectories(directories, &validationError) ||
        (files.isEmpty() && directories.isEmpty()) ||
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
    QJsonArray forwardDirectories;
    for (const QVariant &entry : directories) {
        forwardDirectories.append(entry.toString());
    }

    QJsonObject body{{"sourceDeviceId", m_currentDeviceId},
                     {"targetDeviceId", targetDeviceId.trimmed()},
                     {"destinationPath", destinationPath.trimmed()},
                     {"deleteSource", false},
                     {"channel", normalizedChannel},
                     {"files", forwardFiles},
                     {"directories", forwardDirectories}};
    if (!relayUploadId.trimmed().isEmpty()) {
        body["relayUploadId"] = relayUploadId.trimmed();
    }
    sendRequest("create", "POST", "/api/v1/forwards", body);
}

void ForwardManager::accept(const QString &forwardId)
{
    lifecycleAction(forwardId, "accept");
}

void ForwardManager::reject(const QString &forwardId)
{
    lifecycleAction(forwardId, "reject");
}

void ForwardManager::startTransfer(const QString &forwardId)
{
    lifecycleAction(forwardId, "start");
}

void ForwardManager::reportProgress(const QString &forwardId, qint64 transferredBytes)
{
    const QString id = forwardId.trimmed();
    if (id.isEmpty() || transferredBytes < 0) {
        emit actionFailed("progress", "INVALID_REQUEST", tr("Transferred bytes cannot be negative"));
        return;
    }
    if (m_progressInFlight.contains(id)) {
        m_pendingProgress[id] = qMax(m_pendingProgress.value(id, -1), transferredBytes);
        return;
    }
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty() || m_currentDeviceId.isEmpty() ||
        m_currentDeviceToken.isEmpty()) {
        lifecycleAction(id, "progress", {{"transferredBytes", transferredBytes}});
        return;
    }
    m_progressInFlight.insert(id);
    lifecycleAction(id, "progress", {{"transferredBytes", transferredBytes}});
}

void ForwardManager::complete(const QString &forwardId)
{
    lifecycleAction(forwardId, "complete");
}

void ForwardManager::finishDownload(const QString &forwardId, qint64 totalBytes)
{
    const QString id = forwardId.trimmed();
    if (id.isEmpty() || totalBytes < 0) {
        emit actionFailed("download-final-progress", "INVALID_REQUEST",
                          tr("Invalid completed download progress"));
        return;
    }
    if (m_progressInFlight.contains(id)) {
        m_pendingFinalProgress[id] = totalBytes;
        return;
    }
    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(id));
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty() || m_currentDeviceId.isEmpty() ||
        m_currentDeviceToken.isEmpty()) {
        sendRequest("download-final-progress", "POST",
                    "/api/v1/forwards/" + encodedId + "/progress",
                    {{"transferredBytes", totalBytes}}, id);
        return;
    }
    m_progressInFlight.insert(id);
    sendRequest("download-final-progress", "POST",
                "/api/v1/forwards/" + encodedId + "/progress",
                {{"transferredBytes", totalBytes}}, id);
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

void ForwardManager::dismissTask(const QString &forwardId)
{
    const QString id = forwardId.trimmed();
    const auto task = m_tasks.constFind(id);
    if (id.isEmpty() || task == m_tasks.cend()) {
        return;
    }
    const QString state = task->value("state").toString();
    if (state != "FAILED" && state != "COMPLETED" && state != "CANCELLED"
        && state != "REJECTED") {
        return;
    }
    m_dismissedTaskIds.insert(id);
    saveDismissedTasks();
    removeTask(id);
}

void ForwardManager::hideSupersededTask(const QString &forwardId)
{
    const QString id = forwardId.trimmed();
    if (id.isEmpty()) return;
    m_dismissedTaskIds.insert(id);
    m_autoAcceptingTaskIds.remove(id);
    saveDismissedTasks();
    removeTask(id);
}

void ForwardManager::markAutoAccepting(const QString &forwardId)
{
    const QString id = forwardId.trimmed();
    if (id.isEmpty() || m_autoAcceptingTaskIds.contains(id)) return;
    m_autoAcceptingTaskIds.insert(id);
    emit tasksChanged();
}

bool ForwardManager::isAutoAccepting(const QString &forwardId) const
{
    return m_autoAcceptingTaskIds.contains(forwardId.trimmed());
}

void ForwardManager::handleForwardEvent(const QString &action, const QJsonObject &payload)
{
    static const QSet<QString> supported{"task.forward.offer", "task.forward.accepted", "task.forward.started",
                                         "task.forward.completed", "task.forward.failed",
                                         "task.forward.cancelled", "task.forward.rejected"};
    if (!supported.contains(action)) {
        return;
    }
    const QString forwardId = payload.value("forwardId").toString();
    if (!forwardId.isEmpty()) {
        queryTask(forwardId);
    }
}

void ForwardManager::rememberAcceptedTransfer(const QVariantMap &task)
{
    const QString fingerprint = transferFingerprint(task);
    const int rank = channelRank(task.value("channel").toString());
    if (fingerprint.isEmpty() || rank < 0) return;
    pruneAcceptedTransfers();
    AcceptedTransfer &accepted = m_acceptedTransfers[fingerprint];
    accepted.maxChannelRank = qMax(accepted.maxChannelRank, rank);
    accepted.acceptedAtMs = QDateTime::currentMSecsSinceEpoch();
    saveAcceptedTransfers();
}

bool ForwardManager::canAutoAcceptFallback(const QVariantMap &task)
{
    pruneAcceptedTransfers();
    const QString fingerprint = transferFingerprint(task);
    const int rank = channelRank(task.value("channel").toString());
    const auto accepted = m_acceptedTransfers.constFind(fingerprint);
    return accepted != m_acceptedTransfers.cend() && rank > accepted->maxChannelRank;
}

void ForwardManager::forgetAcceptedTransfer(const QVariantMap &task)
{
    const QString fingerprint = transferFingerprint(task);
    if (!fingerprint.isEmpty() && m_acceptedTransfers.remove(fingerprint) > 0)
        saveAcceptedTransfers();
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

    const quint64 listRequestSequence = operation == "list"
        ? ++m_latestListRequestSequence : 0;
    const quint64 listBaselineRevision = m_taskRevision;
    const QString requestBaseUrl = m_baseUrl;
    const QString requestAuthToken = m_authToken;
    const QString requestDeviceId = m_currentDeviceId;
    const QString requestDeviceToken = m_currentDeviceToken;

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
            [this, reply, operation, forwardId, listRequestSequence, listBaselineRevision,
             requestBaseUrl, requestAuthToken, requestDeviceId, requestDeviceToken]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        endRequest();

        // A response created for an earlier login/device session must never
        // repopulate or mutate the current account's task state.
        if (requestBaseUrl != m_baseUrl || requestAuthToken != m_authToken
            || requestDeviceId != m_currentDeviceId
            || requestDeviceToken != m_currentDeviceToken) {
            return;
        }

        if ((operation == "progress" || operation == "download-final-progress") &&
            !forwardId.isEmpty()) {
            m_progressInFlight.remove(forwardId);
            QTimer::singleShot(0, this, [this, forwardId]() {
                continueProgressQueue(forwardId);
            });
        }

        const QJsonDocument document = QJsonDocument::fromJson(payload);
        const bool success = status == 200 || status == 201;
        if (!success) {
            // A status-less HTTP/2 disconnect can poison Qt's keep-alive
            // connection. Clear the pool so the next lifecycle/progress
            // request starts on a fresh connection instead of failing in a
            // loop with "Connection closed".
            if (status == 0) {
                m_network.clearConnectionCache();
            }
            handleError(operation, forwardId, status,
                        document.isObject() ? document.object() : QJsonObject(), networkError);
            return;
        }

        setLastError(QString());
        if (operation == "list" && document.isArray()) {
            applyTaskList(document.array(), listRequestSequence, listBaselineRevision);
            return;
        }
        if (operation == "history" && document.isArray()) {
            m_historyTasks = document.array().toVariantList();
            emit historyTasksChanged();
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
    if (m_dismissedTaskIds.contains(forwardId)) {
        return;
    }
    if (task.value("state").toString() != "OFFERED") {
        m_autoAcceptingTaskIds.remove(forwardId);
    }
    m_tasks[forwardId] = task;
    m_taskMutationRevisions[forwardId] = ++m_taskRevision;
    emit tasksChanged();
    emit taskUpdated(task.toVariantMap());
}

void ForwardManager::removeTask(const QString &forwardId)
{
    const QString id = forwardId.trimmed();
    if (id.isEmpty()) return;
    m_taskMutationRevisions[id] = ++m_taskRevision;
    if (m_tasks.remove(id) > 0) {
        emit tasksChanged();
    }
}

void ForwardManager::applyTaskList(const QJsonArray &tasks, quint64 requestSequence,
                                   quint64 baselineRevision)
{
    // Multiple manual/periodic refreshes may overlap. Only the newest list
    // response is allowed to reconcile durable sessions.
    if (requestSequence != m_latestListRequestSequence) return;

    QMap<QString, QJsonObject> refreshed;
    QSet<QString> serverTaskIds;
    for (const QJsonValue &value : tasks) {
        const QJsonObject task = value.toObject();
        const QString id = task.value("forwardId").toString();
        if (id.isEmpty()) continue;
        serverTaskIds.insert(id);
        if (!m_dismissedTaskIds.contains(id)) refreshed.insert(id, task);
    }

    // The HTTP response is a snapshot that may have been produced before a
    // newer WebSocket event or single-task response. Preserve every task that
    // changed after this list request began, and preserve its presence in the
    // reconciliation set so LAN/P2P cannot delete a freshly prepared session.
    // A newer local removal acts as a tombstone and must not be resurrected by
    // the stale snapshot.
    for (auto it = m_taskMutationRevisions.cbegin(); it != m_taskMutationRevisions.cend(); ++it) {
        if (it.value() <= baselineRevision) continue;
        const QString &id = it.key();
        const auto current = m_tasks.constFind(id);
        if (current != m_tasks.cend()) {
            serverTaskIds.insert(id);
            if (!m_dismissedTaskIds.contains(id)) refreshed[id] = current.value();
        } else {
            serverTaskIds.remove(id);
            refreshed.remove(id);
        }
    }

    const QSet<QString> retainedDismissals = m_dismissedTaskIds & serverTaskIds;
    if (retainedDismissals != m_dismissedTaskIds) {
        m_dismissedTaskIds = retainedDismissals;
        saveDismissedTasks();
    }
    m_tasks = refreshed;

    // Mutations at or before this accepted snapshot are now represented by
    // the merged state and no longer need to be retained as tombstones.
    for (auto it = m_taskMutationRevisions.begin(); it != m_taskMutationRevisions.end();) {
        if (it.value() <= baselineRevision) it = m_taskMutationRevisions.erase(it);
        else ++it;
    }

    emit tasksChanged();
    emit tasksReconciled(serverTaskIds.values());
    for (const QJsonObject &task : std::as_const(refreshed)) {
        emit taskUpdated(task.toVariantMap());
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
    if ((operation == "accept" || operation == "reject")
        && m_autoAcceptingTaskIds.remove(forwardId) > 0) {
        emit tasksChanged();
    }
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

void ForwardManager::continueProgressQueue(const QString &forwardId)
{
    if (m_progressInFlight.contains(forwardId)) return;
    const auto final = m_pendingFinalProgress.find(forwardId);
    if (final != m_pendingFinalProgress.end()) {
        const qint64 totalBytes = final.value();
        m_pendingFinalProgress.erase(final);
        m_pendingProgress.remove(forwardId);
        finishDownload(forwardId, totalBytes);
        return;
    }
    const auto progress = m_pendingProgress.find(forwardId);
    if (progress != m_pendingProgress.end()) {
        const qint64 transferredBytes = progress.value();
        m_pendingProgress.erase(progress);
        reportProgress(forwardId, transferredBytes);
    }
}

QString ForwardManager::transferFingerprint(const QVariantMap &task)
{
    const QString source = task.value("sourceDeviceId").toString().trimmed();
    const QString target = task.value("targetDeviceId").toString().trimmed();
    const QString destination = task.value("destinationPath").toString().trimmed();
    const QVariantList files = task.value("files").toList();
    if (source.isEmpty() || target.isEmpty() || destination.isEmpty() || files.isEmpty()) return {};

    const QVariantList directories = task.value("directories").toList();
    QString rootPrefix;
    if (!directories.isEmpty()) {
        const QString root = directories.constFirst().toString();
        if (!root.isEmpty() && !root.contains('/')) rootPrefix = root + '/';
    }

    QStringList manifest;
    manifest.reserve(files.size());
    qint64 manifestBytes = 0;
    for (const QVariant &entry : files) {
        const QVariantMap file = entry.toMap();
        QString path = file.value("path").toString();
        if (!rootPrefix.isEmpty() && path.startsWith(rootPrefix)) path.remove(0, rootPrefix.size());
        const qint64 size = file.value("size").toLongLong();
        const QString sha256 = file.value("sha256").toString().toLower();
        if (path.isEmpty() || size < 0 || sha256.size() != 64) return {};
        manifestBytes += size;
        manifest.append(path + '\n' + QString::number(size) + '\n' + sha256);
    }
    std::sort(manifest.begin(), manifest.end());
    const qint64 totalBytes = task.contains("totalBytes")
        ? task.value("totalBytes").toLongLong() : manifestBytes;
    if (totalBytes != manifestBytes) return {};
    const QByteArray canonical = (source + '\n' + target + '\n' + destination + '\n' +
                                  QString::number(totalBytes) + '\n' + manifest.join("\n--\n")).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(canonical, QCryptographicHash::Sha256).toHex());
}

int ForwardManager::channelRank(const QString &channel)
{
    const QString normalized = channel.trimmed().toUpper();
    if (normalized == "LAN") return 0;
    if (normalized == "P2P") return 1;
    if (normalized == "RELAY") return 2;
    return -1;
}

void ForwardManager::loadAcceptedTransfers()
{
    const QByteArray stored = QSettings().value("ForwardV11/acceptedFallbacksJson", "[]").toByteArray();
    const QJsonArray entries = QJsonDocument::fromJson(stored).array();
    for (const QJsonValue &value : entries) {
        const QJsonObject entry = value.toObject();
        const QString fingerprint = entry.value("fingerprint").toString();
        const int rank = entry.value("maxChannelRank").toInt(-1);
        const qint64 acceptedAtMs = entry.value("acceptedAtMs").toVariant().toLongLong();
        if (fingerprint.size() == 64 && rank >= 0 && rank <= 2 && acceptedAtMs > 0)
            m_acceptedTransfers.insert(fingerprint, {rank, acceptedAtMs});
    }
    pruneAcceptedTransfers();
}

void ForwardManager::saveAcceptedTransfers() const
{
    QJsonArray entries;
    for (auto it = m_acceptedTransfers.cbegin(); it != m_acceptedTransfers.cend(); ++it) {
        entries.append(QJsonObject{{"fingerprint", it.key()},
                                   {"maxChannelRank", it->maxChannelRank},
                                   {"acceptedAtMs", it->acceptedAtMs}});
    }
    QSettings settings;
    settings.setValue("ForwardV11/acceptedFallbacksJson",
                      QJsonDocument(entries).toJson(QJsonDocument::Compact));
    settings.sync();
}

void ForwardManager::pruneAcceptedTransfers()
{
    const qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - AcceptedTransferTtlMs;
    bool changed = false;
    for (auto it = m_acceptedTransfers.begin(); it != m_acceptedTransfers.end();) {
        if (it->acceptedAtMs < cutoff) {
            it = m_acceptedTransfers.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed) saveAcceptedTransfers();
}

void ForwardManager::loadDismissedTasks()
{
    const QStringList ids = QSettings().value("ForwardV11/dismissedTaskIds").toStringList();
    for (const QString &id : ids) {
        if (!id.trimmed().isEmpty()) {
            m_dismissedTaskIds.insert(id.trimmed());
        }
    }
}

void ForwardManager::saveDismissedTasks() const
{
    QStringList ids = m_dismissedTaskIds.values();
    ids.sort();
    QSettings settings;
    settings.setValue("ForwardV11/dismissedTaskIds", ids);
    settings.sync();
}

bool ForwardManager::validateFiles(const QVariantList &files, QString *error)
{
    static const QRegularExpression shaPattern("^[0-9a-f]{64}$");
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

bool ForwardManager::validateDirectories(const QVariantList &directories, QString *error)
{
    if (directories.size() > 10000) {
        if (error) *error = tr("A forward task cannot contain more than 10000 directories");
        return false;
    }
    QSet<QString> paths;
    for (const QVariant &entry : directories) {
        const QString path = entry.toString();
        if (!isValidRelativePath(path) || paths.contains(path)) {
            if (error) *error = tr("The forward directory manifest is invalid");
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
