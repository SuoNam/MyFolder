#include "httphandler.h"
#include "serverconfig.h"
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

HttpHandler::HttpHandler(QObject *parent)
    : QObject{parent}
{
    manager=new QNetworkAccessManager(this);
    setBaseUrl(MyFolderServerConfig::baseUrl());
    connect(manager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401 && !reply->request().rawHeader("Authorization").isEmpty()) {
            emit authenticationRequired();
        }
    });
    // connect(manager,&QNetworkAccessManager::finished,this,&HttpHandler::onReplyFinished);
}

void HttpHandler::setBaseUrl(const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    if (!normalized.isEmpty() && normalized != m_baseUrl) {
        m_baseUrl = normalized;
        emit baseUrlChanged();
    }
}

void HttpHandler::login(const QString &account, const QString &password)
{
    post(m_baseUrl + "/user/login", {{"account", account}, {"password", password}, {"clientType", "DESKTOP"}}, Login);
}

void HttpHandler::signup(const QString &account, const QString &password, const QString &email, const QString &emailCode)
{
    post(m_baseUrl + "/user/signup", {{"account", account}, {"password", password}, {"email", email},
                                      {"emailCode", emailCode}, {"clientType", "DESKTOP"}}, SignUp);
}

void HttpHandler::sendEmailCode(const QString &email, const QString &purpose)
{
    post(m_baseUrl + "/user/email/code", {{"email", email}, {"purpose", purpose}}, EmailCode);
}

void HttpHandler::resetPassword(const QString &email, const QString &code, const QString &newPassword)
{
    post(m_baseUrl + "/user/password/reset", {{"email", email}, {"code", code}, {"newPassword", newPassword}}, PasswordReset);
}

void HttpHandler::refreshStoredSession()
{
    const QJsonObject stored = loadStoredSession();
    m_refreshToken = stored.value("refreshToken").toString();
    if (m_refreshToken.isEmpty()) {
        emit refreshResult(QJsonObject{{"status", 401}, {"message", tr("No stored session")}});
        return;
    }
    refreshSession();
}

void HttpHandler::refreshSession()
{
    if (m_refreshToken.isEmpty()) m_refreshToken = loadStoredSession().value("refreshToken").toString();
    if (m_refreshToken.isEmpty()) {
        emit refreshResult(QJsonObject{{"status", 401}, {"message", tr("No stored session")}});
        return;
    }
    if (m_refreshInFlight) return;

    setRefreshInFlight(true);
    // A connection that survived suspend/TUN route replacement can remain
    // ESTABLISHED in the kernel while never delivering another response.
    manager->clearConnectionCache();
    manager->clearAccessCache();
    refreshSessionAttempt(0);
}

void HttpHandler::refreshSessionAttempt(int attempt)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/user/refresh"));
    applyRequestDefaults(request, false);
    request.setTransferTimeout(20000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray payload = QJsonDocument(QJsonObject{
        {"refreshToken", m_refreshToken}, {"clientType", "DESKTOP"}
    }).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = manager->post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply, attempt]() {
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorText = reply->errorString();
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();

        if (isTransientNetworkFailure(httpStatus, networkError) && attempt < 2) {
            manager->clearConnectionCache();
            manager->clearAccessCache();
            QTimer::singleShot(retryDelayMs(attempt), this,
                               [this, attempt]() { refreshSessionAttempt(attempt + 1); });
            return;
        }

        if (json.isEmpty()) {
            json = {{"status", httpStatus}, {"message", networkErrorText}};
        }
        setRefreshInFlight(false);
        persistSessionResponse(json);
        emitSessionReady(json);
        emit refreshResult(json);
    });
}

void HttpHandler::setRefreshInFlight(bool inFlight)
{
    if (m_refreshInFlight == inFlight) return;
    m_refreshInFlight = inFlight;
    emit refreshInFlightChanged();
}

void HttpHandler::logout()
{
    const QString oldToken = m_refreshToken.isEmpty() ? loadStoredSession().value("refreshToken").toString() : m_refreshToken;
    if (!oldToken.isEmpty()) post(m_baseUrl + "/user/logout", {{"refreshToken", oldToken}}, Logout);
    m_refreshToken.clear();
    setRefreshInFlight(false);
    clearStoredSession();
    emit storedSessionChanged();
}

