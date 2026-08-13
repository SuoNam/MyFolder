#include "transfermanager.h"
#include "serverconfig.h"
#include <QUuid>
#include <QDateTime>
#include <QHostInfo>
#include <QRandomGenerator>
#include <cmath>

TransferManager::TransferManager(QObject *parent)
    : QObject(parent),
      m_netManager(new QNetworkAccessManager(this)),
      m_baseUrl(MyFolderServerConfig::baseUrl()),
      m_queueTimer(new QTimer(this)),
      m_networkCheckTimer(new QTimer(this)),
      m_speedTimer(new QTimer(this))
{
    if (m_baseUrl.endsWith('/')) {
        m_baseUrl.chop(1);
    }

    connect(m_queueTimer, &QTimer::timeout, this, &TransferManager::processNextChunk);
    m_queueTimer->start(200);

    connect(m_networkCheckTimer, &QTimer::timeout, this, &TransferManager::checkNetworkStatus);
    m_networkCheckTimer->start(10000);

    connect(m_speedTimer, &QTimer::timeout, this, &TransferManager::updateSpeedStats);
    m_speedTimer->start(1000);

    loadTasksFromStorage();
}

TransferManager::~TransferManager()
{
    saveTasksToStorage();
}

void TransferManager::setBaseUrl(const QString &url)
{
    QString cleanUrl = url;
    if (cleanUrl.endsWith('/')) {
        cleanUrl.chop(1);
    }
    if (!cleanUrl.isEmpty() && m_baseUrl != cleanUrl) {
        m_baseUrl = cleanUrl;
        emit baseUrlChanged();
    }
}

void TransferManager::setAuthToken(const QString &token)
{
    if (m_authToken != token) {
        m_authToken = token;
        emit authTokenChanged();
        if (!m_authToken.isEmpty()) {
            for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                if (!it.value().pausedByUser && it.value().state != "COMPLETED" &&
                    it.value().state != "CANCELLED") {
                    QString sourceError;
                    if (!validateLocalSources(it.value(), &sourceError)) {
                        it.value().state = "FAILED";
                        it.value().pausedByUser = true;
                        emit taskStatusChanged(it.key(), "FAILED", sourceError);
                        continue;
                    }
                    it.value().statusSyncedWithServer = it.key().startsWith("temp_");
                    if (!m_activeQueue.contains(it.key())) {
                        m_activeQueue.append(it.key());
                    }
                }
            }
            if (!m_tasks.isEmpty()) {
                saveTasksToStorage();
                emit taskListChanged();
            }
        }
    }
}

QString TransferManager::calculateFileHash(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for SHA-256 calculation:" << filePath;
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    char buffer[65536];
    qint64 bytesRead = 0;
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
        hash.addData(QByteArrayView(buffer, bytesRead));
    }

    file.close();
    return hash.result().toHex().toLower();
}

QString TransferManager::calculateChunkHash(const QString &filePath, qint64 offset, qint64 size)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    if (!file.seek(offset)) {
        file.close();
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    char buffer[65536];
    qint64 remaining = size;
    while (remaining > 0) {
        qint64 toRead = qMin(remaining, static_cast<qint64>(sizeof(buffer)));
        qint64 bytesRead = file.read(buffer, toRead);
        if (bytesRead <= 0) break;
        hash.addData(QByteArrayView(buffer, bytesRead));
        remaining -= bytesRead;
    }

    file.close();
    if (remaining != 0) {
        return QString();
    }
    return hash.result().toHex().toLower();
}

// Percent encode file path strictly (docs/a b.txt -> docs%2Fa%20b.txt)
QString TransferManager::encodePathForProtocol(const QString &relativePath)
{
    return QString::fromUtf8(QUrl::toPercentEncoding(relativePath));
}

bool TransferManager::isValidProtocolPath(const QString &path)
{
    if (path.isEmpty() || path.startsWith('/') || path.endsWith('/') ||
        path.contains('\\') || path.contains(QChar::Null)) {
        return false;
    }
    if (path.size() >= 2 && path.at(0).isLetter() && path.at(1) == ':') {
        return false;
    }

    const QStringList segments = path.split('/', Qt::KeepEmptyParts);
    for (const QString &segment : segments) {
        if (segment.isEmpty() || segment == "." || segment == "..") {
            return false;
        }
    }
    return true;
}

qint64 TransferManager::calculateUploadedBytes(qint64 fileSize, qint64 chunkSize,
                                               const QSet<int> &completedChunks)
{
    if (fileSize <= 0 || chunkSize <= 0) {
        return 0;
    }

    qint64 uploaded = 0;
    for (int chunkIndex : completedChunks) {
        if (chunkIndex < 0) {
            continue;
        }
        const qint64 start = static_cast<qint64>(chunkIndex) * chunkSize;
        if (start >= fileSize) {
            continue;
        }
        uploaded += qMin(chunkSize, fileSize - start);
    }
    return qMin(fileSize, uploaded);
}

// Exponential backoff starting at 1s (1s, 2s, 4s, 8s, 16s, max 30s) + 0%-20% jitter
qint64 TransferManager::calculateBackoffDelayMs(int retryCount)
{
    int exponent = qBound(0, retryCount - 1, 5); // retryCount=1 -> 1s, retryCount=2 -> 2s, etc.
    qint64 baseMs = 1000LL * (1LL << exponent);
    if (baseMs > 30000LL) baseMs = 30000LL;

    qint64 maxJitter = static_cast<qint64>(baseMs * 0.2); // 0% - 20% jitter
    qint64 jitter = QRandomGenerator::global()->bounded(maxJitter + 1);
    return baseMs + jitter;
}

void TransferManager::applyRequestDefaults(QNetworkRequest &request) const
{
    request.setTransferTimeout(REQUEST_TIMEOUT_MS);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", m_authToken.toUtf8());
    }
}

void TransferManager::recalculateProgress(TransferTaskGroup &group) const
{
    group.uploadedBytes = 0;
    for (TransferFileItem &file : group.files) {
        file.uploadedBytes = calculateUploadedBytes(file.size, group.chunkSize,
                                                    file.completedChunks);
        group.uploadedBytes += file.uploadedBytes;
    }
    group.uploadedBytes = qMin(group.totalBytes, group.uploadedBytes);
}

