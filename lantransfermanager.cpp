#include "lantransfermanager.h"

#include "serverconfig.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QPointer>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>
#include <QtConcurrentRun>
#include <QtEndian>
#include <utility>

namespace {
constexpr qint64 MaxControlFrame = 64 * 1024;
constexpr qint64 PumpChunk = 256 * 1024;
constexpr qint64 MaxQueuedBytes = 1024 * 1024;
constexpr qint64 ProgressReportBytes = 8 * 1024 * 1024;
constexpr qint64 ProgressReportIntervalMs = 1000;
constexpr int MaxConnectionAttempts = 5;
constexpr int RetryBaseDelayMs = 500;
constexpr int ConnectTimeoutMs = 4000;
constexpr int HandshakeTimeoutMs = 5000;
constexpr int FinalAckTimeoutMs = 30000;
constexpr int RequestTimeoutMs = 30000;
constexpr int MaxManifestEntries = 10000;
constexpr quint16 DefaultListenPort = 45897;

QTcpSocket *createLanSocket(QObject *parent)
{
    auto *socket = new QTcpSocket(parent);
    // LAN data must never inherit an HTTP/SOCKS/system proxy. On Windows a
    // DefaultProxy socket can fail before emitting a SYN (ProxyProtocolError),
    // especially while a TUN/proxy client is active.
    socket->setProxy(QNetworkProxy::NoProxy);
    return socket;
}
}

quint16 LanTransferManager::configuredListenPort()
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue("MYFOLDER_LAN_PORT", &ok);
    return ok && value >= 0 && value <= 65535 ? quint16(value) : DefaultListenPort;
}

LanTransferManager::LanTransferManager(QObject *parent) : QObject(parent)
{
    setBaseUrl(MyFolderServerConfig::baseUrl());
    const QString defaultRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
                                    .filePath("MyFolder");
    setReceiveRoot(QSettings().value("transfer/receiveRoot", defaultRoot).toString());
    connect(&m_server, &QTcpServer::newConnection, this, &LanTransferManager::acceptConnection);
    const quint16 listenPort = configuredListenPort();
    if (!m_server.listen(QHostAddress::AnyIPv4, listenPort)) {
        setLastError(tr("LAN receiver could not start on TCP port %1: %2")
                         .arg(listenPort)
                         .arg(m_server.errorString()));
    } else {
        emit listenPortChanged();
    }
    loadOutgoing();
}

LanTransferManager::~LanTransferManager()
{
    m_server.close();
    for (QTcpSocket *socket : m_receivers.keys()) {
        socket->disconnect(this);
        socket->abort();
    }
    for (const Outgoing &outgoing : std::as_const(m_outgoing)) {
        if (outgoing.socket) {
            outgoing.socket->disconnect(this);
            outgoing.socket->abort();
        }
    }
}

void LanTransferManager::setBaseUrl(const QString &value)
{
    QString normalized = value.trimmed();
    while (normalized.endsWith('/')) normalized.chop(1);
    if (m_baseUrl == normalized) return;
    m_baseUrl = normalized;
    emit baseUrlChanged();
}

void LanTransferManager::setAuthToken(const QString &value)
{
    if (m_authToken == value) return;
    m_authToken = value;
    emit authTokenChanged();
}

void LanTransferManager::setCurrentDeviceId(const QString &value)
{
    if (m_currentDeviceId == value) return;
    m_currentDeviceId = value;
    emit credentialsChanged();
}

void LanTransferManager::setCurrentDeviceToken(const QString &value)
{
    if (m_currentDeviceToken == value) return;
    m_currentDeviceToken = value;
    emit credentialsChanged();
}

void LanTransferManager::setReceiveRoot(const QString &value)
{
    const QString normalized = QDir::cleanPath(value.trimmed());
    if (normalized.isEmpty() || normalized == "." || m_receiveRoot == normalized) return;
    if (!QDir().mkpath(normalized)) {
        setLastError(tr("Cannot create the LAN receive directory"));
        return;
    }
    m_receiveRoot = normalized;
    QSettings().setValue("transfer/receiveRoot", normalized);
    emit receiveRootChanged();
}

bool LanTransferManager::isDirectory(const QString &path) const
{
    return QFileInfo(path).isDir();
}

void LanTransferManager::sendFile(const QString &targetDeviceId, const QString &targetAddress,
                                  int targetPort, const QString &destinationPath,
                                  const QString &localFilePath)
{
    sendPath(targetDeviceId, targetAddress, targetPort, destinationPath, localFilePath);
}

void LanTransferManager::sendPath(const QString &targetDeviceId, const QString &targetAddress,
                                  int targetPort, const QString &destinationPath,
                                  const QString &localPath)
{
    sendPathViaRoute(targetDeviceId, targetAddress, QString(), targetPort,
                     destinationPath, localPath);
}

