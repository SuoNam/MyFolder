#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include<QQuickStyle>
#include<QIcon>
#include <QQmlContext>
#include"ocr.h"
#include"fat12format.h"
#include <QProcessEnvironment>
#include"command.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Fusion");
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/app.ico"));

    QQmlApplicationEngine engine;
    Fat12Format fat12;
    OCR ocr;
    Command command;
    engine.rootContext()->setContextProperty("fat12",&fat12);
    engine.rootContext()->setContextProperty("ocr",&ocr);
    engine.rootContext()->setContextProperty("command",&command);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MyTest", "Main");

    return app.exec();
}