bool TransferManager::applyServerTaskResponse(TransferTaskGroup &group,
                                              const QJsonObject &response,
                                              QString *errorMessage) const
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    };
    static const QSet<QString> states{"PENDING", "UPLOADING", "VERIFYING",
                                      "COMPLETED", "FAILED", "CANCELLED"};

    const QString responseId = response.value("uploadId").toString();
    const QString state = response.value("state").toString();
    const qint64 responseChunkSize = response.value("chunkSize").toVariant().toLongLong();
    const qint64 responseTotalBytes = response.value("totalBytes").toVariant().toLongLong();
    const int responseTotalFiles = response.value("totalFiles").toInt(-1);
    if (responseId.isEmpty() || responseId != group.uploadId || !states.contains(state) ||
        responseChunkSize != group.chunkSize || responseTotalBytes != group.totalBytes ||
        responseTotalFiles != group.totalFiles) {
        return fail(tr("Upload task response does not match the local manifest"));
    }

    if (response.value("targetPath").toString() != group.targetPath) {
        return fail(tr("Upload task target path changed on the server"));
    }

    const QJsonArray serverFiles = response.value("files").toArray();
    if (serverFiles.size() != group.files.size()) {
        return fail(tr("Upload task file manifest size changed on the server"));
    }
    QMap<QString, QJsonObject> byPath;
    for (const QJsonValue &value : serverFiles) {
        if (!value.isObject()) {
            return fail(tr("Upload task contains an invalid file response"));
        }
        const QJsonObject object = value.toObject();
        const QString path = object.value("path").toString();
        if (path.isEmpty() || byPath.contains(path)) {
            return fail(tr("Upload task contains duplicate or empty file paths"));
        }
        byPath.insert(path, object);
    }

    for (TransferFileItem &file : group.files) {
        if (!byPath.contains(file.path)) {
            return fail(tr("Upload task file is missing from the server response"));
        }
        const QJsonObject serverFile = byPath.value(file.path);
        const QString fileState = serverFile.value("state").toString();
        if (serverFile.value("size").toVariant().toLongLong() != file.size ||
            serverFile.value("sha256").toString().compare(file.sha256, Qt::CaseInsensitive) != 0 ||
            serverFile.value("totalChunks").toInt(-1) != file.totalChunks ||
            !states.contains(fileState)) {
            return fail(tr("Upload task file response does not match the local manifest"));
        }

        QSet<int> completed;
        QList<int> missing;
        for (const QJsonValue &value : serverFile.value("completedChunks").toArray()) {
            const int index = value.toInt(-1);
            if (index < 0 || index >= file.totalChunks || completed.contains(index)) {
                return fail(tr("Server returned an invalid completed chunk list"));
            }
            completed.insert(index);
        }
        QSet<int> seenMissing;
        for (const QJsonValue &value : serverFile.value("missingChunks").toArray()) {
            const int index = value.toInt(-1);
            if (index < 0 || index >= file.totalChunks || completed.contains(index) ||
                seenMissing.contains(index)) {
                return fail(tr("Server returned an invalid missing chunk list"));
            }
            seenMissing.insert(index);
            missing.append(index);
        }
        if (fileState != "COMPLETED" && completed.size() + seenMissing.size() != file.totalChunks) {
            return fail(tr("Server chunk lists do not cover the complete file"));
        }

        file.state = fileState;
        file.completedChunks = completed;
        file.missingChunks = missing;
        file.failureReason = serverFile.value("failureReason").toString();
        file.completionInFlight = false;
    }

    group.state = state;
    recalculateProgress(group);
    return true;
}

bool TransferManager::isRetryableError(const QString &code, int httpStatus) const
{
    return code == "CHUNK_HASH_MISMATCH" || httpStatus == 0 || httpStatus == 408 ||
           httpStatus == 429 || httpStatus == 500 || httpStatus == 502 ||
           httpStatus == 503 || httpStatus == 504;
}

