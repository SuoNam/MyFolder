#include "relaydownloadmanager.h"
#include "serverconfig.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QTimer>
#include <QUrlQuery>

namespace {
constexpr int RequestTimeoutMs = 30000;
constexpr int MaxNetworkRetries = 5;
constexpr int MaxLocalRecoveries = 1;

struct RangeContext
{
    QSharedPointer<QFile> file;
    QByteArray errorBody;
    QString protocolError;
    qint64 offset = 0;
    qint64 expectedSize = 0;
    bool headersChecked = false;
    bool accepted = false;
};
}

RelayDownloadManager::RelayDownloadManager(QObject *parent)
    : QObject(parent)
{
    setBaseUrl(MyFolderServerConfig::baseUrl());
    QString defaultRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (defaultRoot.isEmpty()) {
        defaultRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    setReceiveRoot(QDir(defaultRoot).filePath("MyFolder"));
    QDir().mkpath(m_receiveRoot);
    loadTasks();
}

void RelayDownloadManager::setBaseUrl(const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith('/')) normalized.chop(1);
    if (m_baseUrl == normalized) return;
    m_baseUrl = normalized;
    emit baseUrlChanged();
}

void RelayDownloadManager::setAuthToken(const QString &authToken)
{
    if (m_authToken == authToken) return;
    m_authToken = authToken;
    emit authTokenChanged();
}

void RelayDownloadManager::setCurrentDeviceId(const QString &deviceId)
{
    if (m_currentDeviceId == deviceId) return;
    m_currentDeviceId = deviceId;
    emit currentDeviceIdChanged();
}

void RelayDownloadManager::setCurrentDeviceToken(const QString &deviceToken)
{
    if (m_currentDeviceToken == deviceToken) return;
    m_currentDeviceToken = deviceToken;
    emit currentDeviceTokenChanged();
}

void RelayDownloadManager::setReceiveRoot(const QString &receiveRoot)
{
    const QString normalized = QDir::cleanPath(receiveRoot.trimmed());
    if (m_receiveRoot == normalized) return;
    m_receiveRoot = normalized;
    emit receiveRootChanged();
}

QVariantList RelayDownloadManager::downloads() const
{
    QVariantList result;
    for (const RelayDownloadTask &task : m_tasks) {
        QVariantMap item{{"forwardId", task.forwardId},
                         {"destinationPath", task.destinationPath},
                         {"totalBytes", task.totalBytes},
                         {"verifiedBytes", task.verifiedBytes},
                         {"state", task.state},
                         {"error", task.error}};
        result.append(item);
    }
    return result;
}

bool RelayDownloadManager::startTask(const QVariantMap &taskMap)
{
    const QString forwardId = taskMap.value("forwardId").toString();
    const QString targetDeviceId = taskMap.value("targetDeviceId").toString();
    const QString destinationPath = taskMap.value("destinationPath").toString();
    const QString channel = taskMap.value("channel").toString();
    const QString serverState = taskMap.value("state").toString();
    const QVariantList files = taskMap.value("files").toList();

    if (forwardId.isEmpty() || targetDeviceId != m_currentDeviceId || channel != "RELAY" ||
        serverState != "TRANSFERRING" || !isValidRelativePath(destinationPath) ||
        files.isEmpty() || m_baseUrl.isEmpty() || m_authToken.isEmpty() ||
        m_currentDeviceToken.isEmpty() || m_receiveRoot.isEmpty()) {
        setLastError(tr("Invalid or unauthorized RELAY download task"));
        return false;
    }
    RelayDownloadTask task;
    task.forwardId = forwardId;
    task.destinationPath = destinationPath;
    task.totalBytes = taskMap.value("totalBytes").toLongLong();
    task.state = "PENDING";
    qint64 manifestBytes = 0;
    QSet<QString> paths;
    static const QRegularExpression shaPattern("^[0-9a-f]{64}$");
    for (const QVariant &entry : files) {
        const QVariantMap fileMap = entry.toMap();
        RelayDownloadFile file;
        file.path = fileMap.value("path").toString();
        file.size = fileMap.value("size").toLongLong();
        file.sha256 = fileMap.value("sha256").toString();
        if (!isValidRelativePath(file.path) || file.size < 0 ||
            !shaPattern.match(file.sha256).hasMatch() || paths.contains(file.path)) {
            setLastError(tr("Invalid RELAY file manifest"));
            return false;
        }
        paths.insert(file.path);
        manifestBytes += file.size;
        task.files.append(file);
    }
    if (manifestBytes != task.totalBytes) {
        setLastError(tr("RELAY manifest size does not match totalBytes"));
        return false;
    }
    QString pathError;
    if (!prepareTargetPaths(task, &pathError)) {
        setLastError(pathError);
        return false;
    }

    if (m_activeForwardId == forwardId || m_queue.contains(forwardId)) {
        return true;
    }
    if (m_tasks.contains(forwardId)) {
        RelayDownloadTask &saved = m_tasks[forwardId];
        bool sameManifest = saved.destinationPath == task.destinationPath &&
                            saved.totalBytes == task.totalBytes &&
                            saved.files.size() == task.files.size();
        for (int index = 0; sameManifest && index < saved.files.size(); ++index) {
            sameManifest = saved.files.at(index).path == task.files.at(index).path &&
                           saved.files.at(index).size == task.files.at(index).size &&
                           saved.files.at(index).sha256 == task.files.at(index).sha256;
        }
        if (!sameManifest) {
            setLastError(tr("RELAY manifest changed during recovery"));
            return false;
        }
        if (saved.state == "COMPLETED") return true;
        if (saved.state == "AWAITING_SERVER_COMPLETE") {
            if (!saved.completionEmitted) {
                saved.completionEmitted = true;
                emit taskDownloadCompleted(saved.forwardId, saved.totalBytes);
            }
            return true;
        }
    }

    m_tasks[forwardId] = task;
    m_queue.append(forwardId);
    saveTasks();
    emit downloadsChanged();
    processQueue();
    return true;
}

