#ifndef FORWARDMANAGER_H
#define FORWARDMANAGER_H

#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class ForwardManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId WRITE setCurrentDeviceId NOTIFY currentDeviceIdChanged)
    Q_PROPERTY(QString currentDeviceToken READ currentDeviceToken WRITE setCurrentDeviceToken NOTIFY currentDeviceTokenChanged)
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY tasksChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ForwardManager(QObject *parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }
    QString authToken() const { return m_authToken; }
    QString currentDeviceId() const { return m_currentDeviceId; }
    QString currentDeviceToken() const { return m_currentDeviceToken; }
    QVariantList tasks() const;
    bool busy() const { return m_pendingRequests > 0; }
    QString lastError() const { return m_lastError; }

    void setBaseUrl(const QString &baseUrl);
    void setAuthToken(const QString &authToken);
    void setCurrentDeviceId(const QString &deviceId);
    void setCurrentDeviceToken(const QString &deviceToken);

    Q_INVOKABLE void refreshTasks();
    Q_INVOKABLE void queryTask(const QString &forwardId);
    Q_INVOKABLE void createForward(const QString &targetDeviceId,
                                   const QString &destinationPath,
                                   bool deleteSource,
                                   const QString &channel,
                                   const QString &relayUploadId,
                                   const QVariantList &files);
    Q_INVOKABLE void accept(const QString &forwardId);
    Q_INVOKABLE void startTransfer(const QString &forwardId);
    Q_INVOKABLE void reportProgress(const QString &forwardId, qint64 transferredBytes);
    Q_INVOKABLE void complete(const QString &forwardId);
    Q_INVOKABLE void finishDownload(const QString &forwardId, qint64 totalBytes);
    Q_INVOKABLE void fail(const QString &forwardId, const QString &reason);
    Q_INVOKABLE void cancel(const QString &forwardId);
    Q_INVOKABLE void handleForwardEvent(const QString &action, const QJsonObject &payload);
    Q_INVOKABLE bool validatePath(const QString &path) const;

signals:
    void baseUrlChanged();
    void authTokenChanged();
    void currentDeviceIdChanged();
    void currentDeviceTokenChanged();
    void tasksChanged();
    void busyChanged();
    void lastErrorChanged();
    void taskUpdated(const QVariantMap &task);
    void forwardCreated(const QString &forwardId);
    void actionFailed(const QString &operation, const QString &code, const QString &message);
    void devicesRefreshRequested();
    void authenticationRequired();
    void finalizationFailed(const QString &forwardId, const QString &code);

private:
    void lifecycleAction(const QString &forwardId, const QString &action,
                         const QJsonObject &body = QJsonObject());
    void sendRequest(const QString &operation, const QByteArray &method,
                     const QString &path, const QJsonObject &body = QJsonObject(),
                     const QString &forwardId = QString());
    void updateTask(const QJsonObject &task);
    void removeTask(const QString &forwardId);
    void handleError(const QString &operation, const QString &forwardId,
                     int status, const QJsonObject &error, const QString &networkError);
    void setLastError(const QString &error);
    void beginRequest();
    void endRequest();
    void applyRequestDefaults(QNetworkRequest &request) const;
    static bool validateFiles(const QVariantList &files, QString *error);
    static bool isValidRelativePath(const QString &path);

    QNetworkAccessManager m_network;
    QMap<QString, QJsonObject> m_tasks;
    QString m_baseUrl;
    QString m_authToken;
    QString m_currentDeviceId;
    QString m_currentDeviceToken;
    QString m_lastError;
    int m_pendingRequests = 0;
};

#endif
