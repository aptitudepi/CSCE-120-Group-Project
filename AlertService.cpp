
// AlertService.h
#ifndef ALERTSERVICE_H
#define ALERTSERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSmtp>
#include <QtMath>

class AlertManager : public QObject
{
    Q_OBJECT

public:
    explicit AlertManager(QObject *parent = nullptr);
    ~AlertManager();

    bool initializeDatabase();
    int createSubscription(const QJsonObject &subscriptionData);
    QJsonArray getUserSubscriptions(const QString &userId);
    QJsonArray checkAlertConditions(const QJsonObject &weatherPrediction, const QJsonObject &subscription);
    bool sendEmailAlert(const QString &email, const QJsonObject &alert);
    void saveAlertHistory(int subscriptionId, const QJsonObject &alert, const QJsonObject &weatherData, const QString &status = "sent");

private:
    QSqlDatabase m_database;
    QString m_databasePath;
    QSettings *m_emailSettings;

    bool setupTables();
};

class AlertService : public QObject
{
    Q_OBJECT

public:
    explicit AlertService(QObject *parent = nullptr);
    bool start(int port = 8004);
    void stop();

private:
    QHttpServer *m_server;
    AlertManager *m_alertManager;
    QSettings *m_settings;
};

#endif // ALERTSERVICE_H

// AlertService.cpp
#include "AlertService.h"
#include <QCoreApplication>
#include <QDebug>

AlertManager::AlertManager(QObject *parent)
    : QObject(parent)
    , m_emailSettings(new QSettings(this))
{
    m_databasePath = QCoreApplication::applicationDirPath() + "/alerts.db";
    initializeDatabase();
}

AlertManager::~AlertManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool AlertManager::initializeDatabase()
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", "alerts");
    m_database.setDatabaseName(m_databasePath);

    if (!m_database.open()) {
        qCritical() << "Failed to open alerts database:" << m_database.lastError().text();
        return false;
    }

    return setupTables();
}

bool AlertManager::setupTables()
{
    QSqlQuery query(m_database);

    // Alert subscriptions table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS alert_subscriptions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT,
            location_name TEXT,
            latitude REAL,
            longitude REAL,
            alert_types TEXT,
            threshold_config TEXT,
            notification_methods TEXT,
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Failed to create alert_subscriptions table:" << query.lastError().text();
        return false;
    }

    // Alert history table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS alert_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            subscription_id INTEGER,
            alert_type TEXT,
            severity TEXT,
            message TEXT,
            weather_data TEXT,
            sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            status TEXT DEFAULT 'sent',
            FOREIGN KEY (subscription_id) REFERENCES alert_subscriptions (id)
        )
    )")) {
        qCritical() << "Failed to create alert_history table:" << query.lastError().text();
        return false;
    }

    // User preferences table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS user_preferences (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT UNIQUE,
            email TEXT,
            phone TEXT,
            push_token TEXT,
            quiet_hours_start TIME,
            quiet_hours_end TIME,
            timezone TEXT DEFAULT 'UTC',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Failed to create user_preferences table:" << query.lastError().text();
        return false;
    }

    qDebug() << "Alert database tables initialized successfully";
    return true;
}

int AlertManager::createSubscription(const QJsonObject &subscriptionData)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO alert_subscriptions 
        (user_id, location_name, latitude, longitude, alert_types, threshold_config, notification_methods)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(subscriptionData["user_id"].toString());
    query.addBindValue(subscriptionData["location_name"].toString());
    query.addBindValue(subscriptionData["latitude"].toDouble());
    query.addBindValue(subscriptionData["longitude"].toDouble());

    QJsonDocument alertTypesDoc(subscriptionData["alert_types"].toArray());
    query.addBindValue(alertTypesDoc.toJson(QJsonDocument::Compact));

    QJsonDocument thresholdConfigDoc(subscriptionData["threshold_config"].toObject());
    query.addBindValue(thresholdConfigDoc.toJson(QJsonDocument::Compact));

    QJsonDocument notificationMethodsDoc(subscriptionData["notification_methods"].toArray());
    query.addBindValue(notificationMethodsDoc.toJson(QJsonDocument::Compact));

    if (query.exec()) {
        return query.lastInsertId().toInt();
    } else {
        qCritical() << "Failed to create alert subscription:" << query.lastError().text();
        return -1;
    }
}

