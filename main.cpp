#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include<QQuickStyle>
#include<QIcon>
#include <QQmlContext>
#include"ocr.h"
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    qDebug() << "TESSDATA_PREFIX ="
             << QProcessEnvironment::systemEnvironment().value("TESSDATA_PREFIX");
    QQuickStyle::setStyle("Fusion");
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/app.ico"));

    QQmlApplicationEngine engine;
    OCR ocr;

    engine.rootContext()->setContextProperty("ocr",&ocr);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MyTest", "Main");

    return app.exec();
}
