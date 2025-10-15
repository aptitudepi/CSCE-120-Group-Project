#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "weatherclient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application information
    QCoreApplication::setOrganizationName("CSCE-120");
    QCoreApplication::setOrganizationDomain("tamu.edu");
    QCoreApplication::setApplicationName("Hyperlocal Weather Forecasting");

    qDebug() << "Starting Hyperlocal Weather Application...";

    // Create QML engine
    QQmlApplicationEngine engine;

    // Register WeatherClient type for QML
    qmlRegisterType<WeatherClient>("com.weather.api", 1, 0, "WeatherAPI");

    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "Failed to load QML file";
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "No root objects loaded";
        return -1;
    }

    qDebug() << "Application started successfully";
    return app.exec();
}
