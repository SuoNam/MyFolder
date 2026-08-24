#include "../relaydownloadmanager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class RelayDownloadManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void downloadsAndVerifiesRelayContent();
};

void RelayDownloadManagerTest::downloadsAndVerifiesRelayContent()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    qputenv("MYFOLDER_DOWNLOAD_STATE_DIR", QDir(temporary.path()).filePath("state").toUtf8());

    QByteArray content(9 * 1024 * 1024 + 37, Qt::Uninitialized);
    for (qsizetype index = 0; index < content.size(); ++index) content[index] = char(index % 251);
    const QString sha256 = QString::fromLatin1(
        QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    QByteArray requestBytes;
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (QTcpSocket *socket = server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                requestBytes += socket->readAll();
                if (!requestBytes.contains("\r\n\r\n")) return;
                const QByteArray response =
                    "HTTP/1.1 206 Partial Content\r\n"
                    "Accept-Ranges: bytes\r\n"
                    "Content-Range: bytes 0-" + QByteArray::number(content.size() - 1) +
                    "/" + QByteArray::number(content.size()) + "\r\n"
                    "Content-Length: " + QByteArray::number(content.size()) + "\r\n"
                    "X-File-SHA256: " + sha256.toUtf8() + "\r\n"
                    "Connection: close\r\n\r\n" + content;
                socket->write(response);
                socket->disconnectFromHost();
            });
        }
    });

    RelayDownloadManager manager;
    manager.setBaseUrl(QString("http://127.0.0.1:%1").arg(server.serverPort()));
    manager.setAuthToken("Bearer test-token");
    manager.setCurrentDeviceId("target");
    manager.setCurrentDeviceToken("device-token");
    manager.setReceiveRoot(QDir(temporary.path()).filePath("receive"));

    QSignalSpy completed(&manager, &RelayDownloadManager::taskDownloadCompleted);
    QSignalSpy failed(&manager, &RelayDownloadManager::taskDownloadFailed);
    QSignalSpy progress(&manager, &RelayDownloadManager::progressReady);
    const QVariantMap task{
        {"forwardId", "relay-forward"}, {"sourceDeviceId", "source"},
        {"targetDeviceId", "target"}, {"channel", "RELAY"},
        {"state", "TRANSFERRING"}, {"destinationPath", "Downloads"},
        {"totalBytes", content.size()},
        {"files", QVariantList{QVariantMap{{"path", "relay.txt"},
                                             {"size", content.size()}, {"sha256", sha256}}}}
    };
    QVERIFY(manager.startTask(task));
    QVERIFY2(completed.wait(5000), qPrintable(manager.lastError()));
    QCOMPARE(failed.count(), 0);
    QVERIFY(progress.count() > 0);
    QVERIFY(progress.first().at(1).toLongLong() > 0);
    QCOMPARE(completed.first().at(0).toString(), QString("relay-forward"));
    QCOMPARE(completed.first().at(1).toLongLong(), qint64(content.size()));
    QVERIFY(requestBytes.startsWith("GET /api/v1/forwards/relay-forward/files/content?path=relay.txt"));
    QVERIFY(requestBytes.toLower().contains("range: bytes=0-\r\n"));
    QVERIFY(requestBytes.toLower().contains("x-device-id: target\r\n"));
    QVERIFY(requestBytes.toLower().contains("x-device-token: device-token\r\n"));

    QFile received(QDir(temporary.path()).filePath("receive/Downloads/relay.txt"));
    QVERIFY(received.open(QIODevice::ReadOnly));
    QCOMPARE(received.readAll(), content);
}

QTEST_MAIN(RelayDownloadManagerTest)
#include "relaydownloadmanager_test.moc"