void LanTransferManager::sendPathViaRoute(const QString &targetDeviceId,
                                          const QString &targetAddress,
                                          const QString &sourceAddress,
                                          int targetPort,
                                          const QString &destinationPath,
                                          const QString &localPath)
{
    if (m_authToken.isEmpty() || m_currentDeviceId.isEmpty() || m_currentDeviceToken.isEmpty()
        || targetDeviceId.trimmed().isEmpty() || targetDeviceId == m_currentDeviceId
        || targetAddress.trimmed().isEmpty() || targetPort < 1 || targetPort > 65535
        || !safeRelativePath(destinationPath) || !QFileInfo::exists(localPath)) {
        emit createFailed(tr("LAN transfer parameters are invalid or the target has no listening endpoint"));
        emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath, localPath,
                                       QFileInfo(localPath).isDir(),
                                       tr("LAN transfer parameters are invalid or the target has no listening endpoint"));
        return;
    }
    setBusy(true);
    auto *watcher = new QFutureWatcher<PathManifest>(this);
    connect(watcher, &QFutureWatcher<PathManifest>::finished, this,
            [this, watcher, targetDeviceId, targetAddress, sourceAddress, targetPort, destinationPath,
             localPath]() {
        const PathManifest manifest = watcher->result();
        watcher->deleteLater();
        if (!manifest.error.isEmpty()) {
            setBusy(false);
            emit createFailed(manifest.error);
            emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath,
                                           manifest.rootPath, manifest.directory, manifest.error);
            return;
        }
        createTask(targetDeviceId, targetAddress, sourceAddress, targetPort,
                   destinationPath, manifest);
    });
    watcher->setFuture(QtConcurrent::run(&LanTransferManager::scanPath, localPath));
}

void LanTransferManager::createTask(const QString &targetDeviceId, const QString &targetAddress,
                                    const QString &sourceAddress, int targetPort,
                                    const QString &destinationPath,
                                    const PathManifest &manifest)
{
    QJsonArray files;
    qint64 totalSize = 0;
    for (const TransferFile &file : manifest.files) {
        files.append(QJsonObject{{"path", file.path}, {"size", file.size}, {"sha256", file.sha256}});
        totalSize += file.size;
    }
    QJsonArray directories;
    for (const QString &directory : manifest.directories) directories.append(directory);
    QJsonObject body{{"sourceDeviceId", m_currentDeviceId}, {"targetDeviceId", targetDeviceId},
                     {"destinationPath", destinationPath}, {"deleteSource", false},
                     {"channel", "LAN"},
                     {"files", files}, {"directories", directories}};
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/forwards"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, targetDeviceId, targetAddress, sourceAddress, targetPort, destinationPath,
             manifest, totalSize]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray responseBytes = reply->readAll();
        const QJsonDocument document = QJsonDocument::fromJson(responseBytes);
        const QString networkError = reply->errorString();
        const int networkErrorCode = int(reply->error());
        if (status == 0) {
            // A transparent proxy can leave an already-connected HTTP socket half-open.
            // Do not let the next fallback attempt reuse that dead connection.
            m_network.clearConnectionCache();
        }
        reply->deleteLater();
        setBusy(false);
        if (status != 201 || !document.isObject()) {
            const QJsonObject error = document.object();
            QString message = error.value("message").toString(networkError);
            if (message.isEmpty()) message = tr("No response from server");
            if (status == 0) {
                message += tr(" (network error %1)").arg(networkErrorCode);
            }
            setLastError(message);
            emit createFailed(message);
            emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath,
                                            manifest.rootPath, manifest.directory, message);
            return;
        }
        const QJsonObject task = document.object();
        Outgoing outgoing;
        outgoing.forwardId = task.value("forwardId").toString();
        outgoing.targetDeviceId = targetDeviceId;
        outgoing.targetAddress = targetAddress;
        outgoing.sourceAddress = sourceAddress;
        outgoing.targetPort = targetPort;
        outgoing.destinationPath = destinationPath;
        outgoing.localRootPath = manifest.rootPath;
        outgoing.directory = manifest.directory;
        outgoing.files = manifest.files;
        outgoing.directories = manifest.directories;
        outgoing.totalSize = totalSize;
        outgoing.token = task.value("directTransferToken").toString();
        if (outgoing.forwardId.isEmpty() || outgoing.token.isEmpty()) {
            const QString reason = tr("Server returned no LAN transfer capability");
            setLastError(reason);
            emit createFailed(reason);
            emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath,
                                           manifest.rootPath, manifest.directory, reason);
            return;
        }
        m_outgoing.insert(outgoing.forwardId, outgoing);
        saveOutgoing();
        setLastError(QString());
        emit forwardCreated(outgoing.forwardId);
    });
}

void LanTransferManager::prepareIncoming(const QVariantMap &task)
{
    if (task.value("channel").toString() != "LAN" ||
        task.value("targetDeviceId").toString() != m_currentDeviceId) return;
    if (m_incoming.contains(task.value("forwardId").toString())) return;
    const QVariantList files = task.value("files").toList();
    Incoming incoming;
    incoming.forwardId = task.value("forwardId").toString();
    incoming.token = task.value("directTransferToken").toString();
    incoming.destinationPath = task.value("destinationPath").toString();
    if (incoming.forwardId.isEmpty() || incoming.token.isEmpty()
        || !safeRelativePath(incoming.destinationPath)) {
        emit incomingFailed(incoming.forwardId, "LAN_MANIFEST_INVALID", tr("Invalid LAN manifest"));
        return;
    }
    for (const QVariant &value : files) {
        const QVariantMap item = value.toMap();
        TransferFile file;
        file.path = item.value("path").toString();
        file.sha256 = item.value("sha256").toString().toLower();
        file.size = item.value("size").toLongLong();
        if (!safeRelativePath(file.path) || file.size < 0 || file.sha256.size() != 64
            || incoming.files.contains(file.path)) {
            emit incomingFailed(incoming.forwardId, "LAN_MANIFEST_INVALID", tr("Invalid LAN file manifest"));
            return;
        }
        incoming.files.insert(file.path, file);
        incoming.totalSize += file.size;
    }
    for (const QVariant &value : task.value("directories").toList()) {
        const QString path = value.toString();
        if (!safeRelativePath(path)) {
            emit incomingFailed(incoming.forwardId, "LAN_MANIFEST_INVALID", tr("Invalid LAN directory manifest"));
            return;
        }
        incoming.directories.append(path);
    }
    const QString destinationRoot = QDir(m_receiveRoot).absoluteFilePath(incoming.destinationPath);
    for (const QString &directory : incoming.directories)
        if (!QDir().mkpath(QDir(destinationRoot).absoluteFilePath(directory))) {
            emit incomingFailed(incoming.forwardId, "LAN_CREATE_DIRECTORY_FAILED",
                                tr("Cannot create a received directory"));
            return;
        }
    loadIncomingState(incoming);
    m_incoming.insert(incoming.forwardId, incoming);
}