void HttpHandler::loadOAuthProviders()
{
    QNetworkRequest request(QUrl(m_baseUrl + "/user/oauth/providers"));
    applyRequestDefaults(request, false);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("route", static_cast<int>(OAuthProviders));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty()) json = {{"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()}, {"message", reply->errorString()}};
        reply->deleteLater();
        emit oauthProvidersResult(json);
    });
}

void HttpHandler::startOAuth(const QString &provider)
{
    post(m_baseUrl + "/user/oauth/" + QString::fromUtf8(QUrl::toPercentEncoding(provider)) + "/start",
         {{"clientType", "DESKTOP"}, {"returnUri", "myfolder://oauth/callback"}, {"link", false}}, OAuthStart);
}

void HttpHandler::exchangeOAuthCode(const QString &code)
{
    post(m_baseUrl + "/user/oauth/exchange", {{"code", code}, {"clientType", "DESKTOP"}}, OAuthExchange);
}

void HttpHandler::loadAccountProfile()
{
    QNetworkRequest request(QUrl(m_baseUrl + "/user/me"));
    applyRequestDefaults(request);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty()) json = {{"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()},
                                    {"message", reply->errorString()}};
        reply->deleteLater();
        emit accountProfileResult(json);
    });
}

void HttpHandler::updateDisplayName(const QString &displayName)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/user/me/display-name"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray body = QJsonDocument(QJsonObject{{"displayName", displayName.trimmed()}})
            .toJson(QJsonDocument::Compact);
    QNetworkReply *reply = manager->sendCustomRequest(request, "PATCH", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty()) json = {{"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()},
                                    {"message", reply->errorString()}};
        reply->deleteLater();
        emit displayNameUpdateResult(json);
    });
}

void HttpHandler::loadStorageUsage()
{
    QNetworkRequest request(QUrl(m_baseUrl + "/user/me/storage")); applyRequestDefaults(request);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty()) json = {{"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()}, {"message", reply->errorString()}};
        reply->deleteLater(); emit storageUsageResult(json);
    });
}

void HttpHandler::loadGroups()
{
    loadGroupsAttempt(0);
}

void HttpHandler::loadUploadHistory()
{
    // The v1 endpoint is always authenticated.  The legacy /file route also
    // accepts uploads, but file read routes intentionally have different
    // authentication rules and are therefore the wrong home for history.
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/transfers/tasks"));
    applyRequestDefaults(request);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        const QJsonDocument document = QJsonDocument::fromJson(payload);
        if (status == 200 && document.isArray()) {
            emit uploadHistoryResult(document.array().toVariantList(), QString());
            return;
        }
        const QJsonObject error = document.object();
        emit uploadHistoryResult({}, error.value("message").toString(
                                      networkError.isEmpty() ? tr("加载上传记录失败") : networkError));
    });
}

void HttpHandler::updateTransferPreferences(bool autoAcceptDeviceTransfers)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/user/me/transfer-preferences"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray body = QJsonDocument(QJsonObject{
        {"autoAcceptDeviceTransfers", autoAcceptDeviceTransfers}}).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = manager->sendCustomRequest(request, "PATCH", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty()) json = {{"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()},
                                   {"message", reply->errorString()}};
        reply->deleteLater();
        emit transferPreferencesUpdateResult(json);
    });
}

void HttpHandler::loadGroupsAttempt(int attempt)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups")); applyRequestDefaults(request);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, attempt]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorText = reply->errorString();
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();
        if (isTransientNetworkFailure(status, networkError)) {
            manager->clearConnectionCache();
            if (attempt < 3) {
                QTimer::singleShot(retryDelayMs(attempt), this,
                                   [this, attempt]() { loadGroupsAttempt(attempt + 1); });
                return;
            }
            json = {{"status", 0}, {"message", tr("网络连接暂时中断，请稍后刷新")}};
        } else if (json.isEmpty()) {
            json = {{"status", status}, {"message", networkErrorText}};
        }
        emit groupsResult(json);
    });
}

void HttpHandler::createGroup(const QString &name)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups")); applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, QJsonDocument(QJsonObject{{"name", name.trimmed()}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { reply->deleteLater(); loadGroups(); });
}

