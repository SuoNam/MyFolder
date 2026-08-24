#include "../lantransfermanager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

class LanTransferManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void usesConfiguredListenPort();
    void usesFixedProductionListenPortByDefault();
    void transfersAndResumesAFileOverTcp();
    void retriesWhenReceiverManifestArrivesLate();
    void transfersFolderWithNestedAndEmptyDirectories();
    void rejectsWrongCapabilityToken();
    void rejectsFileWhoseFinalHashDoesNotMatchManifest();
    void fallsBackWhenControlTaskCreationConnectionCloses();
    void fallsBackWhenControlTaskHasNoCapability();
    void fallsBackWhenEndpointCannotBeReached();
    void restoresOutgoingContextAfterRestart();
    void fallsBackToSystemRouteWhenSourceBindFails();
    void senderUsesReceiverAcknowledgedProgress();
    void rejectionCleansOutgoingWithoutFallback();
    void bypassesApplicationProxyForLanSocket();
};

void LanTransferManagerTest::init()
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("MYFOLDER_LAN_PORT", "0");
    QFile::remove(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                      .filePath("lan_outgoing_v2.json"));
}

void LanTransferManagerTest::usesConfiguredListenPort()
{
    QTcpServer reservation;
    QVERIFY(reservation.listen(QHostAddress::AnyIPv4, 0));
    const quint16 port = reservation.serverPort();
    reservation.close();

    qputenv("MYFOLDER_LAN_PORT", QByteArray::number(port));
    LanTransferManager manager;
    QCOMPARE(manager.listenPort(), int(port));
    QVERIFY(manager.lastError().isEmpty());
}

void LanTransferManagerTest::usesFixedProductionListenPortByDefault()
{
    qunsetenv("MYFOLDER_LAN_PORT");
    QCOMPARE(LanTransferManager::configuredListenPort(), quint16(45897));
}

void LanTransferManagerTest::transfersAndResumesAFileOverTcp()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());

    QByteArray content;
    content.resize(1024 * 1024 + 137);
    for (qsizetype i = 0; i < content.size(); ++i) content[i] = char(i % 251);
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    const QString sourcePath = QDir(temporary.path()).filePath("source.bin");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(content), content.size());
    source.close();

    const QString receiveRoot = QDir(temporary.path()).filePath("receive");
    const QString destination = QDir(receiveRoot).filePath("Downloads/source.bin");
    QVERIFY(QDir().mkpath(QFileInfo(destination).absolutePath()));
    QFile partial(destination + ".myfolder-part");
    QVERIFY(partial.open(QIODevice::WriteOnly));
    const qint64 resumeOffset = content.size() / 3;
    QCOMPARE(partial.write(content.left(resumeOffset)), resumeOffset);
    partial.close();
    QFile metadata(destination + ".myfolder-part.json");
    QVERIFY(metadata.open(QIODevice::WriteOnly));
    metadata.write(QJsonDocument(QJsonObject{{"sha256", hash}, {"size", double(content.size())}})
                       .toJson(QJsonDocument::Compact));
    metadata.close();

    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(receiveRoot);
    QVERIFY(receiver.listenPort() > 0);

    const QString forwardId = "lan-test-forward";
    const QString token = "test-capability-token-with-sufficient-entropy";
    receiver.prepareIncoming(QVariantMap{
        {"forwardId", forwardId}, {"sourceDeviceId", "source"}, {"targetDeviceId", "target"},
        {"channel", "LAN"}, {"state", "OFFERED"}, {"directTransferToken", token},
        {"destinationPath", "Downloads"},
        {"files", QVariantList{QVariantMap{{"path", "source.bin"},
                                           {"size", content.size()}, {"sha256", hash}}}}
    });

    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = forwardId;
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = token;
    outgoing.totalSize = content.size();
    outgoing.files.append(LanTransferManager::TransferFile{
        "source.bin", sourcePath, hash, content.size()});
    sender.m_outgoing.insert(forwardId, outgoing);

    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    QSignalSpy failed(&receiver, &LanTransferManager::incomingFailed);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.handleTaskUpdated(QVariantMap{{"forwardId", forwardId}, {"sourceDeviceId", "source"},
                                         {"channel", "LAN"}, {"state", "ACCEPTED"}});

    if (!completed.wait(10000)) {
        QString detail = QString("receiverError=%1 failed=%2 fallback=%3")
                             .arg(receiver.lastError()).arg(failed.count()).arg(fallback.count());
        if (!failed.isEmpty()) detail += " failedMessage=" + failed.first().at(2).toString();
        if (!fallback.isEmpty()) detail += " fallbackMessage=" + fallback.first().at(4).toString();
        QFAIL(qPrintable(detail));
    }
    QCOMPARE(failed.count(), 0);
    QCOMPARE(fallback.count(), 0);
    QCOMPARE(completed.first().at(0).toString(), forwardId);
    QCOMPARE(completed.first().at(1).toLongLong(), qint64(content.size()));
    QFile received(destination);
    QVERIFY(received.open(QIODevice::ReadOnly));
    QCOMPARE(received.readAll(), content);
    QVERIFY(!QFile::exists(destination + ".myfolder-part"));
    QVERIFY(!QFile::exists(destination + ".myfolder-part.json"));
}

