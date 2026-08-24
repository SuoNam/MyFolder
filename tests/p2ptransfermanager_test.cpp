#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include "../p2ptransfermanager.h"

class P2pTransferManagerTest : public QObject
{
    Q_OBJECT
private slots:
    void fallsBackWhenParametersAreInvalid();
    void queuesCandidateUntilRemoteDescription();
    void transfersFolderOverEncryptedDataChannel();
    void rejectsContentWithWrongManifestHash();
    void fallsBackWhenAcknowledgementStalls();
    void retriesTemporarySignalingDisconnects();
    void rejectionClosesSessionWithoutFallback();
};

void P2pTransferManagerTest::fallsBackWhenParametersAreInvalid()
{
    P2pTransferManager manager;
    QSignalSpy fallback(&manager, &P2pTransferManager::outgoingFallbackRequested);
    manager.sendPath("target", "Downloads", "/path/that/does/not/exist");
    QCOMPARE(fallback.count(), 1);
    QCOMPARE(fallback.first().at(1).toString(), QString("target"));
    QCOMPARE(fallback.first().at(2).toString(), QString("Downloads"));
}

void P2pTransferManagerTest::queuesCandidateUntilRemoteDescription()
{
    P2pTransferManager manager;
    manager.m_testSignaling = true;
    manager.setCurrentDeviceId("target");
    auto session = std::make_shared<P2pTransferManager::Session>();
    session->forwardId = "candidate-first";
    session->source = false;
    manager.m_sessions.insert(session->forwardId, session);
    QSignalSpy failed(&manager, &P2pTransferManager::incomingFailed);

    manager.handleSignal({{"forwardId", session->forwardId}, {"fromDeviceId", "source"},
                          {"signal", QJsonObject{{"kind", "candidate"},
                                                  {"candidate", "candidate:1 1 UDP 1 127.0.0.1 5000 typ host"},
                                                  {"mid", "0"}}}});

    QCOMPARE(failed.count(), 0);
    QVERIFY(manager.m_sessions.contains(session->forwardId));
    QCOMPARE(session->pendingRemoteCandidates.size(), 1);
    QCOMPARE(session->pendingRemoteCandidates.constFirst().second, QString("0"));
}

static QVariantMap taskFor(const QString &id, const QString &hash, qint64 size)
{
    return {{"forwardId",id},{"channel","P2P"},{"state","ACCEPTED"},
            {"sourceDeviceId","source"},{"targetDeviceId","target"},
            {"destinationPath","Downloads"},{"directTransferToken","secret-token"},
            {"totalBytes",size},
            {"directories",QVariantList{"Album","Album/empty"}},
            {"files",QVariantList{QVariantMap{{"path","Album/data.bin"},{"size",size},{"sha256",hash}}}}};
}

static void wire(P2pTransferManager &source, P2pTransferManager &target)
{
    QObject::connect(&source,&P2pTransferManager::localSignalReady,&target,
        [&target](const QString &id,const QJsonObject &signal){target.handleSignal({{"forwardId",id},{"fromDeviceId","source"},{"signal",signal}});},
        Qt::QueuedConnection);
    QObject::connect(&target,&P2pTransferManager::localSignalReady,&source,
        [&source](const QString &id,const QJsonObject &signal){source.handleSignal({{"forwardId",id},{"fromDeviceId","target"},{"signal",signal}});},
        Qt::QueuedConnection);
}

void P2pTransferManagerTest::transfersFolderOverEncryptedDataChannel()
{
    QTemporaryDir sourceDir, targetDir; QVERIFY(sourceDir.isValid()&&targetDir.isValid());
    QDir(sourceDir.path()).mkpath("Album/empty");
    QByteArray content(400000,'x'); QFile file(QDir(sourceDir.path()).filePath("Album/data.bin"));
    QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(content),content.size()); file.close();
    QString hash=P2pTransferManager::sha256File(file.fileName()); QString id="p2p-folder";
    P2pTransferManager source,target; source.m_testSignaling=target.m_testSignaling=true;
    source.setCurrentDeviceId("source");target.setCurrentDeviceId("target");target.setReceiveRoot(targetDir.path());wire(source,target);
    auto session=std::make_shared<P2pTransferManager::Session>();session->forwardId=id;session->source=true;
    session->sourceDeviceId="source";session->targetDeviceId="target";session->destinationPath="Downloads";
    session->token="secret-token";session->localRootPath=QDir(sourceDir.path()).filePath("Album");session->directory=true;
    session->files.append({"Album/data.bin",file.fileName(),hash,content.size()});session->directories={"Album","Album/empty"};session->totalBytes=content.size();source.m_sessions.insert(id,session);
    QVariantMap task=taskFor(id,hash,content.size());target.prepareIncoming(task);
    QSignalSpy completed(&target,&P2pTransferManager::incomingCompleted);QSignalSpy failed(&target,&P2pTransferManager::incomingFailed);QSignalSpy outgoingProgress(&source,&P2pTransferManager::outgoingProgressReady);
    target.handleTaskUpdated(task);
    source.handleTaskUpdated(task);
    QCOMPARE(outgoingProgress.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(),1,20000);QCOMPARE(failed.count(),0);
    QFile received(QDir(targetDir.path()).filePath("Downloads/Album/data.bin"));QVERIFY(received.open(QIODevice::ReadOnly));QCOMPARE(received.readAll(),content);
    QVERIFY(QFileInfo(QDir(targetDir.path()).filePath("Downloads/Album/empty")).isDir());
    QVERIFY(outgoingProgress.count()>0);
    QCOMPARE(outgoingProgress.last().at(1).toLongLong(),qint64(content.size()));
    QCOMPARE(source.outgoingProgress().value(id).toLongLong(),qint64(content.size()));
    QCOMPARE(target.incomingVerifiedProgress().value(id).toLongLong(),qint64(content.size()));
}