void HttpHandler::addGroupMember(const QString &groupId, const QString &email, const QString &permission)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups/" + QString::fromUtf8(QUrl::toPercentEncoding(groupId)) + "/members"));
    applyRequestDefaults(request); request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, QJsonDocument(QJsonObject{{"email", email.trimmed()}, {"permission", permission}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (status >= 200 && status < 300 && json.value("status").toInt(status) == 200) loadGroups();
        else emit groupsResult(json.isEmpty() ? QJsonObject{{"status", status}, {"message", tr("添加成员失败")}} : json);
    });
}

void HttpHandler::leaveGroup(const QString &groupId)
{
    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(groupId));
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups/" + encoded + "/leave"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (status >= 200 && status < 300 && json.value("status").toInt(status) == 200) loadGroups();
        else emit groupsResult(json.isEmpty() ? QJsonObject{{"status", status}, {"message", tr("退出群组失败")}} : json);
    });
}

void HttpHandler::applyRequestDefaults(QNetworkRequest &request, bool authenticated) const
{
    request.setTransferTimeout(30000);
    if (authenticated && !m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", m_authToken.toUtf8());
    }
}

void HttpHandler::applyStorageScope(QNetworkRequest &request, const QString &scopeType, const QString &scopeId)
{
    const QString normalizedType = scopeType.trimmed().toUpper();
    request.setRawHeader("X-Storage-Scope", (normalizedType == "GROUP" ? "GROUP" : "PRIVATE"));
    if (normalizedType == "GROUP" && !scopeId.trimmed().isEmpty()) {
        request.setRawHeader("X-Storage-Scope-Id", scopeId.trimmed().toUtf8());
    }
}

void HttpHandler::get(QString url){
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    applyRequestDefaults(request);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void HttpHandler::listServerDirectory(const QString &directoryPath)
{
    listScopedDirectory(directoryPath, "PRIVATE", QString());
}

void HttpHandler::listScopedDirectory(const QString &directoryPath, const QString &scopeType, const QString &scopeId)
{
    listScopedDirectoryAttempt(directoryPath, scopeType, scopeId, 0);
}

void HttpHandler::listScopedDirectoryAttempt(const QString &directoryPath, const QString &scopeType,
                                             const QString &scopeId, int attempt)
{
    const QString normalizedType = scopeType.trimmed().toUpper() == "GROUP" ? "GROUP" : "PRIVATE";
    const QString normalizedId = normalizedType == "GROUP" ? scopeId.trimmed() : QString();
    QNetworkRequest request(QUrl(m_baseUrl + "/file/getfilelist"));
    applyRequestDefaults(request);
    applyStorageScope(request, normalizedType, normalizedId);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    const QJsonObject body{{"directoryPath", directoryPath}};
    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, directoryPath, normalizedType, normalizedId, attempt]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QNetworkReply::NetworkError networkErrorCode = reply->error();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        if (isTransientNetworkFailure(status, networkErrorCode)) {
            manager->clearConnectionCache();
            if (attempt < 3) {
                QTimer::singleShot(retryDelayMs(attempt), this,
                                   [this, directoryPath, normalizedType, normalizedId, attempt]() {
                    listScopedDirectoryAttempt(directoryPath, normalizedType, normalizedId, attempt + 1);
                });
                return;
            }
        }
        const QJsonObject response = QJsonDocument::fromJson(payload).object();
        QVariantList entries;
        QString error;
        if (status == 200 && response.value("data").isArray()) {
            entries = response.value("data").toArray().toVariantList();
        } else {
            error = isTransientNetworkFailure(status, networkErrorCode)
                ? tr("网络连接暂时中断，请稍后刷新")
                : response.value("message").toString(networkError);
        }
        emit scopedDirectoryListed(normalizedType, normalizedId, directoryPath, entries, error);
        if (normalizedType == "PRIVATE") {
            emit serverDirectoryListed(directoryPath, entries, error);
        }
    });
}

bool HttpHandler::isTransientNetworkFailure(int status, QNetworkReply::NetworkError error)
{
    return status == 0 && error != QNetworkReply::NoError &&
           error != QNetworkReply::OperationCanceledError;
}

