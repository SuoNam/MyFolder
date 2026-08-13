#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QTimer>
#include <QStandardPaths>
#include <QSet>
#include <QUrl>
#include <QMutex>
#include <QDebug>
#include <QDateTime>
#include <QSaveFile>
#include <QtQml/qqmlregistration.h>

struct TransferFileItem {
    QString path;              // Relative UTF-8 path using '/' (e.g., "docs/a.txt")
    qint64 size = 0;           // File size in bytes
    QString sha256;            // Whole file 64-char hex SHA-256
    int totalChunks = 0;       // ceil(size / chunkSize)
    QString state;             // PENDING, UPLOADING, VERIFYING, COMPLETED, FAILED
    QSet<int> completedChunks;
    QList<int> missingChunks;
    QString failureReason;

    // Retry & Local tracking
    int retryCount = 0;
    qint64 nextRetryTimeMs = 0;
    qint64 uploadedBytes = 0;
    bool completionInFlight = false;
};

struct TransferTaskGroup {
    QString uploadId;          // Server returned task UUID (or temp_xxx before creation)
    QString localRootPath;     // Local folder or file root path
    QString directoryName;     // Target directory name (or file directory name)
    QString parentPath;        // Server parent path (default "inbox")
    QString targetPath;        // Exact v1.1.1 server destination root
    QString serverBaseUrl;     // Persisted endpoint used by this task
    qint64 chunkSize = 4LL * 1024 * 1024; // v1.1.1 default: 4 MiB
    int totalFiles = 0;
    qint64 totalBytes = 0;
    qint64 uploadedBytes = 0;
    QString state;             // PENDING, UPLOADING, VERIFYING, COMPLETED, FAILED, CANCELLED

    bool statusSyncedWithServer = false; // Must be true before uploading chunks
    bool isSyncingStatus = false;
    bool creationInFlight = false;
    bool completionInFlight = false;
    bool pausedByUser = false;
    QList<TransferFileItem> files;
    int retryCount = 0;
    qint64 nextRetryTimeMs = 0;
};

class TransferManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList taskList READ getTaskList NOTIFY taskListChanged)
    Q_PROPERTY(bool isNetworkAvailable READ isNetworkAvailable NOTIFY networkStatusChanged)
    Q_PROPERTY(double globalSpeedMbps READ globalSpeedMbps NOTIFY speedUpdated)
    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)

public:
    explicit TransferManager(QObject *parent = nullptr);
    ~TransferManager();

    QVariantList getTaskList() const;
    bool isNetworkAvailable() const { return m_isNetworkAvailable; }
    double globalSpeedMbps() const { return m_globalSpeedMbps; }

    QString authToken() const { return m_authToken; }
    void setAuthToken(const QString &token);

    QString baseUrl() const { return m_baseUrl; }
    void setBaseUrl(const QString &url);

    // QML Invokable APIs (myfolder-transfer v1.0.0 compliant)
    Q_INVOKABLE QString startFileUpload(const QString &localFilePath, const QString &parentPath = "inbox");
    Q_INVOKABLE QString startFolderUpload(const QString &localFolderPath, const QString &parentPath = "inbox");
    Q_INVOKABLE void queryTaskStatus(const QString &uploadId);
    Q_INVOKABLE void pauseTask(const QString &uploadId);
    Q_INVOKABLE void resumeTask(const QString &uploadId);
    Q_INVOKABLE void cancelTask(const QString &uploadId);
    Q_INVOKABLE void retryTask(const QString &uploadId);
    Q_INVOKABLE void clearCompletedTasks();

    // Static Utility: SHA-256 calculation & percent encoding
    static QString calculateFileHash(const QString &filePath);
    static QString calculateChunkHash(const QString &filePath, qint64 offset, qint64 size);
    static QString encodePathForProtocol(const QString &relativePath);
    static bool isValidProtocolPath(const QString &path);
    static qint64 calculateUploadedBytes(qint64 fileSize, qint64 chunkSize,
                                         const QSet<int> &completedChunks);
    static qint64 calculateBackoffDelayMs(int retryCount);

signals:
    void taskListChanged();
    void networkStatusChanged(bool available);
    void speedUpdated(double speedMbps);
    void authTokenChanged();
    void baseUrlChanged();
    void taskProgressUpdated(const QString &uploadId, qint64 uploaded, qint64 total, double speedMbps);
    void taskStatusChanged(const QString &uploadId, const QString &status, const QString &errorMsg);
    void taskIdChanged(const QString &temporaryId, const QString &uploadId);

private slots:
    void processNextChunk();
    void checkNetworkStatus();

private:
    void createServerTask(TransferTaskGroup &group);
    void uploadChunk(TransferTaskGroup &group, int fileIndex, int chunkIndex);
    void completeSingleFile(TransferTaskGroup &group, int fileIndex);
    void completeWholeTask(TransferTaskGroup &group);

    void saveTasksToStorage();
    void loadTasksFromStorage();
    void updateSpeedStats();

    QString getStorageFilePath() const;
    void handleServerError(const QJsonObject &errorJson, const QString &uploadId, int httpStatus, int fileIndex = -1, int chunkIndex = -1);
    void applyRequestDefaults(QNetworkRequest &request) const;
    void recalculateProgress(TransferTaskGroup &group) const;
    bool applyServerTaskResponse(TransferTaskGroup &group, const QJsonObject &response,
                                 QString *errorMessage = nullptr) const;
    bool isRetryableError(const QString &code, int httpStatus) const;
    QString normalizedParentPath(const QString &path) const;
    QString endpointFor(const TransferTaskGroup &group) const;
    bool validateLocalSources(TransferTaskGroup &group, QString *errorMessage = nullptr) const;

private:
    QNetworkAccessManager *m_netManager;
    QString m_baseUrl;
    QString m_authToken;
    bool m_isNetworkAvailable = true;
    double m_globalSpeedMbps = 0.0;

    QMap<QString, TransferTaskGroup> m_tasks; // Key: uploadId
    QStringList m_activeQueue;

    QTimer *m_queueTimer;
    QTimer *m_networkCheckTimer;
    QTimer *m_speedTimer;

    qint64 m_bytesUploadedInWindow = 0;
    bool m_isUploadingChunk = false;
    bool m_networkCheckInFlight = false;
    const qint64 DEFAULT_CHUNK_SIZE = 4LL * 1024 * 1024; // Match Web: 4 MiB
    const int MAX_RETRIES = 5;
    const int REQUEST_TIMEOUT_MS = 30000;
};

#endif // TRANSFERMANAGER_H
