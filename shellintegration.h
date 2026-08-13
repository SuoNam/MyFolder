#pragma once

#include <QObject>
#include <QVariantList>

class ShellIntegration final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(int enabledCount READ enabledCount NOTIFY commandsChanged)

public:
    explicit ShellIntegration(QObject *parent = nullptr);

    QVariantList commands() const;
    int enabledCount() const;

    Q_INVOKABLE void setCommandEnabled(const QString &id, bool enabled);

signals:
    void commandsChanged();

private:
    bool isEnabled(const QString &id) const;
    void ensureDefaults();
};