int HttpHandler::retryDelayMs(int attempt)
{
    static constexpr int delays[] = {500, 1500, 3000};
    return delays[qBound(0, attempt, 2)];
}
void HttpHandler::postScopedOperation(const QString &endpoint, const QString &operation,
                                        const QJsonValue &body, const QString &scopeType,
                                        const QString &scopeId)
{
    const QString normalizedType = scopeType.trimmed().toUpper() == "GROUP" ? "GROUP" : "PRIVATE";
    const QString normalizedId = normalizedType == "GROUP" ? scopeId.trimmed() : QString();
    QNetworkRequest request(QUrl(m_baseUrl + endpoint));
    applyRequestDefaults(request);
    applyStorageScope(request, normalizedType, normalizedId);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QByteArray payload;
    if (body.isArray()) payload = QJsonDocument(body.toArray()).toJson(QJsonDocument::Compact);
    else payload = QJsonDocument(body.toObject()).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = manager->post(request, payload);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, normalizedType, normalizedId, operation]() {
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();

        const QJsonObject response = QJsonDocument::fromJson(payload).object();
        const int businessStatus = response.value("status").toInt(httpStatus);
        const bool success = httpStatus >= 200 && httpStatus < 300 &&
                             (businessStatus == 0 || (businessStatus >= 200 && businessStatus < 300));
        QString message = response.value("message").toString().trimmed();
        if (message.isEmpty()) {
            message = success ? tr("操作完成") : networkError;
        }
        if (!success && message.isEmpty()) {
            message = tr("操作失败（HTTP %1）").arg(httpStatus);
        }

        QVariantList results;
        if (response.value("data").isArray()) {
            results = response.value("data").toArray().toVariantList();
        }
        emit scopedOperationFinished(normalizedType, normalizedId, operation, success, message, results);
    });
}

void HttpHandler::createScopedFolder(const QString &path, const QString &scopeType, const QString &scopeId)
{
    postScopedOperation("/file/createfolder", "createFolder", QJsonObject{{"path", path}}, scopeType, scopeId);
}

void HttpHandler::moveScopedPaths(const QVariantList &operations, const QString &scopeType, const QString &scopeId)
{
    QJsonArray body;
    for (const QVariant &value : operations) {
        const QVariantMap operation = value.toMap();
        const QString from = operation.value("targetPath", operation.value("from")).toString();
        const QString to = operation.value("newPath", operation.value("to")).toString();
        if (!from.isEmpty() && !to.isEmpty()) body.append(QJsonObject{{"targetPath", from}, {"newPath", to}});
    }
    postScopedOperation("/file/move", "move", body, scopeType, scopeId);
}

void HttpHandler::copyScopedPaths(const QVariantList &operations, const QString &scopeType, const QString &scopeId)
{
    QJsonArray body;
    for (const QVariant &value : operations) {
        const QVariantMap operation = value.toMap();
        const QString from = operation.value("targetPath", operation.value("from")).toString();
        const QString to = operation.value("newPath", operation.value("to")).toString();
        if (!from.isEmpty() && !to.isEmpty()) body.append(QJsonObject{{"targetPath", from}, {"newPath", to}});
    }
    postScopedOperation("/file/copy", "copy", body, scopeType, scopeId);
}

void HttpHandler::transferPrivatePathsToGroup(const QString &mode, const QVariantList &sourcePaths,
                                              const QString &groupId, const QString &targetDirectory)
{
    QJsonArray paths;
    for (const QVariant &value : sourcePaths) {
        const QString path = value.toString().trimmed();
        if (!path.isEmpty()) paths.append(path);
    }
    const bool copy = mode.trimmed().toLower() == "copy";
    const QJsonObject body{{"groupId", groupId.trimmed()},
                           {"targetDirectory", targetDirectory},
                           {"sourcePaths", paths}};
    postScopedOperation(copy ? "/file/copy-to-group" : "/file/move-to-group",
                        copy ? "copyToGroup" : "moveToGroup", body, "PRIVATE", QString());
}

void HttpHandler::deleteScopedPaths(const QVariantList &paths, const QString &scopeType, const QString &scopeId)
{
    QJsonArray body;
    for (const QVariant &value : paths) {
        const QString path = value.typeId() == QMetaType::QVariantMap
                                 ? value.toMap().value("deletePath", value.toMap().value("path")).toString()
                                 : value.toString();
        if (!path.isEmpty()) body.append(QJsonObject{{"deletePath", path}});
    }
    postScopedOperation("/file/delete", "delete", body, scopeType, scopeId);
}