void LanTransferManager::handleTaskUpdated(const QVariantMap &task)
{
    const QString id = task.value("forwardId").toString();
    const QString state = task.value("state").toString();
    if (task.value("channel").toString() != "LAN") return;
    if (task.value("sourceDeviceId").toString() == m_currentDeviceId
        && (state == "ACCEPTED" || state == "TRANSFERRING") && m_outgoing.contains(id)) {
        startOutgoing(id);
    }
    if (task.value("targetDeviceId").toString() == m_currentDeviceId
        && state == "TRANSFERRING" && m_incoming.contains(id)) {
        const Incoming incoming = m_incoming.value(id);
        if (incoming.files.isEmpty() || incoming.completed.size() == incoming.files.size()) {
            m_incoming.remove(id);
            emit incomingCompleted(id, incoming.totalSize);
        }
    }
    if (task.value("sourceDeviceId").toString() == m_currentDeviceId
        && state == "FAILED" && m_outgoing.contains(id)) {
        failOutgoing(id, task.value("failureReason").toString());
        return;
    }
    if (state == "COMPLETED" || state == "CANCELLED" || state == "REJECTED") cancel(id);
}

void LanTransferManager::reconcileTasks(const QStringList &activeTaskIds)
{
    const QSet<QString> active(activeTaskIds.cbegin(), activeTaskIds.cend());
    QSet<QString> localIds(m_outgoing.keyBegin(), m_outgoing.keyEnd());
    localIds.unite(QSet<QString>(m_incoming.keyBegin(), m_incoming.keyEnd()));
    for (const QString &id : localIds) {
        if (active.contains(id)) continue;
        cancel(id);
    }
}

void LanTransferManager::startOutgoing(const QString &forwardId)
{
    auto it = m_outgoing.find(forwardId);
    if (it == m_outgoing.end() || it->started) return;
    it->started = true;
    it->fileIndex = 0;
    saveOutgoing();
    startNextFile(forwardId);
}

void LanTransferManager::startNextFile(const QString &forwardId)
{
    auto it = m_outgoing.find(forwardId);
    if (it == m_outgoing.end() || it->fileIndex >= it->files.size() || it->retryScheduled) return;
    const TransferFile currentFile = it->files.at(it->fileIndex);
    ++it->connectionAttempts;
    it->responseLength = -1;
    it->responseReady = false;
    it->allQueued = false;
    it->fileAcknowledged = false;
    it->responseBuffer.clear();
    it->sent = 0;
    QTcpSocket *socket = createLanSocket(this);
    if (!it->sourceAddress.isEmpty()
        && !socket->bind(QHostAddress(it->sourceAddress), 0)) {
        // A selected physical interface can disappear or become temporarily
        // unavailable after sleep/network roaming. Binding is only a routing
        // preference; it must not consume a LAN connection attempt.
        socket->deleteLater();
        socket = createLanSocket(this);
        it->sourceAddress.clear();
        saveOutgoing();
    }
    it->socket = socket;
    it->file = new QFile(currentFile.localPath, it->socket);
    if (!it->file->open(QIODevice::ReadOnly)) {
        failOutgoing(forwardId, tr("Source file can no longer be read"));
        return;
    }
    connect(socket, &QTcpSocket::disconnected, this, [this, forwardId, socket]() {
        auto current = m_outgoing.find(forwardId);
        if (current != m_outgoing.end() && current->socket == socket) {
            const bool acknowledged = current->fileAcknowledged;
            current->socket = nullptr;
            current->file = nullptr;
            if (!acknowledged && !current->retryScheduled) {
                socket->deleteLater();
                retryOutgoing(forwardId, tr("LAN receiver disconnected before confirming the file"));
                return;
            }
        }
        socket->deleteLater();
    });
    connect(socket, &QTcpSocket::connected, this, [this, forwardId]() {
        auto current = m_outgoing.find(forwardId);
        if (current == m_outgoing.end()) return;
        const TransferFile file = current->files.at(current->fileIndex);
        QJsonObject header{{"protocol", "MYFOLDER_LAN_V2"}, {"forwardId", forwardId},
                           {"token", current->token}, {"filePath", file.path},
                           {"size", file.size}, {"sha256", file.sha256}};
        current->socket->write(frame(header));
        QTcpSocket *activeSocket = current->socket;
        QTimer::singleShot(HandshakeTimeoutMs, this, [this, forwardId, activeSocket]() {
            auto pending = m_outgoing.find(forwardId);
            if (pending != m_outgoing.end() && pending->socket == activeSocket
                && !pending->responseReady && !pending->retryScheduled) {
                retryOutgoing(forwardId, tr("LAN receiver did not answer the transfer handshake"));
            }
        });
    });
    connect(socket, &QTcpSocket::readyRead, this, [this, forwardId]() {
        auto current = m_outgoing.find(forwardId);
        if (current == m_outgoing.end()) return;
        current->responseBuffer += current->socket->readAll();
        QJsonObject response;
        while (takeFrame(current->responseBuffer, current->responseLength, &response)) {
            if (!response.value("ok").toBool()) {
                const QString reason = response.value("message").toString(
                    tr("LAN receiver rejected the transfer"));
                const bool retryable = !response.contains("retryable")
                    || response.value("retryable").toBool();
                if (retryable) retryOutgoing(forwardId, reason);
                else failOutgoing(forwardId, reason);
                return;
            }
            if (response.value("progress").toBool()) {
                const qint64 receivedBytes = qBound<qint64>(
                    0, qint64(response.value("receivedBytes").toDouble()), current->totalSize);
                publishOutgoingProgress(forwardId, receivedBytes);
                continue;
            }
            if (response.contains("receivedBytes")) {
                const qint64 receivedBytes = qBound<qint64>(
                    0, qint64(response.value("receivedBytes").toDouble()), current->totalSize);
                publishOutgoingProgress(forwardId, receivedBytes);
            }
            if (!current->responseReady) {
                const qint64 offset = qint64(response.value("offset").toDouble());
                const qint64 fileSize = current->files.at(current->fileIndex).size;
                if (offset < 0 || offset > fileSize || !current->file->seek(offset)) {
                    failOutgoing(forwardId, tr("LAN resume offset is invalid"));
                    return;
                }
                current->sent = offset;
                current->responseReady = true;
                pumpOutgoing(forwardId);
                if (response.value("complete").toBool()) {
                    current->fileAcknowledged = true;
                    current->connectionAttempts = 0;
                    ++current->fileIndex;
                    saveOutgoing();
                    current->socket->disconnectFromHost();
                    QTimer::singleShot(0, this, [this, forwardId]() { startNextFile(forwardId); });
                    return;
                }
            } else if (response.value("complete").toBool()) {
                current->fileAcknowledged = true;
                current->connectionAttempts = 0;
                ++current->fileIndex;
                saveOutgoing();
                current->socket->disconnectFromHost();
                QTimer::singleShot(0, this, [this, forwardId]() { startNextFile(forwardId); });
                return;
            }
        }
    });
    connect(socket, &QTcpSocket::bytesWritten, this,
            [this, forwardId](qint64) { pumpOutgoing(forwardId); });
    connect(socket, &QTcpSocket::errorOccurred, this,
            [this, forwardId, socket](QAbstractSocket::SocketError) {
        auto current = m_outgoing.find(forwardId);
        if (current != m_outgoing.end() && current->socket == socket
            && !current->allQueued && !current->retryScheduled)
            retryOutgoing(forwardId, socket->errorString());
    });
    socket->connectToHost(it->targetAddress, quint16(it->targetPort));
    QTimer::singleShot(ConnectTimeoutMs, this, [this, forwardId, socket]() {
        auto current = m_outgoing.find(forwardId);
        if (current != m_outgoing.end() && current->socket == socket
            && socket->state() != QAbstractSocket::ConnectedState)
            retryOutgoing(forwardId, tr("LAN connection timed out"));
    });
}

