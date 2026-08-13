#ifndef RELAYDOWNLOADMANAGER_H
#define RELAYDOWNLOADMANAGER_H

#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

struct RelayDownloadFile
{
    QString path;
    qint64 size = 0;
    QString sha256;
    QString finalPath;
    QString partPath;
    bool completed = false;
    int recoveryCount = 0;
};

struct RelayDownloadTask
{
    QString forwardId;
    QString destinationPath;
    qint64 totalBytes = 0;
    qint64 verifiedBytes = 0;
    QString state;
    QString error;
    QList<RelayDownloadFile> files;
    int currentFileIndex = 0;
    int retryCount = 0;
    bool completionEmitted = false;
    int finalizationRetryCount = 0;
};

class RelayDownloadManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId WRITE setCurrentDeviceId NOTIFY currentDeviceIdChanged)
    Q_PROPERTY(QString currentDeviceToken READ currentDeviceToken WRITE setCurrentDeviceToken NOTIFY currentDeviceTokenChanged)
    Q_PROPERTY(QString receiveRoot READ receiveRoot WRITE setReceiveRoot NOTIFY receiveRootChanged)
    Q_PROPERTY(QVariantList downloads READ downloads NOTIFY downloadsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit RelayDownloadManager(QObject *parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }
    QString authToken() const { return m_authToken; }
    QString currentDeviceId() const { return m_currentDeviceId; }
    QString currentDeviceToken() const { return m_currentDeviceToken; }
    QString receiveRoot() const { return m_receiveRoot; }
    QVariantList downloads() const;
    bool busy() const { return m_reply != nullptr || !m_activeForwardId.isEmpty(); }
    QString lastError() const { return m_lastError; }

    void setBaseUrl(const QString &baseUrl);
    void setAuthToken(const QString &authToken);
    void setCurrentDeviceId(const QString &deviceId);
    void setCurrentDeviceToken(const QString &deviceToken);
    void setReceiveRoot(const QString &receiveRoot);

    Q_INVOKABLE bool startTask(const QVariantMap &task);
    Q_INVOKABLE void cancelTask(const QString &forwardId);
    Q_INVOKABLE void retryTask(const QString &forwardId);
    Q_INVOKABLE void confirmTaskCompleted(const QString &forwardId);
    Q_INVOKABLE void retryFinalization(const QString &forwardId);
    Q_INVOKABLE void handleFinalizationFailure(const QString &forwardId, const QString &code);

    static bool isValidRelativePath(const QString &path);
    static QString calculateFileSha256(const QString &path);

signals:
    void baseUrlChanged();
    void authTokenChanged();
    void currentDeviceIdChanged();
    void currentDeviceTokenChanged();
    void receiveRootChanged();
    void downloadsChanged();
    void busyChanged();
    void lastErrorChanged();
    void progressReady(const QString &forwardId, qint64 verifiedBytes);
    void taskDownloadCompleted(const QString &forwardId, qint64 totalBytes);
    void taskDownloadFailed(const QString &forwardId, const QString &code,
                            const QString &message);

private:
    void processQueue();
    void processCurrentFile();
    void beginRangeDownload(RelayDownloadTask &task, RelayDownloadFile &file);
    void finishCurrentFile();
    void finishTask();
    void failTask(const QString &code, const QString &message);
    void scheduleNetworkRetry();
    bool prepareTargetPaths(RelayDownloadTask &task, QString *error);
    bool safeTargetPaths(const QString &destinationPath, const QString &filePath,
                         QString *finalPath, QString *partPath, QString *error) const;
    bool archivePartFile(const QString &partPath) const;
    void setLastError(const QString &error);
    void applyRequestDefaults(QNetworkRequest &request) const;
    QString storageFilePath() const;
    void saveTasks();
    void loadTasks();
    static QJsonObject parseError(const QByteArray &payload);
    static qint64 retryDelayMs(int retryCount);

    QNetworkAccessManager m_network;
    QMap<QString, RelayDownloadTask> m_tasks;
    QStringList m_queue;
    QNetworkReply *m_reply = nullptr;
    QString m_activeForwardId;
    QString m_baseUrl;
    QString m_authToken;
    QString m_currentDeviceId;
    QString m_currentDeviceToken;
    QString m_receiveRoot;
    QString m_lastError;
};

#endif
