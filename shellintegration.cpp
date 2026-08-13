#include "shellintegration.h"

#include <QSettings>
#include <QVariantMap>

namespace {
struct CommandDefinition {
    const char *id;
    const char *title;
    const char *description;
};

constexpr CommandDefinition commandDefinitions[] = {
    {"UploadServer", "上传到 MyFolder 服务器", "把选中的文件上传到个人服务器空间"},
    {"SendClient", "发送到指定 MyFolder 客户端", "选择同账号下的客户端并发送文件"},
};

QSettings shellSettings()
{
#ifdef Q_OS_WIN
    return QSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\MyFolder"),
                     QSettings::NativeFormat);
#else
    return QSettings();
#endif
}
}

ShellIntegration::ShellIntegration(QObject *parent) : QObject(parent)
{
    ensureDefaults();
}

void ShellIntegration::ensureDefaults()
{
    auto settings = shellSettings();
    for (const auto &definition : commandDefinitions) {
        const QString key = QStringLiteral("ShellCommands/") + QString::fromLatin1(definition.id);
        if (!settings.contains(key)) settings.setValue(key, 1);
    }
    settings.sync();
}

bool ShellIntegration::isEnabled(const QString &id) const
{
    auto settings = shellSettings();
    return settings.value(QStringLiteral("ShellCommands/") + id, 1).toInt() != 0;
}

QVariantList ShellIntegration::commands() const
{
    QVariantList result;
    for (const auto &definition : commandDefinitions) {
        const QString id = QString::fromLatin1(definition.id);
        result.push_back(QVariantMap{{QStringLiteral("id"), id},
                                     {QStringLiteral("title"), QString::fromUtf8(definition.title)},
                                     {QStringLiteral("description"), QString::fromUtf8(definition.description)},
                                     {QStringLiteral("enabled"), isEnabled(id)}});
    }
    return result;
}

int ShellIntegration::enabledCount() const
{
    int count = 0;
    for (const auto &definition : commandDefinitions)
        if (isEnabled(QString::fromLatin1(definition.id))) ++count;
    return count;
}

void ShellIntegration::setCommandEnabled(const QString &id, bool enabled)
{
    bool known = false;
    for (const auto &definition : commandDefinitions) {
        if (id == QString::fromLatin1(definition.id)) {
            known = true;
            break;
        }
    }
    if (!known || isEnabled(id) == enabled) return;
    auto settings = shellSettings();
    settings.setValue(QStringLiteral("ShellCommands/") + id, enabled ? 1 : 0);
    settings.sync();
    emit commandsChanged();
}