void LanTransferManager::pumpOutgoing(const QString &forwardId)
{
    auto it = m_outgoing.find(forwardId);
    if (it == m_outgoing.end() || !it->responseReady || it->allQueued || !it->socket || !it->file) return;
    while (it->socket->bytesToWrite() < MaxQueuedBytes && !it->file->atEnd()) {
        const QByteArray data = it->file->read(PumpChunk);
        if (data.isEmpty() && !it->file->atEnd()) {
            failOutgoing(forwardId, tr("Reading the source file failed"));
            return;
        }
        if (it->socket->write(data) != data.size()) {
            failOutgoing(forwardId, tr("Writing to the LAN connection failed"));
            return;
        }
        it->sent += data.size();
    }
    if (it->file->atEnd() && it->socket->bytesToWrite() == 0) {
        it->allQueued = true;
        it->file->close();
        QTcpSocket *activeSocket = it->socket;
        QTimer::singleShot(FinalAckTimeoutMs, this, [this, forwardId, activeSocket]() {
            auto pending = m_outgoing.find(forwardId);
            if (pending != m_outgoing.end() && pending->socket == activeSocket
                && pending->allQueued && !pending->fileAcknowledged
                && !pending->retryScheduled) {
                retryOutgoing(forwardId, tr("LAN receiver did not confirm file verification"));
            }
        });
    }
}

void LanTransferManager::retryOutgoing(const QString &forwardId, const QString &reason)
{
    auto it = m_outgoing.find(forwardId);
    if (it == m_outgoing.end() || it->retryScheduled) return;
    if (it->connectionAttempts >= MaxConnectionAttempts) {
        failOutgoing(forwardId, tr("LAN connection to %1:%2 failed after %3 attempts: %4")
                                    .arg(it->targetAddress).arg(it->targetPort)
                                    .arg(MaxConnectionAttempts).arg(reason));
        return;
    }

    it->retryScheduled = true;
    QTcpSocket *socket = it->socket;
    if (socket) {
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
    }
    it->socket = nullptr;
    it->file = nullptr;
    it->responseReady = false;
    it->allQueued = false;
    it->fileAcknowledged = false;
    it->responseBuffer.clear();
    it->responseLength = -1;
    const int delay = RetryBaseDelayMs * it->connectionAttempts;
    setLastError(tr("LAN connection is not ready; retrying (%1/%2): %3")
                     .arg(it->connectionAttempts).arg(MaxConnectionAttempts).arg(reason));
    QTimer::singleShot(delay, this, [this, forwardId]() {
        auto current = m_outgoing.find(forwardId);
        if (current == m_outgoing.end()) return;
        current->retryScheduled = false;
        startNextFile(forwardId);
    });
}

