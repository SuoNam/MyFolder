#ifndef LANTRANSFERMANAGER_H
#define LANTRANSFERMANAGER_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTcpServer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QTcpSocket;
class QNetworkRequest;
class QFile;
class LanTransferManagerTest;

class LanTransferManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId WRITE setCurrentDeviceId NOTIFY credentialsChanged)
    Q_PROPERTY(QString currentDeviceToken READ currentDeviceToken WRITE setCurrentDeviceToken NOTIFY credentialsChanged)
    Q_PROPERTY(QString receiveRoot READ receiveRoot WRITE setReceiveRoot NOTIFY receiveRootChanged)
    Q_PROPERTY(int listenPort READ listenPort NOTIFY listenPortChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantMap outgoingProgress READ outgoingProgress NOTIFY outgoingProgressChanged)

public:
    explicit LanTransferManager(QObject *parent = nullptr);
    ~LanTransferManager() override;

    QString baseUrl() const { return m_baseUrl; }
    QString authToken() const { return m_authToken; }
    QString currentDeviceId() const { return m_currentDeviceId; }
    QString currentDeviceToken() const { return m_currentDeviceToken; }
    QString receiveRoot() const { return m_receiveRoot; }
    int listenPort() const { return m_server.isListening() ? int(m_server.serverPort()) : 0; }
    QString lastError() const { return m_lastError; }
    bool busy() const { return m_busy; }
    QVariantMap outgoingProgress() const { return m_outgoingProgress; }

    void setBaseUrl(const QString &value);
    void setAuthToken(const QString &value);
    void setCurrentDeviceId(const QString &value);
    void setCurrentDeviceToken(const QString &value);
    void setReceiveRoot(const QString &value);

    Q_INVOKABLE void sendFile(const QString &targetDeviceId, const QString &targetAddress,
                              int targetPort, const QString &destinationPath,
                              const QString &localFilePath);
    Q_INVOKABLE void sendPath(const QString &targetDeviceId, const QString &targetAddress,
                              int targetPort, const QString &destinationPath,
                              const QString &localPath);
    Q_INVOKABLE void sendPathViaRoute(const QString &targetDeviceId, const QString &targetAddress,
                                     const QString &sourceAddress, int targetPort,
                                     const QString &destinationPath, const QString &localPath);
    Q_INVOKABLE void prepareIncoming(const QVariantMap &task);
    Q_INVOKABLE void handleTaskUpdated(const QVariantMap &task);
    Q_INVOKABLE void reconcileTasks(const QStringList &activeTaskIds);
    Q_INVOKABLE void cancel(const QString &forwardId);
    Q_INVOKABLE bool isDirectory(const QString &path) const;

signals:
    void baseUrlChanged();
    void authTokenChanged();
    void credentialsChanged();
    void receiveRootChanged();
    void listenPortChanged();
    void lastErrorChanged();
    void busyChanged();
    void outgoingProgressChanged();
    void forwardCreated(const QString &forwardId);
    void createFailed(const QString &message);
    void outgoingFallbackRequested(const QString &forwardId, const QString &targetDeviceId,
                                   const QString &destinationPath, const QString &localPath,
                                   bool directory, const QString &reason);
    void incomingProgress(const QString &forwardId, qint64 verifiedBytes);
    void outgoingProgressReady(const QString &forwardId, qint64 receivedBytes);
    void incomingCompleted(const QString &forwardId, qint64 totalBytes);
    void incomingFailed(const QString &forwardId, const QString &code, const QString &message);

private:
    friend class LanTransferManagerTest;
    struct TransferFile {
        QString path, localPath, sha256;
        qint64 size = 0;
    };
    struct PathManifest {
        QString rootPath;
        QList<TransferFile> files;
        QStringList directories;
        bool directory = false;
        QString error;
    };
    struct Outgoing {
        QString forwardId, targetDeviceId, targetAddress, sourceAddress;
        QString destinationPath, localRootPath, token;
        QList<TransferFile> files;
        QStringList directories;
        int fileIndex = 0;
        int targetPort = 0;
        int connectionAttempts = 0;
        qint64 totalSize = 0, sent = 0, responseLength = -1;
        bool directory = false;
        bool started = false, responseReady = false, allQueued = false, fileAcknowledged = false;
        bool retryScheduled = false;
        QByteArray responseBuffer;
        QTcpSocket *socket = nullptr;
        QFile *file = nullptr;
    };
    struct Incoming {
        QString forwardId, token, destinationPath;
        QHash<QString, TransferFile> files;
        QStringList directories;
        QSet<QString> completed;
        QHash<QString, QString> finalPaths;
        qint64 totalSize = 0, verifiedBytes = 0;
        qint64 lastProgressBytes = 0, lastProgressAtMs = 0;
    };
    struct ReceiverConnection {
        QByteArray buffer;
        qint64 controlLength = -1;
        bool headerReady = false;
        QString forwardId, filePath, expectedHash, finalPath, partPath, metaPath;
        qint64 received = 0, total = 0;
        QFile *file = nullptr;
    };

    void createTask(const QString &targetDeviceId, const QString &targetAddress,
                    const QString &sourceAddress, int targetPort,
                    const QString &destinationPath, const PathManifest &manifest);
    void startOutgoing(const QString &forwardId);
    void startNextFile(const QString &forwardId);
    void pumpOutgoing(const QString &forwardId);
    void retryOutgoing(const QString &forwardId, const QString &reason);
    void failOutgoing(const QString &forwardId, const QString &reason);
    void acceptConnection();
    void consumeReceiver(QTcpSocket *socket);
    void finalizeReceiver(QTcpSocket *socket);
    void setLastError(const QString &value);
    void setBusy(bool value);
    void applyRequestDefaults(QNetworkRequest &request) const;
    QString persistencePath() const;
    void loadOutgoing();
    void saveOutgoing() const;
    QString incomingStatePath(const QString &forwardId) const;
    void loadIncomingState(Incoming &incoming) const;
    void saveIncomingState(const Incoming &incoming) const;
    void publishOutgoingProgress(const QString &forwardId, qint64 receivedBytes);

    static QString sha256File(const QString &path);
    static PathManifest scanPath(const QString &path);
    static QString availableTargetPath(const QString &path);
    static quint16 configuredListenPort();
    static bool safeRelativePath(const QString &path);
    static QByteArray frame(const QJsonObject &object);
    static bool takeFrame(QByteArray &buffer, qint64 &expectedLength, QJsonObject *object);

    QTcpServer m_server;
    QNetworkAccessManager m_network;
    QString m_baseUrl;
    QString m_authToken;
    QString m_currentDeviceId;
    QString m_currentDeviceToken;
    QString m_receiveRoot;
    QString m_lastError;
    bool m_busy = false;
    QHash<QString, Outgoing> m_outgoing;
    QHash<QString, Incoming> m_incoming;
    QHash<QTcpSocket *, ReceiverConnection> m_receivers;
    QVariantMap m_outgoingProgress;
};

#endif
