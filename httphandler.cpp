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
    if (m_refreshToken.isEmpty()) return;
    post(m_baseUrl + "/user/refresh", {{"refreshToken", m_refreshToken}, {"clientType", "DESKTOP"}}, RefreshSession);
}

void HttpHandler::logout()
{
    const QString oldToken = m_refreshToken.isEmpty() ? loadStoredSession().value("refreshToken").toString() : m_refreshToken;
    if (!oldToken.isEmpty()) post(m_baseUrl + "/user/logout", {{"refreshToken", oldToken}}, Logout);
    m_refreshToken.clear();
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
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups")); applyRequestDefaults(request);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.isEmpty()) json = {{"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()}, {"message", reply->errorString()}};
        reply->deleteLater(); emit groupsResult(json);
    });
}

void HttpHandler::createGroup(const QString &name)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups")); applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, QJsonDocument(QJsonObject{{"name", name.trimmed()}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { reply->deleteLater(); loadGroups(); });
}

void HttpHandler::addGroupMember(const QString &groupId, const QString &account, const QString &permission)
{
    QNetworkRequest request(QUrl(m_baseUrl + "/api/v1/groups/" + QString::fromUtf8(QUrl::toPercentEncoding(groupId)) + "/members"));
    applyRequestDefaults(request); request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, QJsonDocument(QJsonObject{{"account", account.trimmed()}, {"permission", permission}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { reply->deleteLater(); loadGroups(); });
}

void HttpHandler::applyRequestDefaults(QNetworkRequest &request, bool authenticated) const
{
    request.setTransferTimeout(30000);
    if (authenticated && !m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", m_authToken.toUtf8());
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
    QNetworkRequest request(QUrl(m_baseUrl + "/file/getfilelist"));
    applyRequestDefaults(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    const QJsonObject body{{"directoryPath", directoryPath}};
    QNetworkReply *reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, directoryPath]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->errorString();
        reply->deleteLater();
        const QJsonObject response = QJsonDocument::fromJson(payload).object();
        if (status == 200 && response.value("data").isArray()) {
            emit serverDirectoryListed(directoryPath, response.value("data").toArray().toVariantList(), QString());
        } else {
            emit serverDirectoryListed(directoryPath, {}, response.value("message").toString(networkError));
        }
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
            if (!authorizationUrl.isEmpty()) QDesktopServices::openUrl(QUrl(authorizationUrl));
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