void LanTransferManager::failOutgoing(const QString &forwardId, const QString &reason)
{
    auto it = m_outgoing.find(forwardId);
    if (it == m_outgoing.end()) return;
    const Outgoing failed = it.value();
    if (failed.socket) failed.socket->abort();
    m_outgoing.erase(it);
    if (m_outgoingProgress.remove(forwardId) > 0) emit outgoingProgressChanged();
    saveOutgoing();
    setLastError(reason);
    emit outgoingFallbackRequested(forwardId, failed.targetDeviceId, failed.destinationPath,
                                   failed.localRootPath, failed.directory, reason);
}

void LanTransferManager::cancel(const QString &forwardId)
{
    auto it = m_outgoing.find(forwardId);
    if (it != m_outgoing.end()) {
        if (it->socket) it->socket->abort();
        m_outgoing.erase(it);
        saveOutgoing();
    }
    if (m_outgoingProgress.remove(forwardId) > 0) emit outgoingProgressChanged();
    m_incoming.remove(forwardId);
    QFile::remove(incomingStatePath(forwardId));
}

void LanTransferManager::acceptConnection()
{
    while (QTcpSocket *socket = m_server.nextPendingConnection()) {
        m_receivers.insert(socket, ReceiverConnection{});
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { consumeReceiver(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            auto it = m_receivers.find(socket);
            if (it != m_receivers.end()) {
                if (it->file) { it->file->close(); delete it->file; }
                m_receivers.erase(it);
            }
            socket->deleteLater();
        });
    }
}

void LanTransferManager::consumeReceiver(QTcpSocket *socket)
{
    auto it = m_receivers.find(socket);
    if (it == m_receivers.end()) return;
    it->buffer += socket->readAll();
    if (!it->headerReady) {
        QJsonObject header;
        if (!takeFrame(it->buffer, it->controlLength, &header)) return;
        const QString id = header.value("forwardId").toString();
        auto expected = m_incoming.find(id);
        const QString filePath = header.value("filePath").toString();
        const auto expectedFile = expected == m_incoming.end()
                                      ? QHash<QString, TransferFile>::const_iterator{}
                                      : expected->files.constFind(filePath);
        if (header.value("protocol").toString() != "MYFOLDER_LAN_V2") {
            socket->write(frame(QJsonObject{{"ok", false}, {"code", "LAN_PROTOCOL_MISMATCH"},
                                             {"retryable", false},
                                             {"message", "LAN protocol version is not supported"}}));
            socket->disconnectFromHost();
            return;
        }
        if (expected == m_incoming.end()) {
            socket->write(frame(QJsonObject{{"ok", false}, {"code", "LAN_TASK_NOT_READY"},
                                             {"retryable", true},
                                             {"message", "LAN receiver is still preparing the accepted task"}}));
            socket->disconnectFromHost();
            return;
        }
        const bool authorized = expectedFile != expected->files.constEnd()
            && header.value("token").toString() == expected->token
            && qint64(header.value("size").toDouble()) == expectedFile->size
            && header.value("sha256").toString().compare(expectedFile->sha256, Qt::CaseInsensitive) == 0;
        if (!authorized) {
            socket->write(frame(QJsonObject{{"ok", false}, {"code", "LAN_UNAUTHORIZED"},
                                             {"retryable", false},
                                             {"message", "LAN capability or file manifest was rejected"}}));
            socket->disconnectFromHost();
            return;
        }
        if (expected->completed.contains(filePath)) {
            socket->write(frame(QJsonObject{{"ok", true}, {"offset", double(expectedFile->size)},
                                             {"receivedBytes", double(expected->verifiedBytes)},
                                             {"complete", true}, {"filePath", filePath}}));
            socket->disconnectFromHost();
            return;
        }
        const QString relative = expected->destinationPath + "/" + filePath;
        const QString root = QDir::fromNativeSeparators(QDir(m_receiveRoot).absolutePath());
        QString finalPath = QDir(root).absoluteFilePath(relative);
        finalPath = QDir::fromNativeSeparators(QFileInfo(finalPath).absoluteFilePath());
        if (!finalPath.startsWith(root + "/", Qt::CaseInsensitive)) {
            socket->write(frame(QJsonObject{{"ok", false}, {"message", "Destination escapes receive root"}}));
            socket->disconnectFromHost();
            return;
        }
        QDir().mkpath(QFileInfo(finalPath).absolutePath());
        if (QFile::exists(finalPath)) finalPath = availableTargetPath(finalPath);
        it->forwardId = id;
        it->filePath = filePath;
        it->expectedHash = expectedFile->sha256;
        it->finalPath = finalPath;
        it->partPath = finalPath + ".myfolder-part";
        it->metaPath = it->partPath + ".json";
        it->total = expectedFile->size;
        bool resumeMatches = false;
        QFile metadata(it->metaPath);
        if (metadata.open(QIODevice::ReadOnly)) {
            const QJsonObject saved = QJsonDocument::fromJson(metadata.readAll()).object();
            resumeMatches = saved.value("sha256").toString() == expectedFile->sha256
                            && qint64(saved.value("size").toDouble()) == expectedFile->size;
            metadata.close();
        }
        if (!resumeMatches) {
            QFile::remove(it->partPath);
            QFile::remove(it->metaPath);
        }
        QSaveFile savedMetadata(it->metaPath);
        if (!savedMetadata.open(QIODevice::WriteOnly)
            || savedMetadata.write(QJsonDocument(QJsonObject{{"sha256", expectedFile->sha256},
                                                              {"size", double(expectedFile->size)}})
                                       .toJson(QJsonDocument::Compact)) < 0
            || !savedMetadata.commit()) {
            socket->write(frame(QJsonObject{{"ok", false}, {"message", "Cannot persist resume metadata"}}));
            socket->disconnectFromHost();
            return;
        }
        it->file = new QFile(it->partPath);
        if (!it->file->open(QIODevice::ReadWrite)) {
            socket->write(frame(QJsonObject{{"ok", false}, {"message", "Cannot open destination file"}}));
            socket->disconnectFromHost();
            return;
        }
        if (it->file->size() > it->total) it->file->resize(0);
        it->received = it->file->size();
        it->file->seek(it->received);
        it->headerReady = true;
        socket->write(frame(QJsonObject{{"ok", true}, {"offset", double(it->received)},
                                         {"receivedBytes", double(expected->verifiedBytes + it->received)}}));
    }
    if (!it->headerReady) return;
    const qint64 remaining = it->total - it->received;
    const qint64 take = qMin<qint64>(remaining, it->buffer.size());
    if (take > 0) {
        const QByteArray data = it->buffer.left(take);
        it->buffer.remove(0, int(take));
        if (it->file->write(data) != data.size()) {
            emit incomingFailed(it->forwardId, "LAN_WRITE_FAILED", tr("Writing the received file failed"));
            socket->abort();
            return;
        }
        it->received += take;
        auto incoming = m_incoming.find(it->forwardId);
        if (incoming != m_incoming.end()) {
            const qint64 progressBytes = incoming->verifiedBytes + it->received;
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (progressBytes - incoming->lastProgressBytes >= ProgressReportBytes ||
                now - incoming->lastProgressAtMs >= ProgressReportIntervalMs) {
                incoming->lastProgressBytes = progressBytes;
                incoming->lastProgressAtMs = now;
                emit incomingProgress(it->forwardId, progressBytes);
                socket->write(frame(QJsonObject{{"ok", true}, {"progress", true},
                                                 {"receivedBytes", double(progressBytes)}}));
            }
        }
    }
    if (it->received == it->total) finalizeReceiver(socket);
}