void RelayDownloadManager::cancelTask(const QString &forwardId)
{
    m_queue.removeAll(forwardId);
    if (m_activeForwardId == forwardId) {
        m_activeForwardId.clear();
        if (m_reply) m_reply->abort();
        emit busyChanged();
    }
    if (m_tasks.contains(forwardId)) {
        m_tasks[forwardId].state = "CANCELLED";
        saveTasks();
        emit downloadsChanged();
    }
    processQueue();
}

void RelayDownloadManager::retryTask(const QString &forwardId)
{
    if (!m_tasks.contains(forwardId) || m_activeForwardId == forwardId ||
        m_queue.contains(forwardId)) return;
    RelayDownloadTask &task = m_tasks[forwardId];
    task.state = "PENDING";
    task.error.clear();
    task.retryCount = 0;
    m_queue.append(forwardId);
    saveTasks();
    emit downloadsChanged();
    processQueue();
}

void RelayDownloadManager::confirmTaskCompleted(const QString &forwardId)
{
    if (!m_tasks.contains(forwardId)) return;
    RelayDownloadTask &task = m_tasks[forwardId];
    task.state = "COMPLETED";
    task.error.clear();
    task.finalizationRetryCount = 0;
    saveTasks();
    emit downloadsChanged();
}

void RelayDownloadManager::retryFinalization(const QString &forwardId)
{
    if (!m_tasks.contains(forwardId)) return;
    RelayDownloadTask &task = m_tasks[forwardId];
    if (task.state != "AWAITING_SERVER_COMPLETE") return;
    task.completionEmitted = true;
    emit taskDownloadCompleted(task.forwardId, task.totalBytes);
}

void RelayDownloadManager::handleFinalizationFailure(const QString &forwardId,
                                                     const QString &code)
{
    if (!m_tasks.contains(forwardId)) return;
    RelayDownloadTask &task = m_tasks[forwardId];
    if (task.state != "AWAITING_SERVER_COMPLETE") return;
    if (code == "UNAUTHORIZED" || code == "FORWARD_NOT_FOUND" ||
        code == "INVALID_REQUEST" || code == "INVALID_PATH") {
        task.error = code;
        saveTasks();
        emit downloadsChanged();
        return;
    }
    ++task.finalizationRetryCount;
    if (task.finalizationRetryCount > MaxNetworkRetries) {
        task.error = tr("Forward completion retry limit reached");
        setLastError(task.error);
        saveTasks();
        emit downloadsChanged();
        return;
    }
    task.completionEmitted = false;
    saveTasks();
    QTimer::singleShot(retryDelayMs(task.finalizationRetryCount), this, [this, forwardId]() {
        if (!m_tasks.contains(forwardId)) return;
        RelayDownloadTask &pending = m_tasks[forwardId];
        if (pending.state == "AWAITING_SERVER_COMPLETE" && !pending.completionEmitted) {
            pending.completionEmitted = true;
            emit taskDownloadCompleted(pending.forwardId, pending.totalBytes);
        }
    });
}

