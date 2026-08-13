#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H
#include <QDebug>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QtQml/qqmlregistration.h>
#include <QFile>
#include <QFileInfo>
#include <QUrlQuery>
#include <QDir>
#include <QTimer>
class HttpHandler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(bool hasStoredSession READ hasStoredSession NOTIFY storedSessionChanged)
public:
    enum RequestType {
        Login,
        SignUp,
        EmailCode,
        PasswordReset,
        RefreshSession,
        OAuthProviders,
        OAuthStart,
        OAuthExchange,
        Logout,
        GetFileList,
        DownloadFile
    };
    Q_ENUM(RequestType)
    explicit HttpHandler(QObject *parent = nullptr);
    QString authToken() const { return m_authToken; }
    QString baseUrl() const { return m_baseUrl; }
    bool hasStoredSession() const;
    void setAuthToken(const QString &token) {
        if (m_authToken != token) {
            m_authToken = token;
            emit authTokenChanged();
        }
    }
    void setBaseUrl(const QString &baseUrl);
    Q_INVOKABLE void login(const QString &account, const QString &password);
    Q_INVOKABLE void signup(const QString &account, const QString &password, const QString &email, const QString &emailCode);
    Q_INVOKABLE void sendEmailCode(const QString &email, const QString &purpose);
    Q_INVOKABLE void resetPassword(const QString &email, const QString &code, const QString &newPassword);
    Q_INVOKABLE void refreshStoredSession();
    Q_INVOKABLE void refreshSession();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void loadOAuthProviders();
    Q_INVOKABLE void startOAuth(const QString &provider);
    Q_INVOKABLE void exchangeOAuthCode(const QString &code);
    Q_INVOKABLE void loadAccountProfile();
    Q_INVOKABLE void updateDisplayName(const QString &displayName);
    Q_INVOKABLE void loadStorageUsage();
    Q_INVOKABLE void loadGroups();
    Q_INVOKABLE void createGroup(const QString &name);
    Q_INVOKABLE void addGroupMember(const QString &groupId, const QString &account, const QString &permission);
    Q_INVOKABLE void get(QString url);
    Q_INVOKABLE void post(const QString& url,const QJsonObject& data,RequestType route);
    Q_INVOKABLE void download(const QString &baseUrl, const QString &serverFilePath, const QString &saveDirectory, const QString &token);
    Q_INVOKABLE void listServerDirectory(const QString &directoryPath);
signals:
    void responseReady(const QString &result);
    void dataListReady(const QVariantList &dataList);
    void objectReady(const QVariantMap &dataObj);
    void loginResult(const QJsonObject &dataObj);
    void fileListResult(const QJsonObject &dataObj);
    void signupResult(const QJsonObject &dataObj);
    void emailCodeResult(const QJsonObject &dataObj);
    void passwordResetResult(const QJsonObject &dataObj);
    void refreshResult(const QJsonObject &dataObj);
    void oauthProvidersResult(const QJsonObject &dataObj);
    void oauthStartResult(const QJsonObject &dataObj);
    void oauthExchangeResult(const QJsonObject &dataObj);
    void sessionReady(const QString &accessToken, const QString &account);
    void accountProfileResult(const QJsonObject &dataObj);
    void displayNameUpdateResult(const QJsonObject &dataObj);
    void storageUsageResult(const QJsonObject &dataObj);
    void groupsResult(const QJsonObject &dataObj);
    void authTokenChanged();
    void baseUrlChanged();
    void storedSessionChanged();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString &message, const QString &savedPath);
    void serverDirectoryListed(const QString &directoryPath, const QVariantList &entries, const QString &error);
private slots:
    // void onReplyFinished(QNetworkReply *reply);
private:
    void applyRequestDefaults(QNetworkRequest &request, bool authenticated = true) const;
    QString sessionFilePath() const;
    QJsonObject loadStoredSession() const;
    bool saveStoredSession(const QJsonObject &session) const;
    void clearStoredSession() const;
    QByteArray protect(const QByteArray &plain) const;
    QByteArray unprotect(const QByteArray &cipher) const;
    void persistSessionResponse(const QJsonObject &response);
    void emitSessionReady(const QJsonObject &response);
    QNetworkAccessManager * manager;
    QString m_authToken;
    QString m_baseUrl;
    QString m_refreshToken;


};

#endif // HTTPHANDLER_H