void LanTransferManager::finalizeReceiver(QTcpSocket *socket)
{
    auto it = m_receivers.find(socket);
    if (it == m_receivers.end() || !it->file) return;
    it->file->flush();
    it->file->close();
    delete it->file;
    it->file = nullptr;
    const QString id = it->forwardId;
    const QString filePath = it->filePath;
    const QString partPath = it->partPath;
    const QString metaPath = it->metaPath;
    const QString finalPath = it->finalPath;
    const qint64 total = it->total;
    const QString expectedHash = it->expectedHash;
    const QPointer<QTcpSocket> safeSocket(socket);
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, safeSocket, id, filePath, partPath, metaPath, finalPath, total, expectedHash]() {
        const QString actual = watcher->result();
        watcher->deleteLater();
        if (actual.compare(expectedHash, Qt::CaseInsensitive) != 0) {
            QFile::remove(partPath);
            QFile::remove(metaPath);
            if (safeSocket) safeSocket->write(frame(QJsonObject{{"ok", false},
                                                                  {"message", "LAN SHA-256 mismatch"}}));
            emit incomingFailed(id, "LAN_SHA256_MISMATCH", tr("Received file SHA-256 mismatch"));
        } else {
            if (!QFile::rename(partPath, finalPath)) {
                if (safeSocket) safeSocket->write(frame(QJsonObject{{"ok", false},
                                                                      {"message", "Cannot finalize received file"}}));
                emit incomingFailed(id, "LAN_FINALIZE_FAILED", tr("Cannot finalize the received file"));
            } else {
                QFile::remove(metaPath);
                auto incoming = m_incoming.find(id);
                if (incoming != m_incoming.end()) {
                    incoming->completed.insert(filePath);
                    incoming->verifiedBytes += total;
                    incoming->lastProgressBytes = incoming->verifiedBytes;
                    incoming->lastProgressAtMs = QDateTime::currentMSecsSinceEpoch();
                    incoming->finalPaths.insert(filePath, finalPath);
                    saveIncomingState(*incoming);
                    emit incomingProgress(id, incoming->verifiedBytes);
                    if (safeSocket) safeSocket->write(frame(QJsonObject{{"ok", true},
                                                                          {"complete", true},
                                                                          {"filePath", filePath},
                                                                          {"receivedBytes", double(incoming->verifiedBytes)}}));
                    if (incoming->completed.size() == incoming->files.size()) {
                        const qint64 taskTotal = incoming->totalSize;
                        m_incoming.erase(incoming);
                        emit incomingCompleted(id, taskTotal);
                    }
                }
            }
        }
        if (safeSocket) safeSocket->disconnectFromHost();
    });
    watcher->setFuture(QtConcurrent::run(&LanTransferManager::sha256File, partPath));
}

void LanTransferManager::publishOutgoingProgress(const QString &forwardId, qint64 receivedBytes)
{
    if (m_outgoingProgress.contains(forwardId)
        && m_outgoingProgress.value(forwardId).toLongLong() == receivedBytes) return;
    m_outgoingProgress.insert(forwardId, receivedBytes);
    emit outgoingProgressReady(forwardId, receivedBytes);
    emit outgoingProgressChanged();
}

void LanTransferManager::setLastError(const QString &value)
{
    if (m_lastError == value) return;
    m_lastError = value;
    emit lastErrorChanged();
}

void LanTransferManager::setBusy(bool value)
{
    if (m_busy == value) return;
    m_busy = value;
    emit busyChanged();
}