void HttpHandler::downloadScoped(const QString &serverFilePath, const QString &saveDirectory,
                                 const QString &scopeType, const QString &scopeId)
{
    const QString normalizedType = scopeType.trimmed().toUpper() == "GROUP" ? "GROUP" : "PRIVATE";
    const QString normalizedId = normalizedType == "GROUP" ? scopeId.trimmed() : QString();
    QUrl url(m_baseUrl + "/file/downloadfile");
    QUrlQuery query;
    query.addQueryItem("filePathS", serverFilePath);
    url.setQuery(query);

    QNetworkRequest request(url);
    applyRequestDefaults(request);
    applyStorageScope(request, normalizedType, normalizedId);

    QString cleanSaveDir = saveDirectory;
    const QUrl saveDirectoryUrl(saveDirectory);
    if (saveDirectoryUrl.isLocalFile()) cleanSaveDir = saveDirectoryUrl.toLocalFile();
    if (cleanSaveDir.isEmpty() || !QDir().mkpath(cleanSaveDir)) {
        emit scopedDownloadFinished(normalizedType, normalizedId, false,
                                    tr("无法创建下载目录：%1").arg(cleanSaveDir), QString());
        return;
    }

    const QString fileName = serverFilePath.section('/', -1);
    const QString localFilePath = QDir(cleanSaveDir).filePath(fileName);
    QFile *file = new QFile(localFilePath);
    if (!file->open(QIODevice::WriteOnly)) {
        emit scopedDownloadFinished(normalizedType, normalizedId, false,
                                    tr("无法创建本地文件：%1").arg(localFilePath), QString());
        delete file;
        return;
    }

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        if (file->isOpen()) file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) emit downloadProgress(received, total);
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, localFilePath, normalizedType, normalizedId]() {
        file->close();
        if (reply->error() == QNetworkReply::NoError) {
            emit scopedDownloadFinished(normalizedType, normalizedId, true,
                                        tr("下载完成"), localFilePath);
        } else {
            file->remove();
            emit scopedDownloadFinished(normalizedType, normalizedId, false,
                                        tr("下载失败：%1").arg(reply->errorString()), QString());
        }
        file->deleteLater();
        reply->deleteLater();
    });
}

void HttpHandler::downloadScopedDirectory(const QString &serverDirectoryPath, const QString &saveDirectory,
                                          const QString &scopeType, const QString &scopeId)
{
    const QString normalizedType = scopeType.trimmed().toUpper() == "GROUP" ? "GROUP" : "PRIVATE";
    const QString normalizedId = normalizedType == "GROUP" ? scopeId.trimmed() : QString();
    QUrl url(m_baseUrl + "/directory/downloaddirectory");
    QUrlQuery query;
    query.addQueryItem("directoryPathS", serverDirectoryPath);
    url.setQuery(query);

    QNetworkRequest request(url);
    applyRequestDefaults(request);
    applyStorageScope(request, normalizedType, normalizedId);

    QString cleanSaveDir = saveDirectory;
    const QUrl saveDirectoryUrl(saveDirectory);
    if (saveDirectoryUrl.isLocalFile()) cleanSaveDir = saveDirectoryUrl.toLocalFile();
    if (cleanSaveDir.isEmpty() || !QDir().mkpath(cleanSaveDir)) {
        emit scopedDownloadFinished(normalizedType, normalizedId, false,
                                    tr("无法创建下载目录：%1").arg(cleanSaveDir), QString());
        return;
    }

    QString baseName = serverDirectoryPath.section('/', -1).trimmed();
    if (baseName.isEmpty()) baseName = QStringLiteral("files");
    const QString localFilePath = QDir(cleanSaveDir).filePath(baseName + QStringLiteral(".zip"));
    QFile *file = new QFile(localFilePath);
    if (!file->open(QIODevice::WriteOnly)) {
        emit scopedDownloadFinished(normalizedType, normalizedId, false,
                                    tr("无法创建本地文件：%1").arg(localFilePath), QString());
        delete file;
        return;
    }

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        if (file->isOpen()) file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) emit downloadProgress(received, total);
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, localFilePath, normalizedType, normalizedId]() {
        file->close();
        if (reply->error() == QNetworkReply::NoError) {
            emit scopedDownloadFinished(normalizedType, normalizedId, true,
                                        tr("目录下载完成"), localFilePath);
        } else {
            file->remove();
            emit scopedDownloadFinished(normalizedType, normalizedId, false,
                                        tr("目录下载失败：%1").arg(reply->errorString()), QString());
        }
        file->deleteLater();
        reply->deleteLater();
    });
}