QString TransferManager::normalizedParentPath(const QString &path) const
{
    QString normalized = path.trimmed();
    while (normalized.startsWith('/')) {
        normalized.remove(0, 1);
    }
    while (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    if (normalized.isEmpty()) {
        normalized = "inbox";
    }
    return isValidProtocolPath(normalized) ? normalized : QString();
}

QString TransferManager::endpointFor(const TransferTaskGroup &group) const
{
    return group.serverBaseUrl.isEmpty() ? m_baseUrl : group.serverBaseUrl;
}

bool TransferManager::validateLocalSources(TransferTaskGroup &group, QString *errorMessage) const
{
    const bool rootIsDirectory = QFileInfo(group.localRootPath).isDir();
    for (TransferFileItem &file : group.files) {
        const QString localPath = rootIsDirectory
            ? QDir(group.localRootPath).filePath(file.path)
            : group.localRootPath;
        const QFileInfo info(localPath);
        if (!info.exists() || !info.isFile() || info.size() != file.size ||
            calculateFileHash(localPath) != file.sha256) {
            file.failureReason = "Local source changed since the upload task was created";
            file.state = "FAILED";
            if (errorMessage) {
                *errorMessage = file.failureReason + ": " + file.path;
            }
            return false;
        }
    }
    return true;
}

QVariantList TransferManager::getTaskList() const
{
    QVariantList list;
    for (auto it = m_tasks.constBegin(); it != m_tasks.constEnd(); ++it) {
        const TransferTaskGroup &group = it.value();
        QVariantMap map;
        map["uploadId"] = group.uploadId;
        map["directoryName"] = group.directoryName;
        map["parentPath"] = group.parentPath;
        map["targetPath"] = group.targetPath;
        map["paused"] = group.pausedByUser;
        map["totalFiles"] = group.totalFiles;
        map["totalBytes"] = group.totalBytes;
        map["uploadedBytes"] = group.uploadedBytes;
        map["state"] = group.state;

        double progress = (group.totalBytes > 0) ? static_cast<double>(group.uploadedBytes) / group.totalBytes : 0.0;
        map["progress"] = qBound(0.0, progress, 1.0);

        QVariantList filesList;
        for (const TransferFileItem &f : group.files) {
            QVariantMap fMap;
            fMap["path"] = f.path;
            fMap["size"] = f.size;
            fMap["sha256"] = f.sha256;
            fMap["totalChunks"] = f.totalChunks;
            fMap["state"] = f.state;
            fMap["uploadedBytes"] = f.uploadedBytes;
            fMap["missingChunksCount"] = f.missingChunks.size();
            double fProg = (f.size > 0) ? static_cast<double>(f.uploadedBytes) / f.size : 0.0;
            fMap["progress"] = qBound(0.0, fProg, 1.0);
            filesList.append(fMap);
        }
        map["files"] = filesList;

        list.append(map);
    }
    return list;
}

QString TransferManager::startFileUpload(const QString &localFilePath, const QString &parentPath)
{
    QString cleanPath = localFilePath;
#ifdef Q_OS_WIN
    if (cleanPath.startsWith("file:///")) {
        cleanPath.remove(0, 8);
    } else if (cleanPath.startsWith("file://")) {
        cleanPath.remove(0, 7);
    }
#endif
    QFileInfo fi(cleanPath);
    if (!fi.exists() || !fi.isFile()) {
        qWarning() << "File does not exist:" << cleanPath;
        return QString();
    }

    TransferTaskGroup group;
    group.uploadId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    group.localRootPath = fi.absoluteFilePath();
    group.parentPath = normalizedParentPath(parentPath);
    group.directoryName = group.parentPath == "relay"
                              ? "pc-" + group.uploadId.mid(5)
                              : fi.fileName();
    group.targetPath = group.parentPath == "relay"
                           ? group.parentPath + "/" + group.directoryName
                           : group.parentPath;
    group.serverBaseUrl = m_baseUrl;
    if (!isValidProtocolPath(group.directoryName) ||
        !isValidProtocolPath(fi.fileName())) {
        qWarning() << "Invalid protocol path for upload:" << parentPath << fi.fileName();
        return QString();
    }
    group.chunkSize = DEFAULT_CHUNK_SIZE;
    group.state = "PENDING";
    group.statusSyncedWithServer = true;

    TransferFileItem item;
    item.path = fi.fileName();
    item.size = fi.size();
    item.sha256 = calculateFileHash(cleanPath);
    if (item.sha256.isEmpty()) {
        return QString();
    }
    item.totalChunks = item.size == 0 ? 0
                                     : static_cast<int>((item.size + group.chunkSize - 1) / group.chunkSize);
    item.state = "PENDING";
    for (int i = 0; i < item.totalChunks; ++i) {
        item.missingChunks.append(i);
    }

    group.files.append(item);
    group.totalFiles = 1;
    group.totalBytes = item.size;

    m_tasks[group.uploadId] = group;
    m_activeQueue.append(group.uploadId);

    if (!m_authToken.isEmpty()) {
        createServerTask(m_tasks[group.uploadId]);
    }
    saveTasksToStorage();
    emit taskListChanged();

    return group.uploadId;
}

QString TransferManager::startFolderUpload(const QString &localFolderPath, const QString &parentPath)
{
    QString cleanPath = localFolderPath;
#ifdef Q_OS_WIN
    if (cleanPath.startsWith("file:///")) {
        cleanPath.remove(0, 8);
    } else if (cleanPath.startsWith("file://")) {
        cleanPath.remove(0, 7);
    }
#endif
    QDir folderDir(cleanPath);
    if (!folderDir.exists()) {
        qWarning() << "Folder does not exist:" << cleanPath;
        return QString();
    }

    TransferTaskGroup group;
    group.uploadId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    group.localRootPath = cleanPath;
    group.parentPath = normalizedParentPath(parentPath);
    group.directoryName = group.parentPath == "relay"
                              ? "pc-" + group.uploadId.mid(5)
                              : folderDir.dirName();
    group.targetPath = group.parentPath + "/" + group.directoryName;
    group.serverBaseUrl = m_baseUrl;
    if (group.parentPath.isEmpty() || !isValidProtocolPath(group.directoryName)) {
        qWarning() << "Invalid protocol path for folder upload:" << parentPath << group.directoryName;
        return QString();
    }
    group.chunkSize = DEFAULT_CHUNK_SIZE;
    group.state = "PENDING";
    group.statusSyncedWithServer = true;

    QDirIterator it(cleanPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        QString relPath = fi.absoluteFilePath().mid(cleanPath.length());
        relPath.replace('\\', '/');
        if (relPath.startsWith('/')) relPath = relPath.mid(1);

        TransferFileItem item;
        item.path = relPath;
        if (!isValidProtocolPath(item.path)) {
            qWarning() << "Skipping upload because a local path is not protocol-safe:" << item.path;
            return QString();
        }
        item.size = fi.size();
        item.sha256 = calculateFileHash(fi.absoluteFilePath());
        if (item.sha256.isEmpty()) {
            return QString();
        }
        item.totalChunks = item.size == 0 ? 0
                                         : static_cast<int>((item.size + group.chunkSize - 1) / group.chunkSize);
        item.state = "PENDING";
        for (int i = 0; i < item.totalChunks; ++i) {
            item.missingChunks.append(i);
        }

        group.files.append(item);
        group.totalBytes += item.size;
        group.totalFiles++;
    }

    m_tasks[group.uploadId] = group;
    m_activeQueue.append(group.uploadId);

    if (!m_authToken.isEmpty()) {
        createServerTask(m_tasks[group.uploadId]);
    }
    saveTasksToStorage();
    emit taskListChanged();

    return group.uploadId;
}

void TransferManager::createServerTask(TransferTaskGroup &group)
{
    if (group.creationInFlight || group.pausedByUser) {
        return;
    }
    group.creationInFlight = true;

    QUrl url(endpointFor(group) + "/file/uploadfile");
    QNetworkRequest request(url);
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonObject json;
    json["targetPath"] = group.targetPath;
    json["chunkSize"] = group.chunkSize;
    json["totalFiles"] = group.totalFiles;
    json["totalBytes"] = group.totalBytes;

    QJsonArray filesArr;
    for (const TransferFileItem &f : group.files) {
        QJsonObject fObj;
        fObj["path"] = f.path;
        fObj["size"] = f.size;
        fObj["sha256"] = f.sha256;
        fObj["totalChunks"] = f.totalChunks;
        filesArr.append(fObj);
    }
    json["files"] = filesArr;

    QString oldId = group.uploadId;

    QNetworkReply *reply = m_netManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, oldId]() {
        reply->deleteLater();
        if (!m_tasks.contains(oldId)) return;

        TransferTaskGroup &pendingGroup = m_tasks[oldId];
        pendingGroup.creationInFlight = false;

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray respBytes = reply->readAll();

        if (status == 201) {
            m_isNetworkAvailable = true;
            QJsonObject respJson = QJsonDocument::fromJson(respBytes).object();
            QString serverUploadId = respJson.value("uploadId").toString();
            if (serverUploadId.isEmpty()) {
                QJsonObject malformedError;
                malformedError["code"] = "INVALID_RESPONSE";
                malformedError["message"] = "Create task response has no uploadId";
                handleServerError(malformedError, oldId, status);
                return;
            }

            TransferTaskGroup group = m_tasks.take(oldId);
            m_activeQueue.removeAll(oldId);

            group.uploadId = serverUploadId;
            QString responseError;
            if (!applyServerTaskResponse(group, respJson, &responseError)) {
                group.state = "FAILED";
                group.pausedByUser = true;
                m_tasks[serverUploadId] = group;
                saveTasksToStorage();
                emit taskListChanged();
                emit taskIdChanged(oldId, serverUploadId);
                emit taskStatusChanged(serverUploadId, "FAILED", responseError);
                return;
            }
            group.statusSyncedWithServer = true;
            group.retryCount = 0;
            group.nextRetryTimeMs = 0;

            m_tasks[serverUploadId] = group;
            m_activeQueue.append(serverUploadId);

            saveTasksToStorage();
            emit taskListChanged();
            emit taskIdChanged(oldId, serverUploadId);
            emit taskStatusChanged(serverUploadId, group.state, "");
        } else {
            QJsonObject errJson = QJsonDocument::fromJson(respBytes).object();
            const QString code = errJson.value("code").toString();
            if (isRetryableError(code, status)) {
                pendingGroup.retryCount++;
                if (pendingGroup.retryCount < MAX_RETRIES) {
                    pendingGroup.nextRetryTimeMs = QDateTime::currentMSecsSinceEpoch() +
                                                   calculateBackoffDelayMs(pendingGroup.retryCount);
                    pendingGroup.state = "PENDING";
                    if (status == 0) {
                        m_isNetworkAvailable = false;
                        emit networkStatusChanged(false);
                    }
                    saveTasksToStorage();
                    emit taskListChanged();
                } else {
                    handleServerError(errJson, oldId, status);
                }
            } else {
                handleServerError(errJson, oldId, status);
            }
        }
    });
}

