#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QQmlContext>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include "shellintegration.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
#endif
    if (qEnvironmentVariableIsEmpty("QT_DONT_USE_NATIVE_DIALOGS")) {
        qputenv("QT_DONT_USE_NATIVE_DIALOGS", "1");
    }
    QQuickStyle::setStyle("Fusion");
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/qt/qml/MyTest/Icons/app.ico"));
    app.setOrganizationName("MyFolder");
    app.setApplicationName("MyFolder");
    app.setApplicationVersion(MYFOLDER_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("MyFolder desktop client");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption uploadServerOption(
        "upload-server", "Upload a file to the MyFolder server after login.", "file");
    const QCommandLineOption sendClientOption(
        "send-client", "Choose a client and send a file after login.", "file");
    parser.addOption(uploadServerOption);
    parser.addOption(sendClientOption);
    parser.process(app);

    QString startupAction;
    QString startupFilePath;
    const auto normalizedCommandFile = [](QString value) {
        value = value.trimmed();
        if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.size() - 2);
        } else {
            // Some Windows launchers preserve only the trailing quote.
            while (value.endsWith('"')) value.chop(1);
            while (value.startsWith('"')) value.remove(0, 1);
        }
        return QFileInfo(value).absoluteFilePath();
    };
    if (parser.isSet(uploadServerOption)) {
        startupAction = "upload-server";
        startupFilePath = normalizedCommandFile(parser.value(uploadServerOption));
    } else if (parser.isSet(sendClientOption)) {
        startupAction = "send-client";
        startupFilePath = normalizedCommandFile(parser.value(sendClientOption));
    } else if (!parser.positionalArguments().isEmpty()
               && parser.positionalArguments().constFirst().startsWith("myfolder://oauth/callback", Qt::CaseInsensitive)) {
        startupAction = "oauth-callback";
        startupFilePath = parser.positionalArguments().constFirst();
    }

    // Explorer context-menu commands reuse the signed-in process. A second
    // independent process has no in-memory access token and would show login.
    const QString instanceName = QStringLiteral("MyFolder.Desktop.v1.1.1");
    QLocalSocket existingInstance;
    existingInstance.connectToServer(instanceName);
    if (existingInstance.waitForConnected(350)) {
        const QJsonObject command{{"action", startupAction.isEmpty() ? "activate" : startupAction},
                                  {"filePath", startupFilePath}};
        existingInstance.write(QJsonDocument(command).toJson(QJsonDocument::Compact) + '\n');
        existingInstance.flush();
        existingInstance.waitForBytesWritten(1000);
        existingInstance.waitForReadyRead(1000);
        return 0;
    }

    QLocalServer::removeServer(instanceName);
    QLocalServer instanceServer;
    instanceServer.listen(instanceName);

    QQmlApplicationEngine engine;
    ShellIntegration shellIntegration;
    engine.rootContext()->setContextProperty("startupAction", startupAction);
    engine.rootContext()->setContextProperty("startupFilePath", startupFilePath);
    engine.rootContext()->setContextProperty("ShellIntegration", &shellIntegration);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MyTest", "Main");

    QObject::connect(&instanceServer, &QLocalServer::newConnection, &app,
                     [&instanceServer, &engine]() {
        while (QLocalSocket *socket = instanceServer.nextPendingConnection()) {
            const auto processCommand = [socket, &engine]() {
                if (socket->property("commandHandled").toBool() || !socket->canReadLine()) return;
                socket->setProperty("commandHandled", true);
                const QJsonDocument document = QJsonDocument::fromJson(socket->readLine().trimmed());
                if (document.isObject() && !engine.rootObjects().isEmpty()) {
                    const QJsonObject command = document.object();
                    QMetaObject::invokeMethod(engine.rootObjects().constFirst(),
                                              "handleExternalCommand", Qt::QueuedConnection,
                                              Q_ARG(QVariant, command.value("action").toString()),
                                              Q_ARG(QVariant, command.value("filePath").toString()));
                }
                socket->write("ok\n");
                socket->flush();
                socket->disconnectFromServer();
            };
            QObject::connect(socket, &QLocalSocket::readyRead, socket, processCommand);
            QTimer::singleShot(0, socket, processCommand);
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    return app.exec();
}