void LanTransferManagerTest::retriesWhenReceiverManifestArrivesLate()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QByteArray content(128 * 1024, 'r');
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    const QString sourcePath = QDir(temporary.path()).filePath("late.bin");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(content), content.size());
    source.close();

    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(QDir(temporary.path()).filePath("receive"));
    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    const QString forwardId = "late-manifest-forward";
    const QString token = "late-manifest-capability";
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = forwardId;
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = token;
    outgoing.totalSize = content.size();
    outgoing.files.append({"late.bin", sourcePath, hash, content.size()});
    sender.m_outgoing.insert(forwardId, outgoing);

    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.handleTaskUpdated({{"forwardId", forwardId}, {"sourceDeviceId", "source"},
                              {"channel", "LAN"}, {"state", "TRANSFERRING"}});
    QTimer::singleShot(1300, &receiver, [&receiver, forwardId, token, hash, content]() {
        receiver.prepareIncoming({
            {"forwardId", forwardId}, {"sourceDeviceId", "source"},
            {"targetDeviceId", "target"}, {"channel", "LAN"},
            {"directTransferToken", token}, {"destinationPath", "Downloads"},
            {"files", QVariantList{QVariantMap{{"path", "late.bin"},
                                                 {"size", content.size()}, {"sha256", hash}}}}
        });
    });

    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    QCOMPARE(fallback.count(), 0);
    QVERIFY(sender.lastError().contains("retrying"));
}

void LanTransferManagerTest::transfersFolderWithNestedAndEmptyDirectories()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString sourceRoot = QDir(temporary.path()).filePath("Album");
    QVERIFY(QDir().mkpath(QDir(sourceRoot).filePath("nested/empty")));
    const QByteArray firstContent("first-file-content");
    const QByteArray secondContent(700000, 'z');
    const QString firstPath = QDir(sourceRoot).filePath("first.txt");
    const QString secondPath = QDir(sourceRoot).filePath("nested/second.bin");
    for (const auto &entry : QList<QPair<QString, QByteArray>>{{firstPath, firstContent},
                                                               {secondPath, secondContent}}) {
        QFile file(entry.first);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(entry.second), entry.second.size());
    }
    const auto hash = [](const QByteArray &value) {
        return QString::fromLatin1(QCryptographicHash::hash(value, QCryptographicHash::Sha256).toHex());
    };
    const QString receiveRoot = QDir(temporary.path()).filePath("receive");
    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(receiveRoot);
    const QString forwardId = "folder-forward";
    const QString token = "folder-capability-token";
    const QVariantList manifest{
        QVariantMap{{"path", "Album/first.txt"}, {"size", firstContent.size()}, {"sha256", hash(firstContent)}},
        QVariantMap{{"path", "Album/nested/second.bin"}, {"size", secondContent.size()}, {"sha256", hash(secondContent)}}};
    receiver.prepareIncoming(QVariantMap{
        {"forwardId", forwardId}, {"sourceDeviceId", "source"}, {"targetDeviceId", "target"},
        {"channel", "LAN"}, {"directTransferToken", token}, {"destinationPath", "Downloads"},
        {"directories", QVariantList{"Album", "Album/nested", "Album/nested/empty"}},
        {"files", manifest}});

    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = forwardId;
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourceRoot;
    outgoing.directory = true;
    outgoing.token = token;
    outgoing.totalSize = firstContent.size() + secondContent.size();
    outgoing.files = {
        LanTransferManager::TransferFile{"Album/first.txt", firstPath, hash(firstContent), firstContent.size()},
        LanTransferManager::TransferFile{"Album/nested/second.bin", secondPath, hash(secondContent), secondContent.size()}};
    sender.m_outgoing.insert(forwardId, outgoing);

    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    QSignalSpy failed(&receiver, &LanTransferManager::incomingFailed);
    sender.handleTaskUpdated(QVariantMap{{"forwardId", forwardId}, {"sourceDeviceId", "source"},
                                         {"channel", "LAN"}, {"state", "TRANSFERRING"}});
    QVERIFY2(completed.wait(10000), qPrintable(receiver.lastError()));
    QCOMPARE(failed.count(), 0);
    QCOMPARE(completed.first().at(1).toLongLong(), qint64(firstContent.size() + secondContent.size()));
    QVERIFY(QDir(QDir(receiveRoot).filePath("Downloads/Album/nested/empty")).exists());
    QFile first(QDir(receiveRoot).filePath("Downloads/Album/first.txt"));
    QFile second(QDir(receiveRoot).filePath("Downloads/Album/nested/second.bin"));
    QVERIFY(first.open(QIODevice::ReadOnly));
    QVERIFY(second.open(QIODevice::ReadOnly));
    QCOMPARE(first.readAll(), firstContent);
    QCOMPARE(second.readAll(), secondContent);
}