void TransferManager::processNextChunk()
{
    if (m_authToken.isEmpty() || !m_isNetworkAvailable || m_isUploadingChunk ||
        m_activeQueue.isEmpty()) {
        return;
    }

    QString uploadId = m_activeQueue.first();
    if (!m_tasks.contains(uploadId)) {
        m_activeQueue.removeFirst();
        return;
    }

    TransferTaskGroup &group = m_tasks[uploadId];

    if (group.state == "CANCELLED" || group.state == "COMPLETED" || group.pausedByUser) {
        m_activeQueue.removeFirst();
        return;
    }

    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    if (group.nextRetryTimeMs > nowMs) {
        if (m_activeQueue.size() > 1) {
            m_activeQueue.move(0, m_activeQueue.size() - 1);
        }
        return;
    }

    if (group.uploadId.startsWith("temp_")) {
        createServerTask(group);
        return;
    }

    if (group.completionInFlight) {
        return;
    }

    // Mandatory GET task query BEFORE uploading chunks
    if (!group.statusSyncedWithServer) {
        if (!group.isSyncingStatus) {
            queryTaskStatus(uploadId);
        }
        return;
    }

    // Find next missing chunk across files
    for (int fIdx = 0; fIdx < group.files.size(); ++fIdx) {
        TransferFileItem &file = group.files[fIdx];
        if (file.state == "COMPLETED") continue;
        if (file.completionInFlight) continue;

        if (file.nextRetryTimeMs > nowMs) continue;

        if (file.totalChunks == 0) {
            file.state = "VERIFYING";
            completeSingleFile(group, fIdx);
            return;
        }

        if (!file.missingChunks.isEmpty()) {
            int chunkIdx = file.missingChunks.first();
            uploadChunk(group, fIdx, chunkIdx);
            return;
        } else if (file.state != "VERIFYING" && file.state != "COMPLETED") {
            file.state = "VERIFYING";
            completeSingleFile(group, fIdx);
            return;
        }
    }

    // Check if all files completed
    bool allCompleted = true;
    for (const TransferFileItem &f : group.files) {
        if (f.state != "COMPLETED") {
            allCompleted = false;
            break;
        }
    }

    if (allCompleted && group.state != "COMPLETED") {
        completeWholeTask(group);
    } else if (!allCompleted && m_activeQueue.size() > 1) {
        m_activeQueue.move(0, m_activeQueue.size() - 1);
    }
}

