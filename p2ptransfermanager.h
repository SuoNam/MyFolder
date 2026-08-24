#ifndef P2PTRANSFERMANAGER_H
#define P2PTRANSFERMANAGER_H

#include <QCryptographicHash>
#include <QFile>
#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <rtc/rtc.hpp>

class QNetworkRequest;
class P2pTransferManagerTest;

class P2pTransferManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY credentialsChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId WRITE setCurrentDeviceId NOTIFY credentialsChanged)
    Q_PROPERTY(QString currentDeviceToken READ currentDeviceToken WRITE setCurrentDeviceToken NOTIFY credentialsChanged)
    Q_PROPERTY(QString receiveRoot READ receiveRoot WRITE setReceiveRoot NOTIFY receiveRootChanged)
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QVariantMap outgoingProgress READ outgoingProgress NOTIFY outgoingProgressChanged)
    Q_PROPERTY(QVariantMap incomingVerifiedProgress READ incomingVerifiedProgress NOTIFY incomingVerifiedProgressChanged)

public:
    explicit P2pTransferManager(QObject *parent = nullptr);
    ~P2pTransferManager() override;

    QString baseUrl() const { return m_baseUrl; }
    QString authToken() const { return m_authToken; }
    QString currentDeviceId() const { return m_currentDeviceId; }
    QString currentDeviceToken() const { return m_currentDeviceToken; }
    QString receiveRoot() const { return m_receiveRoot; }
    QString lastError() const { return m_lastError; }
    QVariantMap outgoingProgress() const { return m_outgoingProgress; }
    QVariantMap incomingVerifiedProgress() const { return m_incomingVerifiedProgress; }
    bool available() const { return true; }

    void setBaseUrl(const QString &value);
    void setAuthToken(const QString &value);
    void setCurrentDeviceId(const QString &value);
    void setCurrentDeviceToken(const QString &value);
    void setReceiveRoot(const QString &value);

    Q_INVOKABLE void sendPath(const QString &targetDeviceId, const QString &destinationPath,
                              const QString &localPath);
    Q_INVOKABLE void prepareIncoming(const QVariantMap &task);
    Q_INVOKABLE void handleTaskUpdated(const QVariantMap &task);
    Q_INVOKABLE void reconcileTasks(const QStringList &activeTaskIds);
    Q_INVOKABLE void handleSignal(const QJsonObject &payload);
    Q_INVOKABLE bool isDirectory(const QString &path) const;

signals:
    void baseUrlChanged();
    void credentialsChanged();
    void receiveRootChanged();
    void lastErrorChanged();
    void outgoingProgressChanged();
    void incomingVerifiedProgressChanged();
    void forwardCreated(const QString &forwardId);
    void createFailed(const QString &message);
    void outgoingFallbackRequested(const QString &forwardId, const QString &targetDeviceId,
                                   const QString &destinationPath, const QString &localPath,
                                   bool directory, const QString &reason);
    void incomingProgress(const QString &forwardId, qint64 verifiedBytes);
    void outgoingProgressReady(const QString &forwardId, qint64 transferredBytes);
    void incomingCompleted(const QString &forwardId, qint64 totalBytes);
    void incomingFailed(const QString &forwardId, const QString &code, const QString &message);
    void localSignalReady(const QString &forwardId, const QJsonObject &signal);

private:
    friend class P2pTransferManagerTest;
    struct FileEntry { QString path, localPath, sha256; qint64 size = 0; };
    struct Manifest { QString rootPath; QList<FileEntry> files; QStringList directories; bool directory = false; QString error; };
    struct Session {
        QString forwardId, sourceDeviceId, targetDeviceId, destinationPath, token, localRootPath;
        QString manifestRoot, resolvedRoot;
        QList<FileEntry> files; QStringList directories; bool source = false, directory = false;
        int fileIndex = 0; qint64 totalBytes = 0, transferred = 0, confirmedBytes = 0, startedMs = 0;
        qint64 lastActivityMs = 0;
        std::shared_ptr<rtc::PeerConnection> peer; std::shared_ptr<rtc::DataChannel> channel;
        std::unique_ptr<QFile> file; std::unique_ptr<QCryptographicHash> hash;
        QString currentPath, currentPartPath, currentFinalPath; qint64 currentReceived = 0;
        bool helloAccepted = false, awaitingAck = false, doneSent = false, completed = false;
        bool incomingPrepared = false;
        bool remoteDescriptionSet = false;
        QList<QPair<QString, QString>> pendingRemoteCandidates;
    };

    static Manifest scanPath(const QString &path);
    static QString sha256File(const QString &path);
    static bool safeRelativePath(const QString &path);
    static QString availableTargetPath(const QString &path);
    void createTask(const QString &targetDeviceId, const QString &destinationPath,
                    const Manifest &manifest);
    void ensurePeer(const QString &forwardId, bool offerer);
    void attachChannel(const QString &forwardId, const std::shared_ptr<rtc::DataChannel> &channel);
    void flushRemoteCandidates(const QString &forwardId, int attempt = 0);
    void sendSignal(const QString &forwardId, const QJsonObject &signal, int attempt = 0);
    void processText(const QString &forwardId, const QString &message);
    void processBinary(const QString &forwardId, const QByteArray &data);
    void pump(const QString &forwardId);
    void startNextFile(const QString &forwardId);
    void fail(const QString &forwardId, const QString &code, const QString &message, bool fallback);
    void closeSession(const QString &forwardId);
    void checkTimeouts(qint64 now);
    void publishOutgoingProgress(const QString &forwardId, qint64 transferredBytes);
    void publishIncomingProgress(const QString &forwardId, qint64 verifiedBytes);
    void loadOutgoingSessions();
    void saveOutgoingSessions() const;
    QString persistencePath() const;
    void setLastError(const QString &value);
    void applyHeaders(QNetworkRequest &request) const;

    QNetworkAccessManager m_network;
    QHash<QString, std::shared_ptr<Session>> m_sessions;
    QVariantMap m_outgoingProgress;
    QVariantMap m_incomingVerifiedProgress;
    QTimer m_watchdog;
    bool m_testSignaling = false;
    QString m_baseUrl, m_authToken, m_currentDeviceId, m_currentDeviceToken, m_receiveRoot, m_lastError;
};

#endif