void LanTransferManagerTest::rejectsWrongCapabilityToken()
{
    QTemporaryDir temporary;
    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(temporary.path());
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(QByteArray("x"), QCryptographicHash::Sha256).toHex());
    receiver.prepareIncoming(QVariantMap{
        {"forwardId", "auth-forward"}, {"targetDeviceId", "target"}, {"channel", "LAN"},
        {"directTransferToken", "correct-token"}, {"destinationPath", "Downloads"},
        {"files", QVariantList{QVariantMap{{"path", "x.txt"}, {"size", 1}, {"sha256", hash}}}}});
    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", receiver.listenPort());
    QVERIFY(socket.waitForConnected(3000));
    socket.write(LanTransferManager::frame(QJsonObject{
        {"protocol", "MYFOLDER_LAN_V2"}, {"forwardId", "auth-forward"},
        {"token", "wrong-token"}, {"filePath", "x.txt"}, {"size", 1}, {"sha256", hash}}));
    QSignalSpy readyRead(&socket, &QTcpSocket::readyRead);
    QVERIFY(readyRead.wait(3000));
    QByteArray response = socket.readAll();
    qint64 length = -1;
    QJsonObject object;
    QVERIFY(LanTransferManager::takeFrame(response, length, &object));
    QVERIFY(!object.value("ok").toBool());
    QVERIFY(!QFile::exists(QDir(temporary.path()).filePath("Downloads/x.txt")));
}

void LanTransferManagerTest::rejectsFileWhoseFinalHashDoesNotMatchManifest()
{
    QTemporaryDir temporary;
    const QByteArray actual("tampered-content");
    const QString expectedHash = QString::fromLatin1(
        QCryptographicHash::hash(QByteArray("expected-content"), QCryptographicHash::Sha256).toHex());
    const QString sourcePath = QDir(temporary.path()).filePath("source.bin");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write(actual);
    source.close();
    const QString receiveRoot = QDir(temporary.path()).filePath("receive");
    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(receiveRoot);
    receiver.prepareIncoming(QVariantMap{
        {"forwardId", "hash-forward"}, {"targetDeviceId", "target"}, {"channel", "LAN"},
        {"directTransferToken", "hash-token"}, {"destinationPath", "Downloads"},
        {"files", QVariantList{QVariantMap{{"path", "source.bin"},
                                            {"size", actual.size()}, {"sha256", expectedHash}}}}});
    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = "hash-forward";
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = "hash-token";
    outgoing.totalSize = actual.size();
    outgoing.files.append(LanTransferManager::TransferFile{
        "source.bin", sourcePath, expectedHash, actual.size()});
    sender.m_outgoing.insert(outgoing.forwardId, outgoing);
    QSignalSpy failed(&receiver, &LanTransferManager::incomingFailed);
    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    sender.handleTaskUpdated(QVariantMap{{"forwardId", outgoing.forwardId}, {"sourceDeviceId", "source"},
                                         {"channel", "LAN"}, {"state", "TRANSFERRING"}});
    QVERIFY(failed.wait(5000));
    QCOMPARE(failed.first().at(1).toString(), QString("LAN_SHA256_MISMATCH"));
    QCOMPARE(completed.count(), 0);
    QVERIFY(!QFile::exists(QDir(receiveRoot).filePath("Downloads/source.bin")));
    QVERIFY(!QFile::exists(QDir(receiveRoot).filePath("Downloads/source.bin.myfolder-part")));
}