QJsonArray AlertManager::getUserSubscriptions(const QString &userId)
{
    QJsonArray subscriptions;

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT id, location_name, latitude, longitude, alert_types, threshold_config, 
               notification_methods, is_active, created_at
        FROM alert_subscriptions
        WHERE user_id = ? AND is_active = TRUE
    )");
    query.addBindValue(userId);

    if (query.exec()) {
        while (query.next()) {
            QJsonObject subscription;
            subscription["id"] = query.value("id").toInt();
            subscription["location_name"] = query.value("location_name").toString();
            subscription["latitude"] = query.value("latitude").toDouble();
            subscription["longitude"] = query.value("longitude").toDouble();
            subscription["is_active"] = query.value("is_active").toBool();
            subscription["created_at"] = query.value("created_at").toString();

            // Parse JSON fields
            QJsonDocument alertTypesDoc = QJsonDocument::fromJson(query.value("alert_types").toByteArray());
            subscription["alert_types"] = alertTypesDoc.array();

            QJsonDocument thresholdConfigDoc = QJsonDocument::fromJson(query.value("threshold_config").toByteArray());
            subscription["threshold_config"] = thresholdConfigDoc.object();

            QJsonDocument notificationMethodsDoc = QJsonDocument::fromJson(query.value("notification_methods").toByteArray());
            subscription["notification_methods"] = notificationMethodsDoc.array();

            subscriptions.append(subscription);
        }
    } else {
        qCritical() << "Failed to get user subscriptions:" << query.lastError().text();
    }

    return subscriptions;
}

QJsonArray AlertManager::checkAlertConditions(const QJsonObject &weatherPrediction, const QJsonObject &subscription)
{
    QJsonArray alerts;
    QJsonObject predictions = weatherPrediction["predictions"].toObject();
    double confidence = weatherPrediction["confidence_score"].toDouble();
    QJsonObject thresholdConfig = subscription["threshold_config"].toObject();
    QJsonArray alertTypes = subscription["alert_types"].toArray();

    // Convert alert types array to QStringList for easier checking
    QStringList alertTypesList;
    for (const auto &alertType : alertTypes) {
        alertTypesList.append(alertType.toString());
    }

    // Precipitation alerts
    if (alertTypesList.contains("precipitation")) {
        double precipThreshold = thresholdConfig["precipitation_threshold"].toDouble(1.0);
        double precipConfidenceMin = thresholdConfig["precipitation_confidence"].toDouble(0.8);
        double precipIntensity = predictions["precipitation_intensity"].toDouble();

        if (precipIntensity >= precipThreshold && confidence >= precipConfidenceMin) {
            QJsonObject alert;
            alert["type"] = "precipitation";
            alert["severity"] = (precipIntensity > 5.0) ? "high" : "medium";
            alert["message"] = QString("Heavy precipitation expected: %1mm/hr").arg(precipIntensity, 0, 'f', 1);
            alert["confidence"] = confidence;
            alert["location"] = subscription["location_name"].toString();
            alerts.append(alert);
        }
    }

    // Wind alerts
    if (alertTypesList.contains("wind")) {
        double windThreshold = thresholdConfig["wind_threshold"].toDouble(10.0);
        double windConfidenceMin = thresholdConfig["wind_confidence"].toDouble(0.8);
        double windSpeed = predictions["wind_speed"].toDouble();

        if (windSpeed >= windThreshold && confidence >= windConfidenceMin) {
            QJsonObject alert;
            alert["type"] = "wind";
            alert["severity"] = (windSpeed > 20.0) ? "high" : "medium";
            alert["message"] = QString("Strong winds expected: %1m/s").arg(windSpeed, 0, 'f', 1);
            alert["confidence"] = confidence;
            alert["location"] = subscription["location_name"].toString();
            alerts.append(alert);
        }
    }

    // Temperature alerts
    if (alertTypesList.contains("temperature")) {
        double tempHigh = thresholdConfig["temperature_high"].toDouble(35.0);
        double tempLow = thresholdConfig["temperature_low"].toDouble(-10.0);
        double tempConfidenceMin = thresholdConfig["temperature_confidence"].toDouble(0.8);
        double temperature = predictions["temperature"].toDouble();

        if (confidence >= tempConfidenceMin) {
            if (temperature >= tempHigh) {
                QJsonObject alert;
                alert["type"] = "temperature";
                alert["severity"] = (temperature > 40.0) ? "high" : "medium";
                alert["message"] = QString("Extreme heat warning: %1°C").arg(temperature, 0, 'f', 1);
                alert["confidence"] = confidence;
                alert["location"] = subscription["location_name"].toString();
                alerts.append(alert);
            } else if (temperature <= tempLow) {
                QJsonObject alert;
                alert["type"] = "temperature";
                alert["severity"] = (temperature < -20.0) ? "high" : "medium";
                alert["message"] = QString("Extreme cold warning: %1°C").arg(temperature, 0, 'f', 1);
                alert["confidence"] = confidence;
                alert["location"] = subscription["location_name"].toString();
                alerts.append(alert);
            }
        }
    }

    return alerts;
}