void TransferManager::uploadChunk(TransferTaskGroup &group, int fileIndex, int chunkIndex)
{
    m_isUploadingChunk = true;
    TransferFileItem &file = group.files[fileIndex];

    QString localPath = group.localRootPath;
    if (QFileInfo(group.localRootPath).isDir()) {
        localPath = QDir(group.localRootPath).filePath(file.path);
    }

    qint64 start = static_cast<qint64>(chunkIndex) * group.chunkSize;
    qint64 end = qMin(file.size, start + group.chunkSize) - 1;
    qint64 chunkSize = end - start + 1;

    QString chunkHash = calculateChunkHash(localPath, start, chunkSize);

    QFile fileObj(localPath);
    if (!fileObj.open(QIODevice::ReadOnly) || !fileObj.seek(start)) {
        m_isUploadingChunk = false;
        file.state = "FAILED";
        file.failureReason = "Local file read failed";
        emit taskListChanged();
        return;
    }
    QByteArray chunkData = fileObj.read(chunkSize);
    fileObj.close();
    if (chunkHash.isEmpty() || chunkData.size() != chunkSize) {
        m_isUploadingChunk = false;
        file.state = "FAILED";
        file.failureReason = "Local file changed or could not be read completely";
        group.state = "FAILED";
        group.pausedByUser = true;
        saveTasksToStorage();
        emit taskStatusChanged(group.uploadId, "FAILED", file.failureReason);
        emit taskListChanged();
        return;
    }

    QString urlStr = QString("%1/file/uploadfile/%2/chunks/%3")
                        .arg(endpointFor(group), group.uploadId, QString::number(chunkIndex));

    QUrl url(urlStr);
    QNetworkRequest request(url);
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

    QString encodedPath = encodePathForProtocol(file.path);
    request.setRawHeader("X-File-Path", encodedPath.toUtf8());
    request.setRawHeader("X-Chunk-SHA256", chunkHash.toLower().toUtf8());

    QString contentRange = QString("bytes %1-%2/%3").arg(start).arg(end).arg(file.size);
    request.setRawHeader("Content-Range", contentRange.toUtf8());

    QString uploadId = group.uploadId;

    QNetworkReply *reply = m_netManager->put(request, chunkData);
    connect(reply, &QNetworkReply::finished, this, [this, reply, uploadId, fileIndex, chunkIndex, chunkSize]() {
        reply->deleteLater();
        m_isUploadingChunk = false;

        if (!m_tasks.contains(uploadId)) return;
        TransferTaskGroup &grp = m_tasks[uploadId];

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray respBytes = reply->readAll();

        if (status == 200) {
            m_isNetworkAvailable = true;
            TransferFileItem &f = grp.files[fileIndex];
            f.missingChunks.removeAll(chunkIndex);
            f.completedChunks.insert(chunkIndex);
            f.uploadedBytes = qMin(f.size, f.uploadedBytes + chunkSize);
            f.retryCount = 0; // Reset retry counter on success
            f.nextRetryTimeMs = 0;
            f.state = "UPLOADING";
            grp.state = "UPLOADING";
            grp.pausedByUser = false;
            grp.uploadedBytes += chunkSize;
            m_bytesUploadedInWindow += chunkSize;

            emit taskProgressUpdated(uploadId, grp.uploadedBytes, grp.totalBytes, m_globalSpeedMbps);
            saveTasksToStorage();
            emit taskListChanged();
        } else {
            QJsonObject errJson = QJsonDocument::fromJson(respBytes).object();
            QString errCode = errJson.value("code").toString();

            TransferFileItem &f = grp.files[fileIndex];
            f.retryCount++;

            if (isRetryableError(errCode, status) && f.retryCount < MAX_RETRIES) {
                qint64 backoffMs = calculateBackoffDelayMs(f.retryCount);
                f.nextRetryTimeMs = QDateTime::currentMSecsSinceEpoch() + backoffMs;
                f.state = "UPLOADING";
                grp.state = "UPLOADING";
                if (status == 0) {
                    m_isNetworkAvailable = false;
                    emit networkStatusChanged(false);
                }
                qWarning() << "Chunk upload retry" << f.retryCount << "for index" << chunkIndex << "after" << backoffMs << "ms, error:" << errCode;
            } else if (isRetryableError(errCode, status)) {
                f.state = "FAILED";
                f.failureReason = QString("Failed after %1 retries: %2").arg(MAX_RETRIES).arg(errCode);
                grp.state = "FAILED";
                grp.pausedByUser = true;
                emit taskStatusChanged(uploadId, "FAILED", f.failureReason);
            } else {
                handleServerError(errJson, uploadId, status, fileIndex, chunkIndex);
            }
            saveTasksToStorage();
            emit taskListChanged();
        }
    });
}

void TransferManager::completeSingleFile(TransferTaskGroup &group, int fileIndex)
{
    TransferFileItem &file = group.files[fileIndex];
    if (file.completionInFlight) {
        return;
    }
    file.completionInFlight = true;
    QString urlStr = QString("%1/file/uploadfile/%2/files/complete")
                        .arg(endpointFor(group), group.uploadId);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonObject json;
    json["filePath"] = file.path;

    QString uploadId = group.uploadId;

    QNetworkReply *reply = m_netManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, uploadId, fileIndex]() {
        reply->deleteLater();
        if (!m_tasks.contains(uploadId)) return;

        TransferTaskGroup &grp = m_tasks[uploadId];
        if (fileIndex < 0 || fileIndex >= grp.files.size()) return;
        TransferFileItem &f = grp.files[fileIndex];
        f.completionInFlight = false;
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray respBytes = reply->readAll();

        if (status == 200) {
            m_isNetworkAvailable = true;
            f.state = "COMPLETED";
            f.retryCount = 0;
            f.nextRetryTimeMs = 0;
            saveTasksToStorage();
            emit taskListChanged();
        } else {
            QJsonObject errJson = QJsonDocument::fromJson(respBytes).object();
            QString errCode = errJson.value("code").toString();

            if (errCode == "FILE_INCOMPLETE") {
                QJsonObject details = errJson.value("details").toObject();
                QJsonArray missingArr = details.value("missingChunks").toArray();
                f.missingChunks.clear();
                for (const QJsonValue &v : missingArr) {
                    f.missingChunks.append(v.toInt());
                }
                f.state = "UPLOADING";
            } else if (errCode == "FILE_HASH_MISMATCH" || errCode == "FILE_SIZE_MISMATCH") {
                QString sourceError;
                if (!validateLocalSources(grp, &sourceError)) {
                    grp.state = "FAILED";
                    grp.pausedByUser = true;
                    emit taskStatusChanged(uploadId, "FAILED", sourceError);
                } else {
                    grp.statusSyncedWithServer = false;
                    f.state = "UPLOADING";
                    queryTaskStatus(uploadId);
                }
            } else if (errCode == "TASK_STATE_CONFLICT" || errCode == "FILE_NOT_FOUND") {
                grp.statusSyncedWithServer = false;
                f.state = "UPLOADING";
                queryTaskStatus(uploadId);
            } else if (isRetryableError(errCode, status)) {
                f.retryCount++;
                if (f.retryCount < MAX_RETRIES) {
                    qint64 backoffMs = calculateBackoffDelayMs(f.retryCount);
                    f.nextRetryTimeMs = QDateTime::currentMSecsSinceEpoch() + backoffMs;
                    f.state = "UPLOADING";
                    if (status == 0) {
                        m_isNetworkAvailable = false;
                        emit networkStatusChanged(false);
                    }
                } else {
                    f.state = "FAILED";
                    grp.state = "FAILED";
                    grp.pausedByUser = true;
                    emit taskStatusChanged(uploadId, "FAILED", "File completion retry limit reached");
                }
            } else {
                handleServerError(errJson, uploadId, status, fileIndex);
            }
            saveTasksToStorage();
            emit taskListChanged();
        }
    });
}