void LanTransferManagerTest::fallsBackWhenEndpointCannotBeReached()
{
    QTemporaryDir temporary;
    const QString sourcePath = QDir(temporary.path()).filePath("source.txt");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("fallback");
    source.close();
    QTcpServer reservation;
    QVERIFY(reservation.listen(QHostAddress::LocalHost, 0));
    const int unavailablePort = reservation.serverPort();
    reservation.close();

    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = "fallback-forward";
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.targetPort = unavailablePort;
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = "fallback-token";
    outgoing.totalSize = 8;
    outgoing.files.append(LanTransferManager::TransferFile{
        "source.txt", sourcePath,
        QString::fromLatin1(QCryptographicHash::hash(QByteArray("fallback"), QCryptographicHash::Sha256).toHex()), 8});
    sender.m_outgoing.insert(outgoing.forwardId, outgoing);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.handleTaskUpdated(QVariantMap{{"forwardId", outgoing.forwardId}, {"sourceDeviceId", "source"},
                                         {"channel", "LAN"}, {"state", "TRANSFERRING"}});
    // The production retry policy permits five 4-second connection attempts
    // plus incremental backoff before falling back to P2P/Relay.
    QVERIFY(fallback.wait(30000));
    QCOMPARE(fallback.first().at(1).toString(), QString("target"));
    QCOMPARE(fallback.first().at(3).toString(), sourcePath);
}

void LanTransferManagerTest::fallsBackWhenControlTaskCreationConnectionCloses()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString sourcePath = QDir(temporary.path()).filePath("source.txt");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write("control-fallback"), qint64(16));
    source.close();

    QTcpServer api;
    QVERIFY(api.listen(QHostAddress::LocalHost, 0));
    connect(&api, &QTcpServer::newConnection, &api, [&api] {
        while (QTcpSocket *socket = api.nextPendingConnection()) {
            socket->abort();
            socket->deleteLater();
        }
    });

    LanTransferManager sender;
    sender.setBaseUrl(QString("http://127.0.0.1:%1").arg(api.serverPort()));
    sender.setAuthToken("Bearer test-token");
    sender.setCurrentDeviceId("source");
    sender.setCurrentDeviceToken("device-token");

    QSignalSpy failed(&sender, &LanTransferManager::createFailed);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.sendPath("target", "127.0.0.1", 43210, "Downloads", sourcePath);

    QVERIFY(fallback.wait(5000));
    QCOMPARE(failed.count(), 1);
    QCOMPARE(fallback.count(), 1);
    const QList<QVariant> arguments = fallback.first();
    QCOMPARE(arguments.at(0).toString(), QString());
    QCOMPARE(arguments.at(1).toString(), QString("target"));
    QCOMPARE(arguments.at(2).toString(), QString("Downloads"));
    QCOMPARE(arguments.at(3).toString(), sourcePath);
    QCOMPARE(arguments.at(4).toBool(), false);
    QVERIFY(!arguments.at(5).toString().isEmpty());
}