void RelayDownloadManager::processQueue()
{
    if (m_reply || !m_activeForwardId.isEmpty()) return;
    while (!m_queue.isEmpty() && !m_tasks.contains(m_queue.first())) m_queue.removeFirst();
    if (m_queue.isEmpty()) return;
    m_activeForwardId = m_queue.takeFirst();
    RelayDownloadTask &task = m_tasks[m_activeForwardId];
    task.state = "DOWNLOADING";
    task.error.clear();
    emit busyChanged();
    emit downloadsChanged();
    processCurrentFile();
}

void RelayDownloadManager::processCurrentFile()
{
    if (m_activeForwardId.isEmpty() || !m_tasks.contains(m_activeForwardId)) return;
    RelayDownloadTask &task = m_tasks[m_activeForwardId];
    while (task.currentFileIndex < task.files.size() &&
           task.files[task.currentFileIndex].completed) {
        ++task.currentFileIndex;
    }
    if (task.currentFileIndex >= task.files.size()) {
        finishTask();
        return;
    }

    RelayDownloadFile &file = task.files[task.currentFileIndex];
    const QFileInfo finalInfo(file.finalPath);
    if (finalInfo.exists()) {
        if (!finalInfo.isFile() || finalInfo.isSymLink() || finalInfo.size() != file.size ||
            calculateFileSha256(file.finalPath) != file.sha256) {
            failTask("TARGET_ALREADY_EXISTS", tr("Target file exists with different content"));
            return;
        }
        file.completed = true;
        task.verifiedBytes += file.size;
        ++task.currentFileIndex;
        saveTasks();
        emit downloadsChanged();
        if (task.currentFileIndex < task.files.size()) {
            emit progressReady(task.forwardId, task.verifiedBytes);
        }
        processCurrentFile();
        return;
    }

    const QFileInfo partInfo(file.partPath);
    if (partInfo.exists() && (partInfo.isSymLink() || !partInfo.isFile())) {
        failTask("INVALID_PATH", tr("Unsafe partial file path"));
        return;
    }
    if (partInfo.size() > file.size) {
        if (!archivePartFile(file.partPath)) {
            failTask("STORAGE_ERROR", tr("Cannot archive oversized partial file"));
            return;
        }
    }
    if (QFileInfo(file.partPath).exists() && QFileInfo(file.partPath).size() == file.size) {
        finishCurrentFile();
        return;
    }
    if (file.size == 0) {
        QFile empty(file.partPath);
        if (!empty.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            failTask("STORAGE_ERROR", tr("Cannot create empty partial file"));
            return;
        }
        empty.close();
        finishCurrentFile();
        return;
    }
    beginRangeDownload(task, file);
}

