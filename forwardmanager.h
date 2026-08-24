#ifndef FORWARDMANAGER_H
#define FORWARDMANAGER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>
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
    Q_PROPERTY(QVariantList historyTasks READ historyTasks NOTIFY historyTasksChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ForwardManager(QObject *parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }
    QString authToken() const { return m_authToken; }
    QString currentDeviceId() const { return m_currentDeviceId; }
    QString currentDeviceToken() const { return m_currentDeviceToken; }
    QVariantList tasks() const;
    QVariantList historyTasks() const { return m_historyTasks; }
    bool busy() const { return m_pendingRequests > 0; }
    QString lastError() const { return m_lastError; }

    void setBaseUrl(const QString &baseUrl);
    void setAuthToken(const QString &authToken);
    void setCurrentDeviceId(const QString &deviceId);
    void setCurrentDeviceToken(const QString &deviceToken);

    Q_INVOKABLE void refreshTasks();
    Q_INVOKABLE void refreshHistory();
    Q_INVOKABLE void queryTask(const QString &forwardId);
    Q_INVOKABLE void createForward(const QString &targetDeviceId,
                                   const QString &destinationPath,
                                   bool deleteSource,
                                   const QString &channel,
                                   const QString &relayUploadId,
                                   const QVariantList &files);
    Q_INVOKABLE void createForwardWithDirectories(const QString &targetDeviceId,
                                                  const QString &destinationPath,
                                                  bool deleteSource,
                                                  const QString &channel,
                                                  const QString &relayUploadId,
                                                  const QVariantList &files,
                                                  const QVariantList &directories);
    Q_INVOKABLE void accept(const QString &forwardId);
    Q_INVOKABLE void reject(const QString &forwardId);
    Q_INVOKABLE void startTransfer(const QString &forwardId);
    Q_INVOKABLE void reportProgress(const QString &forwardId, qint64 transferredBytes);
    Q_INVOKABLE void complete(const QString &forwardId);
    Q_INVOKABLE void finishDownload(const QString &forwardId, qint64 totalBytes);
    Q_INVOKABLE void fail(const QString &forwardId, const QString &reason);
    Q_INVOKABLE void cancel(const QString &forwardId);
    Q_INVOKABLE void dismissTask(const QString &forwardId);
    Q_INVOKABLE void hideSupersededTask(const QString &forwardId);
    Q_INVOKABLE void markAutoAccepting(const QString &forwardId);
    Q_INVOKABLE bool isAutoAccepting(const QString &forwardId) const;
    Q_INVOKABLE void handleForwardEvent(const QString &action, const QJsonObject &payload);
    Q_INVOKABLE void rememberAcceptedTransfer(const QVariantMap &task);
    Q_INVOKABLE bool canAutoAcceptFallback(const QVariantMap &task);
    Q_INVOKABLE void forgetAcceptedTransfer(const QVariantMap &task);
    Q_INVOKABLE bool validatePath(const QString &path) const;

signals:
    void baseUrlChanged();
    void authTokenChanged();
    void currentDeviceIdChanged();
    void currentDeviceTokenChanged();
    void tasksChanged();
    void historyTasksChanged();
    void busyChanged();
    void lastErrorChanged();
    void taskUpdated(const QVariantMap &task);
    void forwardCreated(const QString &forwardId);
    void actionFailed(const QString &operation, const QString &code, const QString &message);
    void devicesRefreshRequested();
    void authenticationRequired();
    void finalizationFailed(const QString &forwardId, const QString &code);
    void tasksReconciled(const QStringList &activeTaskIds);

private:
    friend class ForwardManagerTest;
    void lifecycleAction(const QString &forwardId, const QString &action,
                         const QJsonObject &body = QJsonObject());
    void sendRequest(const QString &operation, const QByteArray &method,
                     const QString &path, const QJsonObject &body = QJsonObject(),
                     const QString &forwardId = QString());
    void updateTask(const QJsonObject &task);
    void removeTask(const QString &forwardId);
    void applyTaskList(const QJsonArray &tasks, quint64 requestSequence,
                       quint64 baselineRevision);
    void handleError(const QString &operation, const QString &forwardId,
                     int status, const QJsonObject &error, const QString &networkError);
    void setLastError(const QString &error);
    void beginRequest();
    void endRequest();
    void applyRequestDefaults(QNetworkRequest &request) const;
    void continueProgressQueue(const QString &forwardId);
    static QString transferFingerprint(const QVariantMap &task);
    static int channelRank(const QString &channel);
    void loadAcceptedTransfers();
    void saveAcceptedTransfers() const;
    void pruneAcceptedTransfers();
    void loadDismissedTasks();
    void saveDismissedTasks() const;
    static bool validateFiles(const QVariantList &files, QString *error);
    static bool validateDirectories(const QVariantList &directories, QString *error);
    static bool isValidRelativePath(const QString &path);

    QNetworkAccessManager m_network;
    QMap<QString, QJsonObject> m_tasks;
    QVariantList m_historyTasks;
    struct AcceptedTransfer {
        int maxChannelRank = -1;
        qint64 acceptedAtMs = 0;
    };
    QMap<QString, AcceptedTransfer> m_acceptedTransfers;
    QSet<QString> m_dismissedTaskIds;
    QSet<QString> m_autoAcceptingTaskIds;
    QSet<QString> m_progressInFlight;
    QHash<QString, qint64> m_pendingProgress;
    QHash<QString, qint64> m_pendingFinalProgress;
    QHash<QString, quint64> m_taskMutationRevisions;
    QString m_baseUrl;
    QString m_authToken;
    QString m_currentDeviceId;
    QString m_currentDeviceToken;
    QString m_lastError;
    int m_pendingRequests = 0;
    quint64 m_taskRevision = 0;
    quint64 m_latestListRequestSequence = 0;
};

#endif