void LanTransferManager::applyRequestDefaults(QNetworkRequest &request) const
{
    request.setTransferTimeout(RequestTimeoutMs);
    request.setRawHeader("Authorization", m_authToken.toUtf8());
    request.setRawHeader("X-Device-Id", m_currentDeviceId.toUtf8());
    request.setRawHeader("X-Device-Token", m_currentDeviceToken.toUtf8());
}

QString LanTransferManager::persistencePath() const
{
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!directory.exists()) directory.mkpath(".");
    return directory.filePath("lan_outgoing_v2.json");
}

void LanTransferManager::loadOutgoing()
{
    QFile file(persistencePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonArray tasks = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue &value : tasks) {
        const QJsonObject object = value.toObject();
        Outgoing outgoing;
        outgoing.forwardId = object.value("forwardId").toString();
        outgoing.targetDeviceId = object.value("targetDeviceId").toString();
        outgoing.targetAddress = object.value("targetAddress").toString();
        outgoing.sourceAddress = object.value("sourceAddress").toString();
        outgoing.targetPort = object.value("targetPort").toInt();
        outgoing.destinationPath = object.value("destinationPath").toString();
        outgoing.localRootPath = object.value("localRootPath").toString();
        outgoing.token = object.value("token").toString();
        outgoing.directory = object.value("directory").toBool();
        outgoing.totalSize = qint64(object.value("totalSize").toDouble());
        for (const QJsonValue &fileValue : object.value("files").toArray()) {
            const QJsonObject fileObject = fileValue.toObject();
            TransferFile transfer;
            transfer.path = fileObject.value("path").toString();
            transfer.localPath = fileObject.value("localPath").toString();
            transfer.sha256 = fileObject.value("sha256").toString();
            transfer.size = qint64(fileObject.value("size").toDouble());
            if (safeRelativePath(transfer.path) && QFileInfo(transfer.localPath).isFile())
                outgoing.files.append(transfer);
        }
        for (const QJsonValue &directory : object.value("directories").toArray())
            if (safeRelativePath(directory.toString())) outgoing.directories.append(directory.toString());
        if (!outgoing.forwardId.isEmpty() && !outgoing.token.isEmpty()
            && !outgoing.files.isEmpty() && outgoing.targetPort > 0)
            m_outgoing.insert(outgoing.forwardId, outgoing);
    }
}