void RelayDownloadManager::beginRangeDownload(RelayDownloadTask &task, RelayDownloadFile &file)
{
    auto context = QSharedPointer<RangeContext>::create();
    context->file = QSharedPointer<QFile>::create(file.partPath);
    if (!context->file->open(QIODevice::ReadWrite)) {
        failTask("STORAGE_ERROR", tr("Cannot open partial file"));
        return;
    }
    context->offset = context->file->size();
    context->expectedSize = file.size;
    if (!context->file->seek(context->offset)) {
        context->file->close();
        failTask("STORAGE_ERROR", tr("Cannot seek partial file"));
        return;
    }

    QUrl url(m_baseUrl + "/api/v1/forwards/" +
             QString::fromUtf8(QUrl::toPercentEncoding(task.forwardId)) + "/files/content");
    QUrlQuery query;
    query.addQueryItem("path", file.path);
    url.setQuery(query);
    QNetworkRequest request(url);
    applyRequestDefaults(request);
    request.setRawHeader("Range", "bytes=" + QByteArray::number(context->offset) + "-");
    m_reply = m_network.get(request);

    connect(m_reply, &QNetworkReply::metaDataChanged, this,
            [this, context, expectedSha256 = file.sha256]() {
        if (!m_reply || context->headersChecked) return;
        context->headersChecked = true;
        const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 206) return;
        if (m_reply->rawHeader("Accept-Ranges").toLower() != "bytes") {
            context->protocolError = tr("RELAY response has no byte range support");
            return;
        }
        if (QString::fromUtf8(m_reply->rawHeader("X-File-SHA256")) != expectedSha256) {
            context->protocolError = tr("RELAY response hash header does not match manifest");
            return;
        }
        static const QRegularExpression rangePattern("^bytes ([0-9]+)-([0-9]+)/([0-9]+)$");
        const QRegularExpressionMatch match = rangePattern.match(
            QString::fromLatin1(m_reply->rawHeader("Content-Range")));
        if (!match.hasMatch()) {
            context->protocolError = tr("Invalid Content-Range response");
            return;
        }
        const qint64 start = match.captured(1).toLongLong();
        const qint64 end = match.captured(2).toLongLong();
        const qint64 total = match.captured(3).toLongLong();
        const qint64 length = m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (start != context->offset || end != context->expectedSize - 1 ||
            total != context->expectedSize || length != end - start + 1) {
            context->protocolError = tr("RELAY range metadata does not match the manifest");
            return;
        }
        context->accepted = true;
    });

    connect(m_reply, &QNetworkReply::readyRead, this, [this, context]() {
        if (!m_reply) return;
        const QByteArray data = m_reply->readAll();
        if (context->accepted) {
            if (context->file->write(data) != data.size()) {
                context->protocolError = tr("Cannot write partial file");
                m_reply->abort();
            }
        } else {
            context->errorBody.append(data);
        }
    });

    QNetworkReply *reply = m_reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkMessage = reply->errorString();
        const QByteArray remaining = reply->readAll();
        if (context->accepted) {
            if (!remaining.isEmpty() && context->file->write(remaining) != remaining.size()) {
                context->protocolError = tr("Cannot write partial file");
            }
        } else {
            context->errorBody.append(remaining);
        }
        context->file->flush();
        context->file->close();
        reply->deleteLater();
        if (m_reply == reply) m_reply = nullptr;

        if (m_activeForwardId.isEmpty() || !m_tasks.contains(m_activeForwardId)) {
            emit busyChanged();
            processQueue();
            return;
        }
        RelayDownloadTask &task = m_tasks[m_activeForwardId];
        RelayDownloadFile &file = task.files[task.currentFileIndex];
        const QJsonObject error = parseError(context->errorBody);
        const QString code = error.value("code").toString();
        if (status == 416 || code == "RANGE_NOT_SATISFIABLE") {
            if (file.recoveryCount++ < MaxLocalRecoveries && archivePartFile(file.partPath)) {
                saveTasks();
                processCurrentFile();
            } else {
                failTask("RANGE_NOT_SATISFIABLE", tr("Unable to recover the partial file range"));
            }
            return;
        }
        if (networkError != QNetworkReply::NoError && (status == 0 || status == 206)) {
            task.error = networkMessage;
            scheduleNetworkRetry();
            return;
        }
        if (status != 206 || !context->accepted || !context->protocolError.isEmpty()) {
            failTask(code.isEmpty() ? "INVALID_RESPONSE" : code,
                     context->protocolError.isEmpty()
                         ? (error.value("message").toString().isEmpty()
                                ? networkMessage : error.value("message").toString())
                         : context->protocolError);
            return;
        }
        task.retryCount = 0;
        if (QFileInfo(file.partPath).size() != file.size) {
            failTask("INVALID_RESPONSE", tr("Downloaded byte count does not match manifest"));
            return;
        }
        finishCurrentFile();
    });
}

void RelayDownloadManager::finishCurrentFile()
{
    RelayDownloadTask &task = m_tasks[m_activeForwardId];
    RelayDownloadFile &file = task.files[task.currentFileIndex];
    if (QFileInfo(file.partPath).size() != file.size ||
        calculateFileSha256(file.partPath) != file.sha256) {
        if (file.recoveryCount++ < MaxLocalRecoveries && archivePartFile(file.partPath)) {
            saveTasks();
            processCurrentFile();
        } else {
            failTask("FILE_HASH_MISMATCH", tr("Downloaded file failed SHA-256 verification"));
        }
        return;
    }
    if (!QFile::rename(file.partPath, file.finalPath)) {
        failTask("STORAGE_ERROR", tr("Cannot atomically finalize downloaded file"));
        return;
    }
    file.completed = true;
    task.verifiedBytes += file.size;
    ++task.currentFileIndex;
    saveTasks();
    emit downloadsChanged();
    if (task.currentFileIndex < task.files.size()) {
        emit progressReady(task.forwardId, task.verifiedBytes);
    }
    processCurrentFile();
}