void HttpHandler::download(const QString &baseUrl, const QString &serverFilePath, const QString &saveDirectory, const QString &token) {
    // 1. 拼接完整的 URL 和 Query 参数
    QUrl url(baseUrl + "/file/downloadfile");
    QUrlQuery query;
    query.addQueryItem("filePathS", serverFilePath);
    url.setQuery(query);
    QNetworkRequest request(url);
    applyRequestDefaults(request);
    // 2. 携带 JWT Token (如果系统处于登录模式)
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", token.toUtf8());
    }
    // 3. 准备本地文件路径
    // 从服务器路径中截取文件名，例如 "documents/report.pdf" -> "report.pdf"
    QString fileName = serverFilePath.section('/', -1);

    // 处理 QML 传过来的路径 (可能是 file:/// 开头)
    QString cleanSaveDir = saveDirectory;
    const QUrl saveDirectoryUrl(saveDirectory);
    if (saveDirectoryUrl.isLocalFile()) {
        cleanSaveDir = saveDirectoryUrl.toLocalFile();
    }
    if (cleanSaveDir.isEmpty() || !QDir().mkpath(cleanSaveDir)) {
        emit downloadFinished(false, tr("无法创建下载目录: %1").arg(cleanSaveDir), "");
        return;
    }
    QString localFilePath = QDir(cleanSaveDir).filePath(fileName);
    QFile *file = new QFile(localFilePath);
    if (!file->open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "无法在本地创建文件: " + localFilePath, "");
        delete file;
        return;
    }
    // 4. 发起 GET 请求
    QNetworkReply *reply = manager->get(request);
    // 5. 绑定信号槽：边下载边写入文件（防止大文件占用过多内存）
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        if (file->isOpen()) {
            file->write(reply->readAll());
        }
    });
    // 6. 绑定信号槽：更新进度条
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            emit downloadProgress(bytesReceived, bytesTotal);
        }
    });
    // 7. 绑定信号槽：下载完成后的清理工作
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, localFilePath]() {
        file->close();
        if (reply->error() == QNetworkReply::NoError) {
            emit downloadFinished(true, "下载成功", localFilePath);
        } else {
            // 下载失败，删除残缺的本地文件
            file->remove();
            emit downloadFinished(false, "下载失败: " + reply->errorString(), "");
        }
        file->deleteLater();
        reply->deleteLater();
    });
}
void HttpHandler::post(const QString& url,const QJsonObject& data,RequestType route){
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    const bool publicRoute = route == Login || route == SignUp || route == EmailCode || route == PasswordReset
            || route == RefreshSession || route == OAuthStart || route == OAuthExchange || route == Logout;
    applyRequestDefaults(request, !publicRoute);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonDocument doc(data);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    QNetworkReply* reply =manager->post(request,postData);
    reply->setProperty("route", static_cast<int>(route));
    connect(reply,&QNetworkReply::finished,this,[this,reply](){
        reply->deleteLater();
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty() && reply->error() != QNetworkReply::NoError) {
            json["status"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            json["message"] = reply->errorString();
        }
        int route = reply->property("route").toInt();
        if (route == Login) { persistSessionResponse(json); emitSessionReady(json); emit loginResult(json); }
        else if (route == GetFileList)  emit fileListResult(json);
        else if(route == SignUp) { persistSessionResponse(json); emitSessionReady(json); emit signupResult(json); }
        else if(route == EmailCode) emit emailCodeResult(json);
        else if(route == PasswordReset) emit passwordResetResult(json);
        else if(route == RefreshSession) { persistSessionResponse(json); emitSessionReady(json); emit refreshResult(json); }
        else if(route == OAuthStart) {
            const QString authorizationUrl = json.value("data").toObject().value("authorizationUrl").toString();
            if (json.value("status").toInt() == 200) {
                if (authorizationUrl.isEmpty()) {
                    json["status"] = 500;
                    json["message"] = tr("服务端未返回第三方授权地址");
                } else if (!QDesktopServices::openUrl(QUrl(authorizationUrl))) {
                    json["status"] = 500;
                    json["message"] = tr("无法打开系统浏览器，请检查默认浏览器设置");
                }
            }
            emit oauthStartResult(json);
        }
        else if(route == OAuthExchange) { persistSessionResponse(json); emitSessionReady(json); emit oauthExchangeResult(json); }
    });
}

