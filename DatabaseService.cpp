
// DatabaseService.h
#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

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
#include <QRedis>
#include <QTimer>
#include <QDebug>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initializeConnections();
    bool initializeSchema();
    int storeWeatherData(const QJsonObject &weatherData);
    QJsonArray getHistoricalWeather(double lat, double lon, int hours = 24);
    bool cacheForecast(const QString &key, const QJsonObject &forecastData, int ttl = 600);
    QJsonObject getCachedForecast(const QString &key);
    bool storeUserSession(const QString &userId, const QJsonObject &sessionData);

private:
    QSqlDatabase m_pgDatabase;
    QRedis *m_redisClient;
    QSettings *m_settings;
    QString m_databaseUrl;
    QString m_redisUrl;

    bool setupPostgreSQLTables();
    bool connectToRedis();
};

class DatabaseService : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseService(QObject *parent = nullptr);
    bool start(int port = 8006);
    void stop();

private:
    QHttpServer *m_server;
    DatabaseManager *m_dbManager;
    QSettings *m_settings;
};

#endif // DATABASESERVICE_H

// DatabaseService.cpp
#include "DatabaseService.h"
#include <QCoreApplication>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_redisClient(nullptr)
    , m_settings(new QSettings(this))
{
    m_databaseUrl = m_settings->value("database/postgresql_url", 
        "postgresql://postgres:password@localhost:5432/weather_db").toString();
    m_redisUrl = m_settings->value("database/redis_url", "redis://localhost:6379").toString();

    initializeConnections();
}

DatabaseManager::~DatabaseManager()
{
    if (m_pgDatabase.isOpen()) {
        m_pgDatabase.close();
    }
    if (m_redisClient) {
        delete m_redisClient;
    }
}

bool DatabaseManager::initializeConnections()
{
    // PostgreSQL connection
    m_pgDatabase = QSqlDatabase::addDatabase("QPSQL", "weather");

    // Parse connection URL (simplified)
    // In production, use proper URL parsing
    m_pgDatabase.setHostName("localhost");
    m_pgDatabase.setDatabaseName("weather_db");
    m_pgDatabase.setUserName("postgres");
    m_pgDatabase.setPassword("password");
    m_pgDatabase.setPort(5432);

    if (!m_pgDatabase.open()) {
        qCritical() << "Failed to connect to PostgreSQL:" << m_pgDatabase.lastError().text();

        // Fallback to SQLite for development
        qDebug() << "Falling back to SQLite database";
        QSqlDatabase::removeDatabase("weather");
        m_pgDatabase = QSqlDatabase::addDatabase("QSQLITE", "weather");
        m_pgDatabase.setDatabaseName(QCoreApplication::applicationDirPath() + "/weather.db");

        if (!m_pgDatabase.open()) {
            qCritical() << "Failed to connect to SQLite:" << m_pgDatabase.lastError().text();
            return false;
        }
    }

    qDebug() << "Database connection established";

    // Initialize schema
    if (!initializeSchema()) {
        return false;
    }

    // Redis connection (optional)
    connectToRedis();

    return true;
}

bool DatabaseManager::initializeSchema()
{
    QSqlQuery query(m_pgDatabase);

    // Weather data table
    QString weatherTableSql = R"(
        CREATE TABLE IF NOT EXISTS weather_data (
            id SERIAL PRIMARY KEY,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            temperature REAL,
            humidity REAL,
            precipitation REAL,
            wind_speed REAL,
            pressure REAL,
            source_data TEXT,
            processed_data TEXT,
            prediction_data TEXT,
            quality_score REAL
        )
    )";

    if (!query.exec(weatherTableSql)) {
        qCritical() << "Failed to create weather_data table:" << query.lastError().text();
        return false;
    }

    // Weather forecasts table
    QString forecastTableSql = R"(
        CREATE TABLE IF NOT EXISTS weather_forecasts (
            id SERIAL PRIMARY KEY,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            forecast_time TIMESTAMP NOT NULL,
            valid_time TIMESTAMP NOT NULL,
            model_type TEXT DEFAULT 'LSTM',
            predictions TEXT NOT NULL,
            confidence_score REAL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(forecastTableSql)) {
        qCritical() << "Failed to create weather_forecasts table:" << query.lastError().text();
        return false;
    }

    // User sessions table
    QString sessionTableSql = R"(
        CREATE TABLE IF NOT EXISTS user_sessions (
            id SERIAL PRIMARY KEY,
            user_id TEXT UNIQUE,
            session_data TEXT,
            last_location TEXT,
            preferences TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(sessionTableSql)) {
        qCritical() << "Failed to create user_sessions table:" << query.lastError().text();
        return false;
    }

    // Create indexes for better performance
    query.exec("CREATE INDEX IF NOT EXISTS idx_weather_location ON weather_data (latitude, longitude, timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_forecast_location ON weather_forecasts (latitude, longitude, forecast_time DESC)");

    qDebug() << "Database schema initialized successfully";
    return true;
}