bool AlertManager::sendEmailAlert(const QString &email, const QJsonObject &alert)
{
    // Email functionality would require additional SMTP library
    // For now, just log the alert
    qDebug() << "Email alert would be sent to:" << email;
    qDebug() << "Alert:" << QJsonDocument(alert).toJson(QJsonDocument::Compact);

    // In production, implement actual email sending using QSmtp or similar
    return true;
}

void AlertManager::saveAlertHistory(int subscriptionId, const QJsonObject &alert, const QJsonObject &weatherData, const QString &status)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO alert_history 
        (subscription_id, alert_type, severity, message, weather_data, status)
        VALUES (?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(subscriptionId);
    query.addBindValue(alert["type"].toString());
    query.addBindValue(alert["severity"].toString());
    query.addBindValue(alert["message"].toString());
    query.addBindValue(QJsonDocument(weatherData).toJson(QJsonDocument::Compact));
    query.addBindValue(status);

    if (!query.exec()) {
        qCritical() << "Failed to save alert history:" << query.lastError().text();
    }
}

AlertService::AlertService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_alertManager(new AlertManager(this))
    , m_settings(new QSettings(this))
{
    qDebug() << "Alert Service initialized";
}

bool AlertService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Alert Service";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Create subscription endpoint
    m_server->route("/subscribe", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject subscriptionData = doc.object();

        QStringList requiredFields = {"user_id", "location_name", "latitude", "longitude"};
        for (const QString &field : requiredFields) {
            if (!subscriptionData.contains(field)) {
                QJsonObject error;
                error["error"] = "Missing required field: " + field;
                error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

                QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::BadRequest);
                httpResponse.setHeader("Content-Type", "application/json");
                return httpResponse;
            }
        }

        int subscriptionId = m_alertManager->createSubscription(subscriptionData);

        QJsonObject response;
        if (subscriptionId > 0) {
            response["subscription_id"] = subscriptionId;
            response["message"] = "Alert subscription created";
        } else {
            response["error"] = "Failed to create subscription";
        }
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        int statusCode = subscriptionId > 0 ? 200 : 500;
        QHttpServerResponse httpResponse(QJsonDocument(response).toJson(), QHttpServerResponse::StatusCode(statusCode));
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Get user subscriptions
    m_server->route("/users/<arg>/subscriptions", QHttpServerRequest::Method::Get,
                   [this](const QString &userId) {
        QJsonArray subscriptions = m_alertManager->getUserSubscriptions(userId);

        QJsonObject response;
        response["subscriptions"] = subscriptions;
        response["count"] = subscriptions.size();
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Check and send alerts
    m_server->route("/check-alerts", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject requestData = doc.object();

        QJsonObject weatherPrediction = requestData["weather_prediction"].toObject();
        double latitude = requestData["latitude"].toDouble();
        double longitude = requestData["longitude"].toDouble();

        if (weatherPrediction.isEmpty() || qIsNaN(latitude) || qIsNaN(longitude)) {
            QJsonObject error;
            error["error"] = "Weather prediction and coordinates required";
            error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::BadRequest);
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;
        }

        // Find nearby subscriptions (within 5km radius for demo)
        QSqlQuery query(m_alertManager->m_database);
        query.prepare(R"(
            SELECT id, user_id, location_name, latitude, longitude, alert_types, 
                   threshold_config, notification_methods
            FROM alert_subscriptions
            WHERE is_active = TRUE
            AND ABS(latitude - ?) < 0.05 AND ABS(longitude - ?) < 0.05
        )");
        query.addBindValue(latitude);
        query.addBindValue(longitude);

        QJsonArray alertsSent;

        if (query.exec()) {
            while (query.next()) {
                QJsonObject subscription;
                subscription["id"] = query.value("id").toInt();
                subscription["user_id"] = query.value("user_id").toString();
                subscription["location_name"] = query.value("location_name").toString();
                subscription["latitude"] = query.value("latitude").toDouble();
                subscription["longitude"] = query.value("longitude").toDouble();

                QJsonDocument alertTypesDoc = QJsonDocument::fromJson(query.value("alert_types").toByteArray());
                subscription["alert_types"] = alertTypesDoc.array();

                QJsonDocument thresholdConfigDoc = QJsonDocument::fromJson(query.value("threshold_config").toByteArray());
                subscription["threshold_config"] = thresholdConfigDoc.object();

                QJsonDocument notificationMethodsDoc = QJsonDocument::fromJson(query.value("notification_methods").toByteArray());
                subscription["notification_methods"] = notificationMethodsDoc.array();

                // Check for alert conditions
                QJsonArray triggeredAlerts = m_alertManager->checkAlertConditions(weatherPrediction, subscription);

                for (const auto &alertValue : triggeredAlerts) {
                    QJsonObject alert = alertValue.toObject();

                    // Save to history
                    m_alertManager->saveAlertHistory(subscription["id"].toInt(), alert, weatherPrediction);

                    // Send notifications (email simulation)
                    QJsonArray notificationMethods = subscription["notification_methods"].toArray();
                    for (const auto &methodValue : notificationMethods) {
                        if (methodValue.toString() == "email") {
                            QString demoEmail = QString("user_%1@example.com").arg(subscription["user_id"].toString());
                            m_alertManager->sendEmailAlert(demoEmail, alert);
                        }
                    }

                    QJsonObject sentAlert;
                    sentAlert["subscription_id"] = subscription["id"].toInt();
                    sentAlert["user_id"] = subscription["user_id"].toString();
                    sentAlert["alert"] = alert;
                    alertsSent.append(sentAlert);
                }
            }
        }

        QJsonObject response;
        response["alerts_sent"] = alertsSent.size();
        response["alerts"] = alertsSent;
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "alert_service";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Alert Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start Alert Service on port" << port;
        return false;
    }
}

void AlertService::stop()
{
    m_server->disconnect();
    qDebug() << "Alert Service stopped";
}

// main.cpp for Alert Service
#include <QCoreApplication>
#include "AlertService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    AlertService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "AlertService.moc"
