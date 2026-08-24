#include "../forwardmanager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

class ForwardManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void persistsAcceptedFallbackChain();
    void persistsDismissedTask();
    void coalescesProgressWhileRequestIsInFlight();
    void tracksAutomaticAcceptanceAndHidesSupersededTask();
    void staleListPreservesTaskCreatedAfterRequestStarted();
    void olderOverlappingListResponseIsIgnored();
};

void ForwardManagerTest::initTestCase()
{
    QCoreApplication::setOrganizationName("MyFolderTest");
    QCoreApplication::setApplicationName("ForwardManagerTest");
}

void ForwardManagerTest::init()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings().clear();
}

static QVariantMap transfer(const QString &channel, const QString &hash = QString(64, 'a'))
{
    QVariantList directories;
    QString path = "report.txt";
    if (channel != "RELAY") {
        directories = QVariantList{"Folder"};
        path = "Folder/report.txt";
    }
    return {{"sourceDeviceId", "source"}, {"targetDeviceId", "target"},
            {"destinationPath", "Downloads"}, {"channel", channel}, {"totalBytes", 7},
            {"directories", directories},
            {"files", QVariantList{QVariantMap{{"path", path}, {"size", 7}, {"sha256", hash}}}}};
}

void ForwardManagerTest::persistsAcceptedFallbackChain()
{
    ForwardManager manager;
    QVERIFY(!manager.canAutoAcceptFallback(transfer("LAN")));
    manager.rememberAcceptedTransfer(transfer("LAN"));
    QVERIFY(!manager.canAutoAcceptFallback(transfer("LAN")));
    QVERIFY(manager.canAutoAcceptFallback(transfer("P2P")));
    QVERIFY(!manager.canAutoAcceptFallback(transfer("P2P", QString(64, 'b'))));

    ForwardManager restored;
    QVERIFY(restored.canAutoAcceptFallback(transfer("P2P")));
    restored.rememberAcceptedTransfer(transfer("P2P"));
    QVERIFY(restored.canAutoAcceptFallback(transfer("RELAY")));
    restored.forgetAcceptedTransfer(transfer("RELAY"));
    QVERIFY(!restored.canAutoAcceptFallback(transfer("RELAY")));
}

void ForwardManagerTest::persistsDismissedTask()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    int requestCount = 0;
    connect(&server, &QTcpServer::newConnection, &server, [&server, &requestCount]() {
        while (QTcpSocket *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, &requestCount]() {
                socket->readAll();
                ++requestCount;
                const QByteArray body = R"([{"forwardId":"failed-1","state":"FAILED","updatedAt":"2026-08-19T00:00:00Z"}])";
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
                              QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });

    const QString baseUrl = QString("http://127.0.0.1:%1").arg(server.serverPort());
    ForwardManager manager;
    manager.setBaseUrl(baseUrl);
    manager.setAuthToken("jwt");
    manager.setCurrentDeviceId("device");
    manager.setCurrentDeviceToken("device-token");
    manager.refreshTasks();
    QTRY_COMPARE(manager.tasks().size(), 1);

    manager.dismissTask("failed-1");
    QCOMPARE(manager.tasks().size(), 0);
    QVERIFY(QSettings().value("ForwardV11/dismissedTaskIds").toStringList().contains("failed-1"));

    ForwardManager restored;
    restored.setBaseUrl(baseUrl);
    restored.setAuthToken("jwt");
    restored.setCurrentDeviceId("device");
    restored.setCurrentDeviceToken("device-token");
    restored.refreshTasks();
    QTRY_COMPARE(requestCount, 2);
    QTRY_VERIFY(!restored.busy());
    QCOMPARE(restored.tasks().size(), 0);
}

void ForwardManagerTest::coalescesProgressWhileRequestIsInFlight()
{
    ForwardManager manager;
    manager.setBaseUrl("http://127.0.0.1:9");
    manager.setAuthToken("jwt");
    manager.setCurrentDeviceId("device");
    manager.setCurrentDeviceToken("device-token");

    manager.reportProgress("forward-1", 100);
    manager.reportProgress("forward-1", 200);
    manager.reportProgress("forward-1", 150);

    QVERIFY(manager.m_progressInFlight.contains("forward-1"));
    QCOMPARE(manager.m_pendingProgress.size(), 1);
    QCOMPARE(manager.m_pendingProgress.value("forward-1"), 200);

    manager.finishDownload("forward-1", 300);
    QCOMPARE(manager.m_pendingFinalProgress.value("forward-1"), 300);
}

void ForwardManagerTest::tracksAutomaticAcceptanceAndHidesSupersededTask()
{
    ForwardManager manager;
    QJsonObject task = QJsonObject::fromVariantMap(transfer("LAN"));
    task["forwardId"] = "old-lan";
    task["state"] = "OFFERED";
    manager.m_tasks.insert("old-lan", task);

    manager.markAutoAccepting("old-lan");
    QVERIFY(manager.isAutoAccepting("old-lan"));
    manager.hideSupersededTask("old-lan");

    QVERIFY(!manager.isAutoAccepting("old-lan"));
    QCOMPARE(manager.tasks().size(), 0);
    QVERIFY(QSettings().value("ForwardV11/dismissedTaskIds").toStringList().contains("old-lan"));
}

void ForwardManagerTest::staleListPreservesTaskCreatedAfterRequestStarted()
{
    ForwardManager manager;
    const quint64 sequence = ++manager.m_latestListRequestSequence;
    const quint64 baseline = manager.m_taskRevision;

    QJsonObject task = QJsonObject::fromVariantMap(transfer("LAN"));
    task["forwardId"] = "new-lan";
    task["state"] = "ACCEPTED";
    manager.updateTask(task);

    QSignalSpy reconciled(&manager, &ForwardManager::tasksReconciled);
    manager.applyTaskList(QJsonArray{}, sequence, baseline);

    QCOMPARE(manager.tasks().size(), 1);
    QCOMPARE(manager.tasks().constFirst().toMap().value("forwardId").toString(), QString("new-lan"));
    QCOMPARE(reconciled.size(), 1);
    QVERIFY(reconciled.constFirst().constFirst().toStringList().contains("new-lan"));
}

void ForwardManagerTest::olderOverlappingListResponseIsIgnored()
{
    ForwardManager manager;
    const quint64 olderSequence = ++manager.m_latestListRequestSequence;
    const quint64 baseline = manager.m_taskRevision;
    const quint64 newerSequence = ++manager.m_latestListRequestSequence;

    QJsonObject current = QJsonObject::fromVariantMap(transfer("LAN"));
    current["forwardId"] = "current";
    current["state"] = "OFFERED";
    manager.updateTask(current);

    QJsonObject stale = current;
    stale["forwardId"] = "stale";
    QSignalSpy reconciled(&manager, &ForwardManager::tasksReconciled);
    manager.applyTaskList(QJsonArray{stale}, olderSequence, baseline);
    QCOMPARE(reconciled.size(), 0);
    QCOMPARE(manager.tasks().size(), 1);
    QCOMPARE(manager.tasks().constFirst().toMap().value("forwardId").toString(), QString("current"));

    manager.applyTaskList(QJsonArray{current}, newerSequence, manager.m_taskRevision);
    QCOMPARE(reconciled.size(), 1);
}

QTEST_MAIN(ForwardManagerTest)
#include "forwardmanager_test.moc"
