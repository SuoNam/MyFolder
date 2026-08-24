#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>

class ShellIntegration final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(int enabledCount READ enabledCount NOTIFY commandsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ShellIntegration(QObject *parent = nullptr);

    QVariantList commands() const;
    int enabledCount() const;
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void setCommandEnabled(const QString &id, bool enabled);
    Q_INVOKABLE QString localFilePath(const QUrl &url) const;
    Q_INVOKABLE bool openFolder(const QString &path);

signals:
    void commandsChanged();
    void lastErrorChanged();

private:
    void setLastError(const QString &message);
    bool isEnabled(const QString &id) const;
    void ensureDefaults();
#ifdef Q_OS_LINUX
    void syncLinuxServiceMenu();
#endif

    QString m_lastError;
};
