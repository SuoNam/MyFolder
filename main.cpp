#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QQmlContext>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonArray>
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
    // Use the transparent PNG on Linux as well as Windows. The ICO contains
    // a Windows-specific opaque background and is not suitable for desktop
    // shells or task switchers.
    app.setWindowIcon(QIcon(":/qt/qml/MyTest/Icons/app.png"));
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
    const QCommandLineOption uploadServerSelectionOption(
        "upload-server-selection", "Upload files listed in a UTF-8 selection file.", "selection-file");
    const QCommandLineOption sendClientSelectionOption(
        "send-client-selection", "Send files listed in a UTF-8 selection file.", "selection-file");
    parser.addOption(uploadServerOption);
    parser.addOption(sendClientOption);
    parser.addOption(uploadServerSelectionOption);
    parser.addOption(sendClientSelectionOption);
    parser.process(app);

    QString startupAction;
    QString startupFilePath;
    QStringList startupFilePaths;
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
    const auto readSelectionFile = [&normalizedCommandFile](const QString &value) {
        QStringList paths;
        const QString selectionPath = normalizedCommandFile(value);
        QFile file(selectionPath);
        if (file.open(QIODevice::ReadOnly)) {
            const QString contents = QString::fromUtf8(file.readAll());
            for (const QString &line : contents.split('\n', Qt::SkipEmptyParts)) {
                const QString path = QFileInfo(line.trimmed()).absoluteFilePath();
                if (QFileInfo(path).isFile() && !paths.contains(path, Qt::CaseInsensitive))
                    paths.append(path);
            }
        }
        file.close();
        QFile::remove(selectionPath);
        return paths;
    };
    if (parser.isSet(uploadServerSelectionOption)) {
        startupAction = "upload-server";
        startupFilePaths = readSelectionFile(parser.value(uploadServerSelectionOption));
    } else if (parser.isSet(sendClientSelectionOption)) {
        startupAction = "send-client";
        startupFilePaths = readSelectionFile(parser.value(sendClientSelectionOption));
    } else if (parser.isSet(uploadServerOption)) {
        startupAction = "upload-server";
        startupFilePath = normalizedCommandFile(parser.value(uploadServerOption));
        startupFilePaths.append(startupFilePath);
        for (const QString &value : parser.positionalArguments()) {
            const QString path = normalizedCommandFile(value);
            if (QFileInfo(path).isFile() && !startupFilePaths.contains(path, Qt::CaseInsensitive))
                startupFilePaths.append(path);
        }
    } else if (parser.isSet(sendClientOption)) {
        startupAction = "send-client";
        startupFilePath = normalizedCommandFile(parser.value(sendClientOption));
        startupFilePaths.append(startupFilePath);
        for (const QString &value : parser.positionalArguments()) {
            const QString path = normalizedCommandFile(value);
            if (QFileInfo(path).isFile() && !startupFilePaths.contains(path, Qt::CaseInsensitive))
                startupFilePaths.append(path);
        }
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
        QJsonArray selectedPaths;
        for (const QString &path : startupFilePaths) selectedPaths.append(path);
        const QJsonObject command{{"action", startupAction.isEmpty() ? "activate" : startupAction},
                                  {"filePath", startupFilePath}, {"filePaths", selectedPaths}};
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
    QJsonArray startupSelectedPaths;
    for (const QString &path : startupFilePaths) startupSelectedPaths.append(path);
    engine.rootContext()->setContextProperty("startupAction", startupAction);
    engine.rootContext()->setContextProperty("startupFilePath", startupFilePath);
    engine.rootContext()->setContextProperty(
        "startupFilePaths",
        QString::fromUtf8(QJsonDocument(startupSelectedPaths).toJson(QJsonDocument::Compact)));
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
                    const QVariant payload = command.value("action").toString() == "oauth-callback"
                        ? QVariant(command.value("filePath").toString())
                        : QVariant(QString::fromUtf8(QJsonDocument(command.value("filePaths").toArray())
                                                         .toJson(QJsonDocument::Compact)));
                    QMetaObject::invokeMethod(engine.rootObjects().constFirst(),
                                              "handleExternalCommand", Qt::QueuedConnection,
                                              Q_ARG(QVariant, command.value("action").toString()),
                                              Q_ARG(QVariant, payload));
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