void P2pTransferManagerTest::rejectsContentWithWrongManifestHash()
{
    QTemporaryDir sourceDir,targetDir;QVERIFY(sourceDir.isValid()&&targetDir.isValid());QByteArray content="actual";
    QString path=QDir(sourceDir.path()).filePath("data.bin");QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write(content);f.close();
    QString wrong(64,'0'),id="p2p-bad-hash";P2pTransferManager source,target;source.m_testSignaling=target.m_testSignaling=true;
    source.setCurrentDeviceId("source");target.setCurrentDeviceId("target");target.setReceiveRoot(targetDir.path());wire(source,target);
    auto session=std::make_shared<P2pTransferManager::Session>();session->forwardId=id;session->source=true;session->sourceDeviceId="source";session->targetDeviceId="target";session->destinationPath="Downloads";session->token="secret-token";session->localRootPath=path;session->files.append({"Album/data.bin",path,wrong,content.size()});session->totalBytes=content.size();source.m_sessions.insert(id,session);
    QVariantMap task=taskFor(id,wrong,content.size());target.prepareIncoming(task);QSignalSpy failed(&target,&P2pTransferManager::incomingFailed);
    target.handleTaskUpdated(task);source.handleTaskUpdated(task);QTRY_COMPARE_WITH_TIMEOUT(failed.count(),1,20000);
    QCOMPARE(failed.at(0).at(1).toString(),QString("P2P_HASH_MISMATCH"));
}

void P2pTransferManagerTest::fallsBackWhenAcknowledgementStalls()
{
    P2pTransferManager manager;
    manager.m_watchdog.stop();
    auto session=std::make_shared<P2pTransferManager::Session>();
    session->forwardId="p2p-ack-timeout";session->source=true;session->targetDeviceId="target";
    session->destinationPath="Downloads";session->localRootPath="/tmp/source.bin";
    session->awaitingAck=true;session->lastActivityMs=1000;
    manager.m_sessions.insert(session->forwardId,session);
    QSignalSpy fallback(&manager,&P2pTransferManager::outgoingFallbackRequested);

    manager.checkTimeouts(1000+30001);

    QCOMPARE(fallback.count(),1);
    QVERIFY(!manager.m_sessions.contains(session->forwardId));
    QVERIFY(fallback.first().at(5).toString().contains("acknowledgement",Qt::CaseInsensitive));
}

void P2pTransferManagerTest::rejectionClosesSessionWithoutFallback()
{
    P2pTransferManager manager;
    manager.setCurrentDeviceId("source");
    auto session=std::make_shared<P2pTransferManager::Session>();
    session->forwardId="rejected-p2p";session->source=true;
    session->sourceDeviceId="source";session->targetDeviceId="target";
    manager.m_sessions.insert(session->forwardId,session);
    QSignalSpy fallback(&manager,&P2pTransferManager::outgoingFallbackRequested);

    manager.handleTaskUpdated({{"forwardId",session->forwardId},{"channel","P2P"},
                               {"sourceDeviceId","source"},{"state","REJECTED"}});

    QVERIFY(!manager.m_sessions.contains(session->forwardId));
    QCOMPARE(fallback.count(),0);
}

void P2pTransferManagerTest::retriesTemporarySignalingDisconnects()
{
    QTcpServer api;
    QVERIFY(api.listen(QHostAddress::LocalHost, 0));
    int requests = 0;
    connect(&api, &QTcpServer::newConnection, &api, [&api, &requests] {
        while (QTcpSocket *socket = api.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, socket, [socket, &requests] {
                if (!socket->readAll().contains("\r\n\r\n")) return;
                ++requests;
                if (requests < 3) {
                    socket->abort();
                    return;
                }
                socket->write("HTTP/1.1 202 Accepted\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                socket->disconnectFromHost();
            });
        }
    });

    P2pTransferManager manager;
    manager.setBaseUrl(QString("http://127.0.0.1:%1").arg(api.serverPort()));
    manager.setAuthToken("Bearer test-token");
    manager.setCurrentDeviceId("source");
    manager.setCurrentDeviceToken("device-token");
    auto session = std::make_shared<P2pTransferManager::Session>();
    session->forwardId = "signal-retry";
    session->source = true;
    session->targetDeviceId = "target";
    session->destinationPath = "Downloads";
    manager.m_sessions.insert(session->forwardId, session);
    QSignalSpy fallback(&manager, &P2pTransferManager::outgoingFallbackRequested);

    manager.sendSignal(session->forwardId,
                       {{"kind", "candidate"}, {"candidate", "candidate-data"}, {"mid", "0"}});

    QTRY_COMPARE_WITH_TIMEOUT(requests, 3, 5000);
    QTest::qWait(300);
    QCOMPARE(fallback.count(), 0);
    QVERIFY(manager.m_sessions.contains(session->forwardId));
}

QTEST_MAIN(P2pTransferManagerTest)
#include "p2ptransfermanager_test.moc"