void LanTransferManagerTest::fallsBackWhenControlTaskHasNoCapability()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString sourcePath = QDir(temporary.path()).filePath("source.txt");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write("missing-capability"), qint64(18));
    source.close();

    QTcpServer api;
    QVERIFY(api.listen(QHostAddress::LocalHost, 0));
    connect(&api, &QTcpServer::newConnection, &api, [&api] {
        while (QTcpSocket *socket = api.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                const QByteArray request = socket->readAll();
                if (!request.contains("\r\n\r\n")) return;
                const QByteArray body = R"({"forwardId":"lan-without-token"})";
                socket->write("HTTP/1.1 201 Created\r\nContent-Type: application/json\r\n"
                              "Content-Length: " + QByteArray::number(body.size()) +
                              "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });

    LanTransferManager sender;
    sender.setBaseUrl(QString("http://127.0.0.1:%1").arg(api.serverPort()));
    sender.setAuthToken("Bearer test-token");
    sender.setCurrentDeviceId("source");
    sender.setCurrentDeviceToken("device-token");

    QSignalSpy failed(&sender, &LanTransferManager::createFailed);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.sendPath("target", "127.0.0.1", 43210, "Downloads", sourcePath);

    QVERIFY(fallback.wait(5000));
    QCOMPARE(failed.count(), 1);
    QCOMPARE(fallback.count(), 1);
    const QList<QVariant> arguments = fallback.first();
    QCOMPARE(arguments.at(0).toString(), QString());
    QCOMPARE(arguments.at(1).toString(), QString("target"));
    QCOMPARE(arguments.at(2).toString(), QString("Downloads"));
    QCOMPARE(arguments.at(3).toString(), sourcePath);
    QCOMPARE(arguments.at(4).toBool(), false);
    QVERIFY(arguments.at(5).toString().contains("capability", Qt::CaseInsensitive));
}

void LanTransferManagerTest::restoresOutgoingContextAfterRestart()
{
    QTemporaryDir temporary;
    const QString sourcePath = QDir(temporary.path()).filePath("persistent.txt");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write("persisted");
    source.close();
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(QByteArray("persisted"), QCryptographicHash::Sha256).toHex());
    {
        LanTransferManager manager;
        LanTransferManager::Outgoing outgoing;
        outgoing.forwardId = "persistent-forward";
        outgoing.targetDeviceId = "target";
        outgoing.targetAddress = "192.168.1.20";
        outgoing.sourceAddress = "192.168.1.10";
        outgoing.targetPort = 45678;
        outgoing.destinationPath = "Downloads";
        outgoing.localRootPath = sourcePath;
        outgoing.token = "persistent-token";
        outgoing.totalSize = 9;
        outgoing.files.append(LanTransferManager::TransferFile{
            "persistent.txt", sourcePath, hash, 9});
        manager.m_outgoing.insert(outgoing.forwardId, outgoing);
        manager.saveOutgoing();
    }
    LanTransferManager restored;
    QVERIFY(restored.m_outgoing.contains("persistent-forward"));
    const auto outgoing = restored.m_outgoing.value("persistent-forward");
    QCOMPARE(outgoing.targetAddress, QString("192.168.1.20"));
    QCOMPARE(outgoing.sourceAddress, QString("192.168.1.10"));
    QCOMPARE(outgoing.files.size(), 1);
    QCOMPARE(outgoing.files.first().localPath, sourcePath);
    QVERIFY(!outgoing.started);
}

void LanTransferManagerTest::fallsBackToSystemRouteWhenSourceBindFails()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QByteArray content("bind-fallback-content");
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    const QString sourcePath = QDir(temporary.path()).filePath("bind.txt");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(content), qint64(content.size()));
    source.close();

    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(QDir(temporary.path()).filePath("receive"));
    receiver.prepareIncoming({{"forwardId", "bind-forward"}, {"targetDeviceId", "target"},
                              {"channel", "LAN"}, {"directTransferToken", "bind-token"},
                              {"destinationPath", "Downloads"},
                              {"files", QVariantList{QVariantMap{{"path", "bind.txt"},
                                  {"size", content.size()}, {"sha256", hash}}}}});

    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = "bind-forward";
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.sourceAddress = "203.0.113.10";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = "bind-token";
    outgoing.totalSize = content.size();
    outgoing.files.append({"bind.txt", sourcePath, hash, content.size()});
    sender.m_outgoing.insert(outgoing.forwardId, outgoing);

    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.handleTaskUpdated({{"forwardId", outgoing.forwardId}, {"sourceDeviceId", "source"},
                              {"channel", "LAN"}, {"state", "TRANSFERRING"}});
    QVERIFY(completed.wait(5000));
    QCOMPARE(fallback.count(), 0);
    QVERIFY(sender.m_outgoing.value(outgoing.forwardId).sourceAddress.isEmpty());
}

