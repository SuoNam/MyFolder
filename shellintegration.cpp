#include "shellintegration.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>
#include <utility>

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
#ifdef Q_OS_LINUX
    syncLinuxServiceMenu();
#endif
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
#ifdef Q_OS_LINUX
    syncLinuxServiceMenu();
#endif
    emit commandsChanged();
}

#ifdef Q_OS_LINUX
void ShellIntegration::syncLinuxServiceMenu()
{
    const QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (dataRoot.isEmpty()) return;
    const QString menuDirectory = QDir(dataRoot).filePath(QStringLiteral("kio/servicemenus"));
    const QString menuPath = QDir(menuDirectory).filePath(QStringLiteral("myfolder.desktop"));
    if (!QDir().mkpath(menuDirectory)) return;

    QStringList actions;
    QByteArray actionGroups;
    if (isEnabled(QStringLiteral("UploadServer"))) {
        actions.append(QStringLiteral("UploadServer"));
        actionGroups += QByteArrayLiteral(
            "\n[Desktop Action UploadServer]\n"
            "Name=上传到 MyFolder 服务器\n"
            "Icon=myfolder\n"
            "Exec=/usr/bin/myfolder --upload-server %F\n");
    }
    if (isEnabled(QStringLiteral("SendClient"))) {
        actions.append(QStringLiteral("SendClient"));
        actionGroups += QByteArrayLiteral(
            "\n[Desktop Action SendClient]\n"
            "Name=发送到指定 MyFolder 客户端\n"
            "Icon=myfolder\n"
            "Exec=/usr/bin/myfolder --send-client %F\n");
    }

    if (actions.isEmpty()) {
        QFile::remove(menuPath);
    } else {
        QByteArray desktopEntry = QByteArrayLiteral(
            "[Desktop Entry]\n"
            "Type=Service\n"
            "Name=MyFolder\n"
            "TryExec=/usr/bin/myfolder\n"
            "X-KDE-ServiceTypes=KonqPopupMenu/Plugin\n"
            "X-KDE-Submenu=MyFolder\n"
            "X-KDE-Priority=TopLevel\n"
            "MimeType=all/allfiles;inode/directory;\n");
        desktopEntry += "Actions=" + actions.join(';').toUtf8() + ";\n" + actionGroups;

        QSaveFile file(menuPath);
        if (file.open(QIODevice::WriteOnly) && file.write(desktopEntry) == desktopEntry.size()
            && file.commit()) {
            QFile::setPermissions(menuPath,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner
                                  | QFileDevice::ReadGroup | QFileDevice::ExeGroup
                                  | QFileDevice::ReadOther | QFileDevice::ExeOther);
        }
    }

    if (qEnvironmentVariableIsSet("MYFOLDER_SKIP_SERVICE_CACHE")) return;
    const QString cacheBuilder = !QStandardPaths::findExecutable(QStringLiteral("kbuildsycoca6")).isEmpty()
        ? QStringLiteral("kbuildsycoca6") : QStringLiteral("kbuildsycoca5");
    const QString executable = QStandardPaths::findExecutable(cacheBuilder);
    if (!executable.isEmpty()) QProcess::startDetached(executable, {QStringLiteral("--noincremental")});
}
#endif

QString ShellIntegration::localFilePath(const QUrl &url) const
{
    if (!url.isLocalFile()) return {};
    return QDir::cleanPath(url.toLocalFile());
}

bool ShellIntegration::openFolder(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        setLastError(tr("Folder path is empty"));
        return false;
    }

    const QString absolutePath = QFileInfo(trimmed).absoluteFilePath();
    QFileInfo info(absolutePath);
    if (info.exists() && !info.isDir()) {
        setLastError(tr("The selected path is not a folder"));
        return false;
    }
    if (!info.exists() && !QDir().mkpath(absolutePath)) {
        setLastError(tr("Unable to create the folder"));
        return false;
    }
#ifdef Q_OS_LINUX
    // KDE can have a directory MIME association without a file-scheme
    // association. In that case xdg-open returns success but opens nothing.
    QStringList managers;
    const QString overrideManager = qEnvironmentVariable("MYFOLDER_FILE_MANAGER").trimmed();
    if (!overrideManager.isEmpty()) {
        managers.append(overrideManager);
    } else {
        const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toUpper();
        if (desktop.contains("KDE")) managers.append(QStringLiteral("dolphin"));
        if (desktop.contains("GNOME") || desktop.contains("UNITY")) managers.append(QStringLiteral("nautilus"));
        managers << QStringLiteral("gio") << QStringLiteral("xdg-open");
    }
    for (const QString &manager : std::as_const(managers)) {
        const QString executable = QStandardPaths::findExecutable(manager);
        if (executable.isEmpty()) continue;
        QStringList arguments;
        if (QFileInfo(executable).fileName() == QStringLiteral("dolphin"))
            arguments << QStringLiteral("--new-window");
        arguments << absolutePath;
        if (QProcess::startDetached(executable, arguments)) {
            setLastError({});
            return true;
        }
    }
#endif
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath))) {
        setLastError(tr("The desktop could not open this folder"));
        return false;
    }
    setLastError({});
    return true;
}

void ShellIntegration::setLastError(const QString &message)
{
    if (m_lastError == message) return;
    m_lastError = message;
    emit lastErrorChanged();
}