void HttpHandler::emitSessionReady(const QJsonObject &response)
{
    if (response.value("status").toInt() != 200 || !response.value("data").isObject()) return;
    const QJsonObject data = response.value("data").toObject();
    QString accessToken = data.value("accessToken").toString();
    if (accessToken.isEmpty()) accessToken = data.value("token").toString();
    const QString account = data.value("account").toString();
    if (!accessToken.isEmpty() && !account.isEmpty()) emit sessionReady(accessToken, account);
}

bool HttpHandler::hasStoredSession() const
{
    return !loadStoredSession().value("refreshToken").toString().isEmpty();
}

QString HttpHandler::sessionFilePath() const
{
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!directory.exists()) directory.mkpath(".");
    return directory.filePath("auth-session.bin");
}

QJsonObject HttpHandler::loadStoredSession() const
{
    QFile file(sessionFilePath());
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QByteArray plain = unprotect(file.readAll());
    const QJsonDocument document = QJsonDocument::fromJson(plain);
    return document.isObject() ? document.object() : QJsonObject();
}

bool HttpHandler::saveStoredSession(const QJsonObject &session) const
{
    const QByteArray cipher = protect(QJsonDocument(session).toJson(QJsonDocument::Compact));
    if (cipher.isEmpty()) return false;
    QSaveFile file(sessionFilePath());
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return file.write(cipher) == cipher.size() && file.commit();
}

void HttpHandler::clearStoredSession() const
{
    QFile::remove(sessionFilePath());
}

QByteArray HttpHandler::protect(const QByteArray &plain) const
{
#ifdef Q_OS_WIN
    DATA_BLOB input{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE *>(const_cast<char *>(plain.constData()))};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"MyFolder desktop session", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
    QByteArray result(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return result;
#else
    return plain;
#endif
}

QByteArray HttpHandler::unprotect(const QByteArray &cipher) const
{
#ifdef Q_OS_WIN
    DATA_BLOB input{static_cast<DWORD>(cipher.size()), reinterpret_cast<BYTE *>(const_cast<char *>(cipher.constData()))};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
    QByteArray result(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return result;
#else
    return cipher;
#endif
}

void HttpHandler::persistSessionResponse(const QJsonObject &response)
{
    if (response.value("status").toInt() != 200 || !response.value("data").isObject()) return;
    const QJsonObject data = response.value("data").toObject();
    const QString refresh = data.value("refreshToken").toString();
    if (refresh.isEmpty()) return;
    m_refreshToken = refresh;
    const int expiresIn = data.value("expiresIn").toInt();
    if (expiresIn > 0 && expiresIn != m_accessTokenExpiresIn) {
        m_accessTokenExpiresIn = expiresIn;
        emit accessTokenExpiresInChanged();
    }
    saveStoredSession(QJsonObject{{"refreshToken", refresh}, {"account", data.value("account")},
                                  {"email", data.value("email")}, {"refreshExpiresAt", data.value("refreshExpiresAt")}});
    emit storedSessionChanged();
}
// void HttpHandler::onReplyFinished(QNetworkReply *reply){
//     QString result;
//     if(reply->error()!=QNetworkReply::NoError){
//     }
//     QByteArray responseData = reply->readAll();
//     QJsonParseError jsonError;
//     QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);

//     if (jsonDoc.isObject()) {
//         QJsonObject jsonObj = jsonDoc.object();
//         QVariantMap dataMap = jsonObj.toVariantMap();

//         emit objectReady(dataMap);
//     }else if (jsonDoc.isArray()){
//             QJsonArray jsonArray = jsonDoc.array();
//             QVariantList dataList = jsonArray.toVariantList();
//             emit dataListReady(dataList);

//     }
//     reply->deleteLater();
// }