void RelayDownloadManager::finishTask()
{
    RelayDownloadTask &task = m_tasks[m_activeForwardId];
    if (task.verifiedBytes != task.totalBytes) {
        failTask("FORWARD_INCOMPLETE", tr("Verified bytes do not match task total"));
        return;
    }
    task.state = "AWAITING_SERVER_COMPLETE";
    task.error.clear();
    task.completionEmitted = true;
    const QString forwardId = task.forwardId;
    const qint64 totalBytes = task.totalBytes;
    m_activeForwardId.clear();
    saveTasks();
    emit downloadsChanged();
    emit busyChanged();
    emit taskDownloadCompleted(forwardId, totalBytes);
    processQueue();
}

void RelayDownloadManager::failTask(const QString &code, const QString &message)
{
    if (m_activeForwardId.isEmpty() || !m_tasks.contains(m_activeForwardId)) return;
    RelayDownloadTask &task = m_tasks[m_activeForwardId];
    task.state = "FAILED";
    task.error = message;
    const QString forwardId = task.forwardId;
    m_activeForwardId.clear();
    setLastError(message);
    saveTasks();
    emit downloadsChanged();
    emit busyChanged();
    emit taskDownloadFailed(forwardId, code, message);
    processQueue();
}

void RelayDownloadManager::scheduleNetworkRetry()
{
    RelayDownloadTask &task = m_tasks[m_activeForwardId];
    ++task.retryCount;
    if (task.retryCount > MaxNetworkRetries) {
        failTask("NETWORK_ERROR", tr("RELAY download retry limit reached"));
        return;
    }
    saveTasks();
    const QString forwardId = m_activeForwardId;
    QTimer::singleShot(retryDelayMs(task.retryCount), this, [this, forwardId]() {
        if (m_activeForwardId == forwardId && m_tasks.contains(forwardId)) {
            processCurrentFile();
        }
    });
}

bool RelayDownloadManager::prepareTargetPaths(RelayDownloadTask &task, QString *error)
{
    for (RelayDownloadFile &file : task.files) {
        if (!safeTargetPaths(task.destinationPath, file.path,
                             &file.finalPath, &file.partPath, error)) return false;
    }
    return true;
}

bool RelayDownloadManager::safeTargetPaths(const QString &destinationPath,
                                           const QString &filePath,
                                           QString *finalPath, QString *partPath,
                                           QString *error) const
{
    if (!isValidRelativePath(destinationPath) || !isValidRelativePath(filePath)) {
        if (error) *error = tr("Unsafe destination path");
        return false;
    }
    QDir root(m_receiveRoot);
    if (!root.exists() && !QDir().mkpath(root.absolutePath())) {
        if (error) *error = tr("Cannot create receive root");
        return false;
    }
    const QString canonicalRoot = QFileInfo(root.absolutePath()).canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        if (error) *error = tr("Cannot resolve receive root");
        return false;
    }

    const QString relative = destinationPath + "/" + filePath;
    const QStringList parts = relative.split('/');
    QString current = canonicalRoot;
    for (qsizetype index = 0; index + 1 < parts.size(); ++index) {
        current = QDir(current).filePath(parts.at(index));
        QFileInfo info(current);
        if (info.exists() && (info.isSymLink() || !info.isDir())) {
            if (error) *error = tr("Destination contains an unsafe directory component");
            return false;
        }
        if (!info.exists() && !QDir().mkdir(current)) {
            if (error) *error = tr("Cannot create destination directory");
            return false;
        }
        const QString canonical = QFileInfo(current).canonicalFilePath();
        if (canonical != canonicalRoot && !canonical.startsWith(canonicalRoot + '/')) {
            if (error) *error = tr("Destination escapes the receive root");
            return false;
        }
    }
    const QString candidate = QDir(current).filePath(parts.last());
    const QString partial = candidate + ".part";
    if (QFileInfo(candidate).isSymLink() || QFileInfo(partial).isSymLink()) {
        if (error) *error = tr("Destination file is a symbolic link");
        return false;
    }
    *finalPath = candidate;
    *partPath = partial;
    return true;
}

bool RelayDownloadManager::archivePartFile(const QString &partPath) const
{
    if (!QFileInfo::exists(partPath)) return true;
    const QString archive = partPath + ".invalid-" +
        QString::number(QDateTime::currentMSecsSinceEpoch());
    return QFile::rename(partPath, archive);
}