void LanTransferManager::saveOutgoing() const
{
    QJsonArray tasks;
    for (const Outgoing &outgoing : m_outgoing) {
        QJsonArray files;
        for (const TransferFile &file : outgoing.files)
            files.append(QJsonObject{{"path", file.path}, {"localPath", file.localPath},
                                     {"sha256", file.sha256}, {"size", double(file.size)}});
        QJsonArray directories;
        for (const QString &directory : outgoing.directories) directories.append(directory);
        tasks.append(QJsonObject{{"forwardId", outgoing.forwardId},
                                 {"targetDeviceId", outgoing.targetDeviceId},
                                 {"targetAddress", outgoing.targetAddress},
                                 {"sourceAddress", outgoing.sourceAddress},
                                 {"targetPort", outgoing.targetPort},
                                 {"destinationPath", outgoing.destinationPath},
                                 {"localRootPath", outgoing.localRootPath},
                                 {"token", outgoing.token},
                                 {"directory", outgoing.directory},
                                 {"totalSize", double(outgoing.totalSize)},
                                 {"files", files}, {"directories", directories}});
    }
    QSaveFile file(persistencePath());
    if (!file.open(QIODevice::WriteOnly)) return;
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    if (file.write(QJsonDocument(tasks).toJson(QJsonDocument::Compact)) < 0 || !file.commit()) return;
    QFile::setPermissions(persistencePath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QString LanTransferManager::incomingStatePath(const QString &forwardId) const
{
    QDir directory(QDir(m_receiveRoot).filePath(".myfolder-transfer"));
    if (!directory.exists()) directory.mkpath(".");
    const QString name = QString::fromLatin1(
        QCryptographicHash::hash(forwardId.toUtf8(), QCryptographicHash::Sha256).toHex());
    return directory.filePath(name + ".json");
}

void LanTransferManager::loadIncomingState(Incoming &incoming) const
{
    QFile state(incomingStatePath(incoming.forwardId));
    if (!state.open(QIODevice::ReadOnly)) return;
    const QJsonObject rootObject = QJsonDocument::fromJson(state.readAll()).object();
    if (rootObject.value("token").toString() != incoming.token) return;
    const QString receiveRoot = QDir::fromNativeSeparators(QDir(m_receiveRoot).absolutePath());
    for (const QJsonValue &value : rootObject.value("completed").toArray()) {
        const QJsonObject item = value.toObject();
        const QString path = item.value("path").toString();
        const QString finalPath = QDir::fromNativeSeparators(
            QFileInfo(item.value("finalPath").toString()).absoluteFilePath());
        const auto manifest = incoming.files.constFind(path);
        const QFileInfo info(finalPath);
        if (manifest == incoming.files.constEnd() || !finalPath.startsWith(receiveRoot + "/", Qt::CaseInsensitive)
            || !info.isFile() || info.size() != manifest->size
            || qint64(item.value("modifiedMs").toDouble()) != info.lastModified().toMSecsSinceEpoch()
            || item.value("sha256").toString().compare(manifest->sha256, Qt::CaseInsensitive) != 0)
            continue;
        incoming.completed.insert(path);
        incoming.finalPaths.insert(path, finalPath);
        incoming.verifiedBytes += manifest->size;
    }
}

void LanTransferManager::saveIncomingState(const Incoming &incoming) const
{
    QJsonArray completed;
    for (const QString &path : incoming.completed) {
        const QString finalPath = incoming.finalPaths.value(path);
        const QFileInfo info(finalPath);
        const auto manifest = incoming.files.constFind(path);
        if (manifest == incoming.files.constEnd() || !info.isFile()) continue;
        completed.append(QJsonObject{{"path", path}, {"finalPath", finalPath},
                                     {"size", double(manifest->size)}, {"sha256", manifest->sha256},
                                     {"modifiedMs", double(info.lastModified().toMSecsSinceEpoch())}});
    }
    QSaveFile state(incomingStatePath(incoming.forwardId));
    if (!state.open(QIODevice::WriteOnly)) return;
    state.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    state.write(QJsonDocument(QJsonObject{{"token", incoming.token}, {"completed", completed}})
                    .toJson(QJsonDocument::Compact));
    state.commit();
}

QString LanTransferManager::sha256File(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) return {};
    return QString::fromLatin1(hash.result().toHex());
}

LanTransferManager::PathManifest LanTransferManager::scanPath(const QString &path)
{
    PathManifest manifest;
    manifest.rootPath = QDir::cleanPath(path);
    const QFileInfo root(manifest.rootPath);
    if (!root.exists() || root.isSymLink()) {
        manifest.error = tr("The selected file or folder no longer exists");
        return manifest;
    }
    if (root.isFile()) {
        TransferFile file;
        file.path = root.fileName();
        file.localPath = root.absoluteFilePath();
        file.size = root.size();
        file.sha256 = sha256File(file.localPath);
        if (!root.isReadable() || !safeRelativePath(file.path) || file.sha256.isEmpty())
            manifest.error = tr("Unable to read or hash the selected file");
        else
            manifest.files.append(file);
        return manifest;
    }
    if (!root.isDir() || !root.isReadable() || !safeRelativePath(root.fileName())) {
        manifest.error = tr("Unable to read the selected folder");
        return manifest;
    }
    manifest.directory = true;
    manifest.directories.append(root.fileName());
    QDirIterator iterator(root.absoluteFilePath(),
                          QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        QString relative = QDir(root.absoluteFilePath()).relativeFilePath(info.absoluteFilePath());
        relative = QDir::fromNativeSeparators(relative);
        const QString manifestPath = root.fileName() + "/" + relative;
        if (!safeRelativePath(manifestPath)) {
            manifest.error = tr("The selected folder contains an unsafe path");
            return manifest;
        }
        if (info.isDir()) {
            manifest.directories.append(manifestPath);
            if (manifest.directories.size() > MaxManifestEntries) {
                manifest.error = tr("The selected folder contains too many directories");
                return manifest;
            }
            continue;
        }
        if (!info.isFile() || !info.isReadable()) {
            manifest.error = tr("The selected folder contains an unreadable file");
            return manifest;
        }
        TransferFile file;
        file.path = manifestPath;
        file.localPath = info.absoluteFilePath();
        file.size = info.size();
        file.sha256 = sha256File(file.localPath);
        if (file.sha256.isEmpty()) {
            manifest.error = tr("Unable to calculate a file SHA-256");
            return manifest;
        }
        manifest.files.append(file);
        if (manifest.files.size() > MaxManifestEntries) {
            manifest.error = tr("The selected folder contains too many files");
            return manifest;
        }
    }
    return manifest;
}

QString LanTransferManager::availableTargetPath(const QString &path)
{
    if (!QFile::exists(path)) return path;
    const QFileInfo info(path);
    const QString directory = info.absolutePath();
    const QString suffix = info.completeSuffix();
    QString stem = info.fileName();
    if (!suffix.isEmpty()) stem.chop(suffix.size() + 1);
    for (int index = 1; index < 10000; ++index) {
        const QString name = suffix.isEmpty()
            ? QString("%1 (%2)").arg(stem).arg(index)
            : QString("%1 (%2).%3").arg(stem).arg(index).arg(suffix);
        const QString candidate = QDir(directory).filePath(name);
        if (!QFile::exists(candidate) && !QFile::exists(candidate + ".myfolder-part")) return candidate;
    }
    return QDir(directory).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces)
                                    + (suffix.isEmpty() ? QString() : "." + suffix));
}

bool LanTransferManager::safeRelativePath(const QString &path)
{
    if (path.isEmpty() || path.startsWith('/') || path.endsWith('/') || path.contains('\\')
        || path.contains("//") || path.contains(QChar::Null)) return false;
    if (path.size() >= 2 && path.at(0).isLetter() && path.at(1) == ':') return false;
    for (const QString &part : path.split('/'))
        if (part.isEmpty() || part == "." || part == "..") return false;
    return true;
}

QByteArray LanTransferManager::frame(const QJsonObject &object)
{
    const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
    QByteArray result(4, Qt::Uninitialized);
    qToBigEndian<quint32>(quint32(payload.size()), reinterpret_cast<uchar *>(result.data()));
    result += payload;
    return result;
}

bool LanTransferManager::takeFrame(QByteArray &buffer, qint64 &expectedLength, QJsonObject *object)
{
    if (expectedLength < 0) {
        if (buffer.size() < 4) return false;
        expectedLength = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(buffer.constData()));
        buffer.remove(0, 4);
        if (expectedLength <= 0 || expectedLength > MaxControlFrame) {
            expectedLength = -2;
            return false;
        }
    }
    if (expectedLength < 0 || buffer.size() < expectedLength) return false;
    const QJsonDocument document = QJsonDocument::fromJson(buffer.left(expectedLength));
    buffer.remove(0, int(expectedLength));
    expectedLength = -1;
    if (!document.isObject()) return false;
    *object = document.object();
    return true;
}