bool DatabaseManager::connectToRedis()
{
    // For this demo, we'll simulate Redis functionality
    // In production, use actual QRedis library
    qDebug() << "Redis connection simulated (would connect to" << m_redisUrl << ")";
    return true;
}

int DatabaseManager::storeWeatherData(const QJsonObject &weatherData)
{
    QSqlQuery query(m_pgDatabase);
    query.prepare(R"(
        INSERT INTO weather_data 
        (latitude, longitude, temperature, humidity, precipitation, wind_speed, 
         source_data, processed_data, prediction_data, quality_score)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(weatherData["latitude"].toDouble());
    query.addBindValue(weatherData["longitude"].toDouble());
    query.addBindValue(weatherData["temperature"].toVariant());
    query.addBindValue(weatherData["humidity"].toVariant());
    query.addBindValue(weatherData["precipitation"].toVariant());
    query.addBindValue(weatherData["wind_speed"].toVariant());
    query.addBindValue(QJsonDocument(weatherData["source_data"].toObject()).toJson(QJsonDocument::Compact));
    query.addBindValue(QJsonDocument(weatherData["processed_data"].toObject()).toJson(QJsonDocument::Compact));
    query.addBindValue(QJsonDocument(weatherData["prediction_data"].toObject()).toJson(QJsonDocument::Compact));
    query.addBindValue(weatherData["quality_score"].toDouble(0.0));

    if (query.exec()) {
        return query.lastInsertId().toInt();
    } else {
        qCritical() << "Failed to store weather data:" << query.lastError().text();
        return -1;
    }
}

QJsonArray DatabaseManager::getHistoricalWeather(double lat, double lon, int hours)
{
    QJsonArray historicalData;

    QSqlQuery query(m_pgDatabase);
    query.prepare(R"(
        SELECT temperature, humidity, precipitation, wind_speed, timestamp, quality_score
        FROM weather_data
        WHERE ABS(latitude - ?) < 0.01 AND ABS(longitude - ?) < 0.01
        AND timestamp > ?
        ORDER BY timestamp DESC
        LIMIT 100
    )");

    query.addBindValue(lat);
    query.addBindValue(lon);
    query.addBindValue(QDateTime::currentDateTime().addSecs(-hours * 3600));

    if (query.exec()) {
        while (query.next()) {
            QJsonObject record;
            record["temperature"] = query.value("temperature").toDouble();
            record["humidity"] = query.value("humidity").toDouble();
            record["precipitation"] = query.value("precipitation").toDouble();
            record["wind_speed"] = query.value("wind_speed").toDouble();
            record["timestamp"] = query.value("timestamp").toDateTime().toString(Qt::ISODate);
            record["quality_score"] = query.value("quality_score").toDouble();
            historicalData.append(record);
        }
    } else {
        qCritical() << "Failed to get historical weather:" << query.lastError().text();
    }

    return historicalData;
}

bool DatabaseManager::cacheForecast(const QString &key, const QJsonObject &forecastData, int ttl)
{
    // Simulate Redis caching with in-memory cache for demo
    static QHash<QString, QPair<QJsonObject, QDateTime>> cache;

    QDateTime expiry = QDateTime::currentDateTime().addSecs(ttl);
    cache[key] = qMakePair(forecastData, expiry);

    qDebug() << "Cached forecast with key:" << key << "TTL:" << ttl << "seconds";
    return true;
}

QJsonObject DatabaseManager::getCachedForecast(const QString &key)
{
    // Simulate Redis get with in-memory cache for demo
    static QHash<QString, QPair<QJsonObject, QDateTime>> cache;

    if (cache.contains(key)) {
        QPair<QJsonObject, QDateTime> cachedItem = cache[key];
        if (cachedItem.second > QDateTime::currentDateTime()) {
            qDebug() << "Retrieved cached forecast for key:" << key;
            return cachedItem.first;
        } else {
            cache.remove(key); // Remove expired item
        }
    }

    return QJsonObject(); // Empty object if not found or expired
}

bool DatabaseManager::storeUserSession(const QString &userId, const QJsonObject &sessionData)
{
    QSqlQuery query(m_pgDatabase);
    query.prepare(R"(
        INSERT INTO user_sessions (user_id, session_data, updated_at)
        VALUES (?, ?, CURRENT_TIMESTAMP)
        ON CONFLICT (user_id)
        DO UPDATE SET session_data = ?, updated_at = CURRENT_TIMESTAMP
    )");

    QString sessionJson = QJsonDocument(sessionData).toJson(QJsonDocument::Compact);
    query.addBindValue(userId);
    query.addBindValue(sessionJson);
    query.addBindValue(sessionJson); // For the UPDATE part

    if (query.exec()) {
        return true;
    } else {
        // Try SQLite syntax if PostgreSQL syntax fails
        query.prepare(R"(
            INSERT OR REPLACE INTO user_sessions (user_id, session_data, updated_at)
            VALUES (?, ?, CURRENT_TIMESTAMP)
        )");
        query.addBindValue(userId);
        query.addBindValue(sessionJson);

        if (query.exec()) {
            return true;
        } else {
            qCritical() << "Failed to store user session:" << query.lastError().text();
            return false;
        }
    }
}

DatabaseService::DatabaseService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_dbManager(new DatabaseManager(this))
    , m_settings(new QSettings(this))
{
    qDebug() << "Database Service initialized";
}

bool DatabaseService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Database Service";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Store weather data
    m_server->route("/weather/store", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject weatherData = doc.object();

        int recordId = m_dbManager->storeWeatherData(weatherData);

        QJsonObject response;
        if (recordId > 0) {
            response["id"] = recordId;
            response["message"] = "Weather data stored";
        } else {
            response["error"] = "Failed to store weather data";
        }
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        int statusCode = recordId > 0 ? 200 : 500;
        QHttpServerResponse httpResponse(QJsonDocument(response).toJson(), QHttpServerResponse::StatusCode(statusCode));
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Get historical weather data
    m_server->route("/weather/historical/<arg>/<arg>", QHttpServerRequest::Method::Get,
                   [this](double lat, double lon, const QHttpServerRequest &request) {
        QString hoursParam = request.query().queryItemValue("hours");
        int hours = hoursParam.isEmpty() ? 24 : hoursParam.toInt();

        QJsonArray historicalData = m_dbManager->getHistoricalWeather(lat, lon, hours);

        QJsonObject response;
        response["data"] = historicalData;
        response["count"] = historicalData.size();
        response["hours"] = hours;
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Cache forecast data
    m_server->route("/cache/forecast", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject requestData = doc.object();

        QString cacheKey = requestData["key"].toString();
        QJsonObject forecastData = requestData["data"].toObject();
        int ttl = requestData["ttl"].toInt(600);

        if (cacheKey.isEmpty() || forecastData.isEmpty()) {
            QJsonObject error;
            error["error"] = "Key and data required";
            error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::BadRequest);
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;
        }

        bool success = m_dbManager->cacheForecast(cacheKey, forecastData, ttl);

        QJsonObject response;
        response["cached"] = success;
        response["key"] = cacheKey;
        response["ttl"] = ttl;
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Get cached forecast data
    m_server->route("/cache/forecast/<arg>", QHttpServerRequest::Method::Get,
                   [this](const QString &cacheKey) {
        QJsonObject cachedData = m_dbManager->getCachedForecast(cacheKey);

        if (cachedData.isEmpty()) {
            QJsonObject error;
            error["error"] = "Cached data not found";
            error["key"] = cacheKey;
            error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::NotFound);
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;
        }

        QHttpServerResponse httpResponse(QJsonDocument(cachedData).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Store user session
    m_server->route("/users/<arg>/session", QHttpServerRequest::Method::Post,
                   [this](const QString &userId, const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject sessionData = doc.object();

        bool success = m_dbManager->storeUserSession(userId, sessionData);

        QJsonObject response;
        response["stored"] = success;
        response["user_id"] = userId;
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        int statusCode = success ? 200 : 500;
        QHttpServerResponse httpResponse(QJsonDocument(response).toJson(), QHttpServerResponse::StatusCode(statusCode));
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "database";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Database Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start Database Service on port" << port;
        return false;
    }
}

void DatabaseService::stop()
{
    m_server->disconnect();
    qDebug() << "Database Service stopped";
}

// main.cpp for Database Service
#include <QCoreApplication>
#include "DatabaseService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DatabaseService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "DatabaseService.moc"