void LanTransferManagerTest::bypassesApplicationProxyForLanSocket()
{
    struct ProxyRestore {
        QNetworkProxy value;
        ~ProxyRestore() { QNetworkProxy::setApplicationProxy(value); }
    } restore{QNetworkProxy::applicationProxy()};

    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QByteArray content(64 * 1024, 'n');
    const QString contentHash = QString::fromLatin1(
        QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    const QString sourcePath = QDir(temporary.path()).filePath("no-proxy.bin");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(content), content.size());
    source.close();

    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(QDir(temporary.path()).filePath("receive"));
    receiver.prepareIncoming({{"forwardId", "no-proxy-forward"}, {"targetDeviceId", "target"},
                              {"channel", "LAN"}, {"directTransferToken", "no-proxy-token"},
                              {"destinationPath", "Downloads"},
                              {"files", QVariantList{QVariantMap{{"path", "no-proxy.bin"},
                                  {"size", content.size()}, {"sha256", contentHash}}}}});
    QVERIFY(receiver.listenPort() > 0);

    // Keep the receiver setup independent from the proxy under test. The
    // regression is specifically that an outgoing LAN data socket must ignore
    // an application-wide HTTP proxy and still connect directly.
    QNetworkProxy::setApplicationProxy(
        QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), 9));

    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = "no-proxy-forward";
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.sourceAddress = "127.0.0.1";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = "no-proxy-token";
    outgoing.totalSize = content.size();
    outgoing.files.append({"no-proxy.bin", sourcePath, contentHash, content.size()});
    sender.m_outgoing.insert(outgoing.forwardId, outgoing);

    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);
    sender.handleTaskUpdated({{"forwardId", outgoing.forwardId}, {"sourceDeviceId", "source"},
                              {"channel", "LAN"}, {"state", "TRANSFERRING"}});

    QVERIFY2(completed.wait(5000), qPrintable(sender.lastError()));
    QCOMPARE(fallback.count(), 0);
}

void LanTransferManagerTest::senderUsesReceiverAcknowledgedProgress()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QByteArray content(10 * 1024 * 1024, 'p');
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    const QString sourcePath = QDir(temporary.path()).filePath("progress.bin");
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(content), qint64(content.size()));
    source.close();

    LanTransferManager receiver;
    receiver.setCurrentDeviceId("target");
    receiver.setReceiveRoot(QDir(temporary.path()).filePath("receive"));
    receiver.prepareIncoming({{"forwardId", "progress-forward"}, {"targetDeviceId", "target"},
                              {"channel", "LAN"}, {"directTransferToken", "progress-token"},
                              {"destinationPath", "Downloads"},
                              {"files", QVariantList{QVariantMap{{"path", "progress.bin"},
                                  {"size", content.size()}, {"sha256", hash}}}}});

    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = "progress-forward";
    outgoing.targetDeviceId = "target";
    outgoing.targetAddress = "127.0.0.1";
    outgoing.targetPort = receiver.listenPort();
    outgoing.destinationPath = "Downloads";
    outgoing.localRootPath = sourcePath;
    outgoing.token = "progress-token";
    outgoing.totalSize = content.size();
    outgoing.files.append({"progress.bin", sourcePath, hash, content.size()});
    sender.m_outgoing.insert(outgoing.forwardId, outgoing);

    QSignalSpy receiverProgress(&receiver, &LanTransferManager::incomingProgress);
    QSignalSpy senderProgress(&sender, &LanTransferManager::outgoingProgressReady);
    QSignalSpy completed(&receiver, &LanTransferManager::incomingCompleted);
    sender.handleTaskUpdated({{"forwardId", outgoing.forwardId}, {"sourceDeviceId", "source"},
                              {"channel", "LAN"}, {"state", "TRANSFERRING"}});
    QVERIFY(completed.wait(10000));
    QTRY_VERIFY_WITH_TIMEOUT(senderProgress.count() > 0, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(sender.outgoingProgress().value(outgoing.forwardId).toLongLong(),
                              qint64(content.size()), 3000);
    QCOMPARE(receiverProgress.last().at(1).toLongLong(), senderProgress.last().at(1).toLongLong());
}

void LanTransferManagerTest::rejectionCleansOutgoingWithoutFallback()
{
    LanTransferManager sender;
    sender.setCurrentDeviceId("source");
    LanTransferManager::Outgoing outgoing;
    outgoing.forwardId = "rejected-lan";
    outgoing.targetDeviceId = "target";
    sender.m_outgoing.insert(outgoing.forwardId, outgoing);
    QSignalSpy fallback(&sender, &LanTransferManager::outgoingFallbackRequested);

    sender.handleTaskUpdated({{"forwardId", outgoing.forwardId}, {"sourceDeviceId", "source"},
                              {"channel", "LAN"}, {"state", "REJECTED"}});

    QVERIFY(!sender.m_outgoing.contains(outgoing.forwardId));
    QCOMPARE(fallback.count(), 0);
}

QTEST_MAIN(LanTransferManagerTest)
#include "lantransfermanager_test.moc"
