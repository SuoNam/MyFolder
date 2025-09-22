#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include<QQuickStyle>
#include<QIcon>
int main(int argc, char *argv[])
{ QQuickStyle::setStyle("Fusion");
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/app.ico"));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MyTest", "Main");

    return app.exec();
}