bool RelayDownloadManager::isValidRelativePath(const QString &path)
{
    if (path.isEmpty() || path.startsWith('/') || path.endsWith('/') ||
        path.contains('\\') || path.contains("//") || path.contains(QChar::Null)) return false;
    const QStringList parts = path.split('/');
    if (path.size() >= 2 && path.at(0).isLetter() && path.at(1) == ':') return false;
    for (const QString &part : parts) {
        if (part.isEmpty() || part == "." || part == "..") return false;
    }
    return true;
}

QString RelayDownloadManager::calculateFileSha256(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) return QString();
    return QString::fromLatin1(hash.result().toHex());
}

void RelayDownloadManager::setLastError(const QString &error)
{
    if (m_lastError == error) return;
    m_lastError = error;
    emit lastErrorChanged();
}

void RelayDownloadManager::applyRequestDefaults(QNetworkRequest &request) const
{
    request.setTransferTimeout(RequestTimeoutMs);
    request.setRawHeader("Authorization", m_authToken.toUtf8());
    request.setRawHeader("X-Device-Id", m_currentDeviceId.toUtf8());
    request.setRawHeader("X-Device-Token", m_currentDeviceToken.toUtf8());
}

QString RelayDownloadManager::storageFilePath() const
{
    const QString overrideDirectory = QProcessEnvironment::systemEnvironment().value(
        "MYFOLDER_DOWNLOAD_STATE_DIR");
    QDir directory(overrideDirectory.isEmpty()
                       ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       : overrideDirectory);
    if (!directory.exists()) directory.mkpath(".");
    return directory.filePath("relay_downloads_v1_1_1.json");
}

void RelayDownloadManager::saveTasks()
{
    QJsonArray tasks;
    for (const RelayDownloadTask &task : m_tasks) {
        QJsonArray files;
        for (const RelayDownloadFile &file : task.files) {
            files.append(QJsonObject{{"path", file.path}, {"size", file.size},
                                     {"sha256", file.sha256}, {"completed", file.completed},
                                     {"recoveryCount", file.recoveryCount}});
        }
        tasks.append(QJsonObject{{"forwardId", task.forwardId},
                                 {"destinationPath", task.destinationPath},
                                 {"totalBytes", task.totalBytes},
                                 {"verifiedBytes", task.verifiedBytes},
                                 {"state", task.state}, {"error", task.error},
                                 {"finalizationRetryCount", task.finalizationRetryCount},
                                 {"files", files}});
    }
    QSaveFile file(storageFilePath());
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(QJsonObject{{"schemaVersion", 1}, {"tasks", tasks}})
                   .toJson(QJsonDocument::Compact));
    file.commit();
}

void RelayDownloadManager::loadTasks()
{
    QFile file(storageFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonArray tasks = QJsonDocument::fromJson(file.readAll()).object().value("tasks").toArray();
    for (const QJsonValue &value : tasks) {
        const QJsonObject object = value.toObject();
        RelayDownloadTask task;
        task.forwardId = object.value("forwardId").toString();
        task.destinationPath = object.value("destinationPath").toString();
        task.totalBytes = object.value("totalBytes").toInteger();
        task.verifiedBytes = object.value("verifiedBytes").toInteger();
        task.state = object.value("state").toString();
        task.error = object.value("error").toString();
        task.finalizationRetryCount = object.value("finalizationRetryCount").toInt();
        for (const QJsonValue &fileValue : object.value("files").toArray()) {
            const QJsonObject fileObject = fileValue.toObject();
            RelayDownloadFile downloadFile;
            downloadFile.path = fileObject.value("path").toString();
            downloadFile.size = fileObject.value("size").toInteger();
            downloadFile.sha256 = fileObject.value("sha256").toString();
            downloadFile.completed = fileObject.value("completed").toBool();
            downloadFile.recoveryCount = fileObject.value("recoveryCount").toInt();
            task.files.append(downloadFile);
        }
        if (!task.forwardId.isEmpty()) m_tasks.insert(task.forwardId, task);
    }
}

QJsonObject RelayDownloadManager::parseError(const QByteArray &payload)
{
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    return document.isObject() ? document.object() : QJsonObject();
}

qint64 RelayDownloadManager::retryDelayMs(int retryCount)
{
    return qMin<qint64>(30000, 1000LL << qBound(0, retryCount - 1, 5));
}