void TransferManager::completeWholeTask(TransferTaskGroup &group)
{
    if (group.completionInFlight) {
        return;
    }
    group.completionInFlight = true;
    group.state = "VERIFYING";
    QString urlStr = QString("%1/file/uploadfile/%2/complete")
                        .arg(endpointFor(group), group.uploadId);

    QUrl url(urlStr);
    QNetworkRequest request(url);
    applyRequestDefaults(request);

    QString uploadId = group.uploadId;

    QNetworkReply *reply = m_netManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, uploadId]() {
        reply->deleteLater();
        if (!m_tasks.contains(uploadId)) return;

        TransferTaskGroup &grp = m_tasks[uploadId];
        grp.completionInFlight = false;
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status == 200) {
            m_isNetworkAvailable = true;
            grp.state = "COMPLETED";
            grp.retryCount = 0;
            grp.nextRetryTimeMs = 0;
            grp.uploadedBytes = grp.totalBytes;
            m_activeQueue.removeAll(uploadId);
            saveTasksToStorage();
            emit taskListChanged();
            emit taskStatusChanged(uploadId, "COMPLETED", "");
        } else {
            QJsonObject errJson = QJsonDocument::fromJson(reply->readAll()).object();
            const QString code = errJson.value("code").toString();
            if (code == "TASK_STATE_CONFLICT" || code == "FILE_INCOMPLETE") {
                grp.state = "UPLOADING";
                grp.statusSyncedWithServer = false;
                queryTaskStatus(uploadId);
            } else if (isRetryableError(code, status)) {
                grp.retryCount++;
                if (grp.retryCount < MAX_RETRIES) {
                    grp.nextRetryTimeMs = QDateTime::currentMSecsSinceEpoch() +
                                          calculateBackoffDelayMs(grp.retryCount);
                    grp.state = "UPLOADING";
                    if (status == 0) {
                        m_isNetworkAvailable = false;
                        emit networkStatusChanged(false);
                    }
                } else {
                    handleServerError(errJson, uploadId, status);
                }
            } else {
                handleServerError(errJson, uploadId, status);
            }
            saveTasksToStorage();
            emit taskListChanged();
        }
    });
}

void TransferManager::queryTaskStatus(const QString &uploadId)
{
    if (!m_tasks.contains(uploadId) || uploadId.startsWith("temp_")) return;

    TransferTaskGroup &grp = m_tasks[uploadId];
    if (grp.isSyncingStatus) return;
    grp.isSyncingStatus = true;

    QString urlStr = QString("%1/file/uploadfile/%2").arg(endpointFor(grp), uploadId);
    QUrl url(urlStr);
    QNetworkRequest request(url);
    applyRequestDefaults(request);

    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, uploadId]() {
        reply->deleteLater();
        if (!m_tasks.contains(uploadId)) return;

        TransferTaskGroup &grp = m_tasks[uploadId];
        grp.isSyncingStatus = false;
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (status == 200) {
            m_isNetworkAvailable = true;
            QJsonObject respJson = QJsonDocument::fromJson(reply->readAll()).object();
            QString responseError;
            if (!applyServerTaskResponse(grp, respJson, &responseError)) {
                grp.state = "FAILED";
                grp.pausedByUser = true;
                saveTasksToStorage();
                emit taskStatusChanged(uploadId, "FAILED", responseError);
                emit taskListChanged();
                return;
            }
            grp.statusSyncedWithServer = true;
            grp.retryCount = 0;
            grp.nextRetryTimeMs = 0;

            if (grp.state == "COMPLETED") {
                m_activeQueue.removeAll(uploadId);
            } else if (grp.state == "CANCELLED") {
                m_activeQueue.removeAll(uploadId);
                m_tasks.remove(uploadId);
            } else if (!grp.pausedByUser && !m_activeQueue.contains(uploadId)) {
                m_activeQueue.append(uploadId);
            }

            saveTasksToStorage();
            emit taskListChanged();
        } else {
            QJsonObject errJson = QJsonDocument::fromJson(reply->readAll()).object();
            const QString code = errJson.value("code").toString();
            if (isRetryableError(code, status)) {
                grp.retryCount++;
                if (grp.retryCount < MAX_RETRIES) {
                    grp.statusSyncedWithServer = false;
                    grp.nextRetryTimeMs = QDateTime::currentMSecsSinceEpoch() +
                                          calculateBackoffDelayMs(grp.retryCount);
                    if (status == 0) {
                        m_isNetworkAvailable = false;
                        emit networkStatusChanged(false);
                    }
                    saveTasksToStorage();
                    emit taskListChanged();
                } else {
                    handleServerError(errJson, uploadId, status);
                }
            } else {
                handleServerError(errJson, uploadId, status);
            }
        }
    });
}

void TransferManager::pauseTask(const QString &uploadId)
{
    if (m_tasks.contains(uploadId)) {
        m_tasks[uploadId].pausedByUser = true;
        m_activeQueue.removeAll(uploadId);
        saveTasksToStorage();
        emit taskListChanged();
    }
}

void TransferManager::resumeTask(const QString &uploadId)
{
    if (m_tasks.contains(uploadId)) {
        if (m_tasks[uploadId].state != "COMPLETED" && m_tasks[uploadId].state != "CANCELLED") {
            TransferTaskGroup &group = m_tasks[uploadId];
            QString sourceError;
            if (!validateLocalSources(group, &sourceError)) {
                group.state = "FAILED";
                group.pausedByUser = true;
                saveTasksToStorage();
                emit taskStatusChanged(uploadId, "FAILED", sourceError);
                emit taskListChanged();
                return;
            }
            group.pausedByUser = false;
            group.retryCount = 0;
            group.nextRetryTimeMs = 0;
            group.statusSyncedWithServer = group.uploadId.startsWith("temp_");
            if (!m_activeQueue.contains(uploadId)) {
                m_activeQueue.append(uploadId);
            }
            if (!group.uploadId.startsWith("temp_")) {
                queryTaskStatus(uploadId);
            }
            saveTasksToStorage();
            emit taskListChanged();
        }
    }
}

