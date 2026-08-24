#include "../httphandler.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>
#include <utility>

class DisconnectOnceServer : public QObject
{
    Q_OBJECT

public:
    explicit DisconnectOnceServer(const QByteArray &body, QObject *parent = nullptr)
        : QObject(parent), m_body(body)
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket *socket = m_server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                    if (socket->property("handled").toBool()) return;
                    socket->setProperty("handled", true);
                    socket->readAll();
                    ++m_requests;
                    if (m_requests == 1) {
                        socket->abort();
                        return;
                    }
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
                                  QByteArray::number(m_body.size()) +
                                  "\r\nConnection: close\r\n\r\n" + m_body);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    bool listen() { return m_server.listen(QHostAddress::LocalHost); }
    quint16 port() const { return m_server.serverPort(); }
    int requests() const { return m_requests; }

private:
    QTcpServer m_server;
    QByteArray m_body;
    int m_requests = 0;
};

class ResponseServer : public QObject
{
    Q_OBJECT

public:
    ResponseServer(int status, QByteArray body, QObject *parent = nullptr)
        : QObject(parent), m_status(status), m_body(std::move(body))
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket *socket = m_server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                    if (socket->property("handled").toBool()) return;
                    socket->setProperty("handled", true);
                    socket->readAll();
                    ++m_requests;
                    const QByteArray reason = m_status == 200 ? "OK" : "Unauthorized";
                    socket->write("HTTP/1.1 " + QByteArray::number(m_status) + " " + reason +
                                  "\r\nContent-Type: application/json\r\nContent-Length: " +
                                  QByteArray::number(m_body.size()) +
                                  "\r\nConnection: close\r\n\r\n" + m_body);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    bool listen() { return m_server.listen(QHostAddress::LocalHost); }
    quint16 port() const { return m_server.serverPort(); }
    int requests() const { return m_requests; }

private:
    QTcpServer m_server;
    int m_status;
    QByteArray m_body;
    int m_requests = 0;
};

class HttpHandlerTest : public QObject
{
    Q_OBJECT

private slots:
    void retriesGroupListAfterDisconnect();
    void retriesScopedDirectoryAfterDisconnect();
    void coalescesConcurrentSessionRefresh();
    void emitsAuthenticationRequiredForAuthenticated401();
};

void HttpHandlerTest::retriesGroupListAfterDisconnect()
{
    DisconnectOnceServer server(R"({"status":200,"data":[{"groupId":"group-1","name":"Test"}]})");
    QVERIFY(server.listen());
    HttpHandler handler;
    handler.setBaseUrl(QString("http://127.0.0.1:%1").arg(server.port()));
    handler.setAuthToken("jwt");
    QSignalSpy resultSpy(&handler, &HttpHandler::groupsResult);

    handler.loadGroups();

    QTRY_COMPARE_WITH_TIMEOUT(resultSpy.count(), 1, 4000);
    QCOMPARE(server.requests(), 2);
    const QJsonObject response = resultSpy.takeFirst().at(0).toJsonObject();
    QCOMPARE(response.value("status").toInt(), 200);
    QCOMPARE(response.value("data").toArray().size(), 1);
}

void HttpHandlerTest::retriesScopedDirectoryAfterDisconnect()
{
    DisconnectOnceServer server(R"({"status":200,"data":[{"name":"report.txt","path":"report.txt","type":"file"}]})");
    QVERIFY(server.listen());
    HttpHandler handler;
    handler.setBaseUrl(QString("http://127.0.0.1:%1").arg(server.port()));
    handler.setAuthToken("jwt");
    QSignalSpy resultSpy(&handler, &HttpHandler::scopedDirectoryListed);

    handler.listScopedDirectory("", "GROUP", "group-1");

    QTRY_COMPARE_WITH_TIMEOUT(resultSpy.count(), 1, 4000);
    QCOMPARE(server.requests(), 2);
    const QList<QVariant> result = resultSpy.takeFirst();
    QCOMPARE(result.at(0).toString(), QString("GROUP"));
    QCOMPARE(result.at(1).toString(), QString("group-1"));
    QCOMPARE(result.at(3).toList().size(), 1);
    QVERIFY(result.at(4).toString().isEmpty());
}

void HttpHandlerTest::coalescesConcurrentSessionRefresh()
{
    ResponseServer server(200, R"({"status":200,"data":{"accessToken":"access-two","refreshToken":"refresh-two","expiresIn":7200,"account":"alice","email":"alice@example.test"}})");
    QVERIFY(server.listen());
    HttpHandler handler;
    handler.setBaseUrl(QString("http://127.0.0.1:%1").arg(server.port()));
    QVERIFY(handler.saveStoredSession(QJsonObject{{"refreshToken", "refresh-one"},
                                                  {"account", "alice"}}));
    QSignalSpy resultSpy(&handler, &HttpHandler::refreshResult);
    QSignalSpy readySpy(&handler, &HttpHandler::sessionReady);

    handler.refreshSession();
    handler.refreshSession();

    QTRY_COMPARE_WITH_TIMEOUT(resultSpy.count(), 1, 2000);
    QCOMPARE(server.requests(), 1);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(handler.accessTokenExpiresIn(), 7200);
    QVERIFY(!handler.refreshInFlight());
    handler.clearStoredSession();
}

void HttpHandlerTest::emitsAuthenticationRequiredForAuthenticated401()
{
    ResponseServer server(401, R"({"status":401,"code":"SESSION_INVALID","message":"expired"})");
    QVERIFY(server.listen());
    HttpHandler handler;
    handler.setBaseUrl(QString("http://127.0.0.1:%1").arg(server.port()));
    handler.setAuthToken("expired-access-token");
    QSignalSpy authenticationSpy(&handler, &HttpHandler::authenticationRequired);

    handler.loadAccountProfile();

    QTRY_COMPARE_WITH_TIMEOUT(authenticationSpy.count(), 1, 2000);
    QCOMPARE(server.requests(), 1);
}

QTEST_MAIN(HttpHandlerTest)
#include "httphandler_test.moc"
