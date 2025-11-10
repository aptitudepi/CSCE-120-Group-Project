#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QtSql>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <unistd.h>

#include "controllers/WeatherController.h"
#include "controllers/AlertController.h"
#include "database/DatabaseManager.h"

int main(int argc, char *argv[])
{
    // Set XDG_RUNTIME_DIR if not set (fixes permission warning in some environments)
    if (qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR")) {
        QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (runtimeDir.isEmpty()) {
            runtimeDir = "/tmp/runtime-" + QString::number(getuid());
            QDir().mkpath(runtimeDir);
        }
        qputenv("XDG_RUNTIME_DIR", runtimeDir.toLocal8Bit());
    }
    
    QGuiApplication app(argc, argv);
    
    // Set application metadata
    app.setOrganizationName("MaroonPtrs");
    app.setOrganizationDomain("maroonptrs.edu");
    app.setApplicationName("HyperlocalWeather");
    app.setApplicationVersion("1.0.0");
    
    // Initialize database
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager->initialize()) {
        qCritical() << "Failed to initialize database";
        return -1;
    }
    
    // Create controllers
    WeatherController weatherController;
    AlertController alertController;
    
    // Setup QML engine
    QQmlApplicationEngine engine;
    
    // Expose controllers to QML
    engine.rootContext()->setContextProperty("weatherController", &weatherController);
    engine.rootContext()->setContextProperty("alertController", &alertController);
    
    // Load main QML file from QML module
    // With qt6_add_qml_module, the QML files are available at qrc:/qt/qml/HyperlocalWeather/main.qml
    const QUrl url(QStringLiteral("qrc:/qt/qml/HyperlocalWeather/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "Failed to load QML from:" << objUrl.toString();
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);
    
    engine.load(url);
    
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML from:" << url.toString();
        return -1;
    }
    
    qInfo() << "Application started successfully";
    
    return app.exec();
}