void TransferManager::cancelTask(const QString &uploadId)
{
    if (!m_tasks.contains(uploadId)) return;

    m_activeQueue.removeAll(uploadId);
    QString previousState = m_tasks[uploadId].state;
    m_tasks[uploadId].state = "CANCELLED";
    emit taskListChanged();

    if (uploadId.startsWith("temp_")) {
        m_tasks.remove(uploadId);
        saveTasksToStorage();
        emit taskListChanged();
        return;
    }

    QString urlStr = QString("%1/file/uploadfile/%2")
                         .arg(endpointFor(m_tasks[uploadId]), uploadId);
    QUrl url(urlStr);
    QNetworkRequest request(url);
    applyRequestDefaults(request);

    QNetworkReply *reply = m_netManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, uploadId, previousState]() {
        reply->deleteLater();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Remove locally ONLY after server confirmation (200, 204 or 404)
        if (status == 200 || status == 204 || status == 404) {
            m_tasks.remove(uploadId);
            saveTasksToStorage();
            emit taskListChanged();
        } else {
            qWarning() << "Server cancel task failed with status:" << status;
            if (m_tasks.contains(uploadId)) {
                m_tasks[uploadId].state = previousState;
                m_tasks[uploadId].pausedByUser = true;
                saveTasksToStorage();
                emit taskListChanged();
            }
        }
    });
}

void TransferManager::retryTask(const QString &uploadId)
{
    if (m_tasks.contains(uploadId)) {
        m_tasks[uploadId].retryCount = 0;
        for (auto &f : m_tasks[uploadId].files) {
            f.retryCount = 0;
            f.nextRetryTimeMs = 0;
            f.failureReason.clear();
        }
        resumeTask(uploadId);
    }
}

void TransferManager::clearCompletedTasks()
{
    QStringList toRemove;
    for (auto it = m_tasks.constBegin(); it != m_tasks.constEnd(); ++it) {
        if (it.value().state == "COMPLETED" || it.value().state == "CANCELLED") {
            toRemove.append(it.key());
        }
    }
    for (const QString &id : toRemove) {
        m_tasks.remove(id);
    }
    saveTasksToStorage();
    emit taskListChanged();
}

void TransferManager::handleServerError(const QJsonObject &errorJson, const QString &uploadId, int httpStatus, int fileIndex, int chunkIndex)
{
    QString code = errorJson.value("code").toString();
    QString msg = errorJson.value("message").toString();

    qWarning() << "Server error [" << httpStatus << code << "]:" << msg;

    if (!m_tasks.contains(uploadId)) return;
    TransferTaskGroup &grp = m_tasks[uploadId];

    if (code == "TASK_NOT_FOUND") {
        TransferTaskGroup grpCopy = m_tasks.take(uploadId);
        m_activeQueue.removeAll(uploadId);
        QString sourceError;
        if (!validateLocalSources(grpCopy, &sourceError)) {
            grpCopy.state = "FAILED";
            grpCopy.pausedByUser = true;
            m_tasks[uploadId] = grpCopy;
            emit taskStatusChanged(uploadId, "FAILED", sourceError);
            saveTasksToStorage();
            emit taskListChanged();
            return;
        }
        grpCopy.uploadId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        grpCopy.statusSyncedWithServer = true;
        grpCopy.isSyncingStatus = false;
        grpCopy.creationInFlight = false;
        grpCopy.completionInFlight = false;
        grpCopy.retryCount = 0;
        grpCopy.nextRetryTimeMs = 0;
        grpCopy.state = "PENDING";
        grpCopy.pausedByUser = false;
        for (TransferFileItem &file : grpCopy.files) {
            file.completionInFlight = false;
            file.state = "PENDING";
            file.completedChunks.clear();
            file.missingChunks.clear();
            for (int index = 0; index < file.totalChunks; ++index) {
                file.missingChunks.append(index);
            }
        }
        recalculateProgress(grpCopy);
        const QString replacementId = grpCopy.uploadId;
        m_tasks[replacementId] = grpCopy;
        m_activeQueue.append(replacementId);
        emit taskIdChanged(uploadId, replacementId);
        createServerTask(m_tasks[replacementId]);
        saveTasksToStorage();
        emit taskListChanged();
        return;
    }

    if ((code == "TASK_STATE_CONFLICT" || code == "FILE_NOT_FOUND") &&
        !uploadId.startsWith("temp_")) {
        grp.statusSyncedWithServer = false;
        grp.state = "UPLOADING";
        queryTaskStatus(uploadId);
        return;
    }

    if (code == "TARGET_ALREADY_EXISTS" || code == "INVALID_PATH" ||
        code == "INVALID_REQUEST" || code == "CHUNK_INDEX_INVALID" ||
        code == "CHUNK_RANGE_INVALID" || code == "CHUNK_SIZE_INVALID") {
        grp.state = "FAILED";
        grp.pausedByUser = true;
        emit taskStatusChanged(uploadId, "FAILED", msg);
        saveTasksToStorage();
        emit taskListChanged();
        return;
    }

    // For CHUNK_HASH_MISMATCH or network/5xx errors, do NOT immediately fail the task if retryCount < MAX_RETRIES
    if (isRetryableError(code, httpStatus)) {
        if (fileIndex >= 0 && fileIndex < grp.files.size()) {
            TransferFileItem &f = grp.files[fileIndex];
            if (f.retryCount < MAX_RETRIES) {
                // Keep task state in UPLOADING and allow retry after backoff
                return;
            } else {
                grp.state = "FAILED";
                grp.pausedByUser = true;
                f.state = "FAILED";
                emit taskStatusChanged(uploadId, "FAILED", msg);
            }
        } else {
            grp.state = "FAILED";
            grp.pausedByUser = true;
            emit taskStatusChanged(uploadId, "FAILED", msg.isEmpty() ? "Network retry limit reached" : msg);
        }
    } else {
        grp.state = "FAILED";
        grp.pausedByUser = true;
        emit taskStatusChanged(uploadId, "FAILED", msg);
    }

    saveTasksToStorage();
    emit taskListChanged();
}

