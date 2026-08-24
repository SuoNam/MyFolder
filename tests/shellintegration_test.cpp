#include "../shellintegration.h"

#include <QDesktopServices>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class FileUrlReceiver final : public QObject
{
    Q_OBJECT

public:
    QUrl receivedUrl;

public slots:
    void handle(const QUrl &url) { receivedUrl = url; }
};

class ShellIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void convertsLinuxFileUrlToAbsolutePath();
    void convertsWindowsFileUrlToNativePath();
    void rejectsNonLocalUrl();
    void opensFolderWithLocalFileUrl();
#ifdef Q_OS_LINUX
    void createsKdeServiceMenuFromSettings();
#endif
};

void ShellIntegrationTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings().clear();
    qputenv("MYFOLDER_SKIP_SERVICE_CACHE", "1");
}

void ShellIntegrationTest::convertsLinuxFileUrlToAbsolutePath()
{
    ShellIntegration integration;
    QCOMPARE(integration.localFilePath(QUrl("file:///home/suo/My%20File.txt")),
             QString("/home/suo/My File.txt"));
}

void ShellIntegrationTest::convertsWindowsFileUrlToNativePath()
{
    ShellIntegration integration;
    const QString path = integration.localFilePath(QUrl("file:///C:/Users/Test/file.txt"));
#ifdef Q_OS_WIN
    QCOMPARE(path, QString("C:/Users/Test/file.txt"));
#else
    QCOMPARE(path, QString("/C:/Users/Test/file.txt"));
#endif
}

void ShellIntegrationTest::rejectsNonLocalUrl()
{
    ShellIntegration integration;
    QVERIFY(integration.localFilePath(QUrl("https://example.com/file.txt")).isEmpty());
}

void ShellIntegrationTest::opensFolderWithLocalFileUrl()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
#ifdef Q_OS_LINUX
    qputenv("MYFOLDER_FILE_MANAGER", "/usr/bin/true");
#else
    FileUrlReceiver receiver;
    QDesktopServices::setUrlHandler("file", &receiver, "handle");
#endif

    ShellIntegration integration;
    QVERIFY(integration.openFolder(temporary.path()));
#ifdef Q_OS_LINUX
    QVERIFY(integration.lastError().isEmpty());
    qunsetenv("MYFOLDER_FILE_MANAGER");
#else
    QCOMPARE(receiver.receivedUrl, QUrl::fromLocalFile(temporary.path()));
    QVERIFY(integration.lastError().isEmpty());

    QDesktopServices::unsetUrlHandler("file");
#endif
}

#ifdef Q_OS_LINUX
void ShellIntegrationTest::createsKdeServiceMenuFromSettings()
{
    const QString menuPath = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(QStringLiteral("kio/servicemenus/myfolder.desktop"));
    ShellIntegration integration;
    QFile menu(menuPath);
    QVERIFY(menu.open(QIODevice::ReadOnly));
    QByteArray contents = menu.readAll();
    QVERIFY(QFileInfo(menuPath).permissions().testFlag(QFileDevice::ExeOwner));
    QVERIFY(contents.contains("Actions=UploadServer;SendClient;"));
    QVERIFY(contents.contains("--upload-server %F"));
    QVERIFY(contents.contains("--send-client %F"));
    menu.close();

    integration.setCommandEnabled(QStringLiteral("UploadServer"), false);
    QVERIFY(menu.open(QIODevice::ReadOnly));
    contents = menu.readAll();
    QVERIFY(!contents.contains("--upload-server %F"));
    QVERIFY(contents.contains("Actions=SendClient;"));
    menu.close();

    integration.setCommandEnabled(QStringLiteral("SendClient"), false);
    QVERIFY(!QFileInfo::exists(menuPath));

    integration.setCommandEnabled(QStringLiteral("UploadServer"), true);
    integration.setCommandEnabled(QStringLiteral("SendClient"), true);
}
#endif

QTEST_MAIN(ShellIntegrationTest)
#include "shellintegration_test.moc"