void TransferManager::checkNetworkStatus()
{
    if (m_baseUrl.isEmpty() || m_authToken.isEmpty() || m_networkCheckInFlight) return;
    m_networkCheckInFlight = true;

    // Use a lightweight authenticated GET endpoint for reachability checks.
    // HEAD /file/uploadfile is not mapped by the v1.1.1 server and polluted
    // production logs with a 405 warning every ten seconds.
    QUrl url(m_baseUrl + "/api/v1/devices");
    QNetworkRequest request(url);
    applyRequestDefaults(request);

    QNetworkReply *reply = m_netManager->head(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_networkCheckInFlight = false;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool available = reply->error() == QNetworkReply::NoError || status > 0;

        if (m_isNetworkAvailable != available) {
            m_isNetworkAvailable = available;
            emit networkStatusChanged(m_isNetworkAvailable);
            if (m_isNetworkAvailable) {
                for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                    if (!it.value().pausedByUser && it.value().state != "COMPLETED" &&
                        it.value().state != "CANCELLED") {
                        it.value().statusSyncedWithServer = false;
                        it.value().retryCount = 0;
                        it.value().nextRetryTimeMs = 0;
                        if (!it.key().startsWith("temp_")) {
                            queryTaskStatus(it.key());
                        } else if (!m_activeQueue.contains(it.key())) {
                            m_activeQueue.append(it.key());
                        }
                    }
                }
            }
        }
    });
}

void TransferManager::updateSpeedStats()
{
    double mbps = (m_bytesUploadedInWindow * 8.0) / (1024.0 * 1024.0);
    m_globalSpeedMbps = mbps;
    m_bytesUploadedInWindow = 0;
    emit speedUpdated(m_globalSpeedMbps);
}

QString TransferManager::getStorageFilePath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("upload_tasks_v1.json");
}

void TransferManager::saveTasksToStorage()
{
    QJsonObject root;
    root["schemaVersion"] = 1;
    QJsonArray taskArr;

    for (auto it = m_tasks.constBegin(); it != m_tasks.constEnd(); ++it) {
        const TransferTaskGroup &grp = it.value();
        QJsonObject grpObj;
        grpObj["uploadId"] = grp.uploadId;
        grpObj["localRootPath"] = grp.localRootPath;
        grpObj["directoryName"] = grp.directoryName;
        grpObj["parentPath"] = grp.parentPath;
        grpObj["targetPath"] = grp.targetPath;
        grpObj["serverBaseUrl"] = grp.serverBaseUrl;
        grpObj["chunkSize"] = grp.chunkSize;
        grpObj["totalFiles"] = grp.totalFiles;
        grpObj["totalBytes"] = grp.totalBytes;
        grpObj["uploadedBytes"] = grp.uploadedBytes;
        grpObj["state"] = grp.state;
        grpObj["pausedByUser"] = grp.pausedByUser;

        QJsonArray filesArr;
        for (const TransferFileItem &f : grp.files) {
            QJsonObject fObj;
            fObj["path"] = f.path;
            fObj["size"] = f.size;
            fObj["sha256"] = f.sha256;
            fObj["totalChunks"] = f.totalChunks;
            fObj["state"] = f.state;
            fObj["uploadedBytes"] = f.uploadedBytes;
            fObj["failureReason"] = f.failureReason;

            QJsonArray missingArr;
            for (int m : f.missingChunks) missingArr.append(m);
            fObj["missingChunks"] = missingArr;

            QJsonArray completedArr;
            for (int c : f.completedChunks) completedArr.append(c);
            fObj["completedChunks"] = completedArr;

            filesArr.append(fObj);
        }
        grpObj["files"] = filesArr;
        taskArr.append(grpObj);
    }
    root["tasks"] = taskArr;

    QSaveFile file(getStorageFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        if (!file.commit()) {
            qWarning() << "Failed to atomically persist upload tasks:" << file.errorString();
        }
    }
}

void TransferManager::loadTasksFromStorage()
{
    QFile file(getStorageFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonObject root = QJsonDocument::fromJson(data).object();
    QJsonArray taskArr = root.value("tasks").toArray();

    for (const QJsonValue &val : taskArr) {
        QJsonObject grpObj = val.toObject();
        TransferTaskGroup grp;
        grp.uploadId = grpObj.value("uploadId").toString();
        grp.localRootPath = grpObj.value("localRootPath").toString();
        grp.directoryName = grpObj.value("directoryName").toString();
        grp.parentPath = grpObj.value("parentPath").toString();
        grp.targetPath = grpObj.value("targetPath").toString();
        if (grp.targetPath.isEmpty()) {
            // Tasks persisted by earlier clients used parentPath/directoryName.
            grp.targetPath = grp.parentPath + "/" + grp.directoryName;
        }
        grp.serverBaseUrl = grpObj.value("serverBaseUrl").toString(m_baseUrl);
        grp.chunkSize = grpObj.value("chunkSize").toVariant().toLongLong();
        grp.totalFiles = grpObj.value("totalFiles").toInt();
        grp.totalBytes = grpObj.value("totalBytes").toVariant().toLongLong();
        grp.uploadedBytes = grpObj.value("uploadedBytes").toVariant().toLongLong();
        grp.state = grpObj.value("state").toString();
        grp.pausedByUser = grpObj.value("pausedByUser").toBool(false);

        if (grp.state == "CANCELLED") {
            continue;
        }

        grp.statusSyncedWithServer = false;

        QJsonArray filesArr = grpObj.value("files").toArray();
        for (const QJsonValue &fVal : filesArr) {
            QJsonObject fObj = fVal.toObject();
            TransferFileItem f;
            f.path = fObj.value("path").toString();
            f.size = fObj.value("size").toVariant().toLongLong();
            f.sha256 = fObj.value("sha256").toString();
            f.totalChunks = fObj.value("totalChunks").toInt();
            f.state = fObj.value("state").toString();
            f.uploadedBytes = fObj.value("uploadedBytes").toVariant().toLongLong();
            f.failureReason = fObj.value("failureReason").toString();

            QJsonArray missingArr = fObj.value("missingChunks").toArray();
            for (const QJsonValue &mv : missingArr) f.missingChunks.append(mv.toInt());

            QJsonArray completedArr = fObj.value("completedChunks").toArray();
            for (const QJsonValue &cv : completedArr) f.completedChunks.insert(cv.toInt());

            grp.files.append(f);
        }

        recalculateProgress(grp);

        m_tasks[grp.uploadId] = grp;
        if (!grp.pausedByUser && grp.state != "COMPLETED") {
            m_activeQueue.append(grp.uploadId);
        }
    }

    emit taskListChanged();
}
