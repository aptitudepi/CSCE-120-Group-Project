// LocationService.cpp
#include "LocationService.h"
#include <QCoreApplication>
#include <QDebug>
#include <QSqlRecord>
#include <QDir>

LocationManager::LocationManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_databasePath = QCoreApplication::applicationDirPath() + "/locations.db";
    initializeDatabase();
}

LocationManager::~LocationManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool LocationManager::initializeDatabase()
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", "locations");
    m_database.setDatabaseName(m_databasePath);

    if (!m_database.open()) {
        qCritical() << "Failed to open locations database:" << m_database.lastError().text();
        return false;
    }

    return setupTables();
}

bool LocationManager::setupTables()
{
    QSqlQuery query(m_database);

    // Geocoded locations table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS geocoded_locations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            address TEXT UNIQUE,
            latitude REAL,
            longitude REAL,
            country TEXT,
            state TEXT,
            city TEXT,
            geocoded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Failed to create geocoded_locations table:" << query.lastError().text();
        return false;
    }

    // User locations table
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS user_locations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id TEXT,
            name TEXT,
            latitude REAL,
            longitude REAL,
            is_default BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qCritical() << "Failed to create user_locations table:" << query.lastError().text();
        return false;
    }

    qDebug() << "Location database tables initialized successfully";
    return true;
}

QJsonObject LocationManager::geocodeAddress(const QString &address)
{
    // Check cache first
    QSqlQuery query(m_database);
    query.prepare("SELECT latitude, longitude, country, state, city FROM geocoded_locations WHERE address = ?");
    query.addBindValue(address);

    if (query.exec() && query.next()) {
        QJsonObject cached;
        cached["address"] = address;
        cached["latitude"] = query.value("latitude").toDouble();
        cached["longitude"] = query.value("longitude").toDouble();
        cached["country"] = query.value("country").toString();
        cached["state"] = query.value("state").toString();
        cached["city"] = query.value("city").toString();
        cached["source"] = "cache";
        return cached;
    }

    // Geocode using Nominatim (OpenStreetMap)
    QUrl url("https://nominatim.openstreetmap.org/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", address);
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("addressdetails", "1");
    urlQuery.addQueryItem("limit", "1");
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0 (contact@example.com)");

    QNetworkReply *reply = m_networkManager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(10000); // 10 second timeout

    loop.exec();

    QJsonObject result;
    if (reply->error() == QNetworkReply::NoError && timer.isActive()) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray results = doc.array();

        if (!results.isEmpty()) {
            QJsonObject firstResult = results[0].toObject();
            QJsonObject addressDetails = firstResult["address"].toObject();

            result["address"] = address;
            result["latitude"] = firstResult["lat"].toString().toDouble();
            result["longitude"] = firstResult["lon"].toString().toDouble();
            result["country"] = addressDetails["country"].toString();
            result["state"] = addressDetails["state"].toString();
            result["city"] = addressDetails.value("city", addressDetails.value("town", addressDetails.value("village"))).toString();
            result["source"] = "nominatim";

            // Cache the result
            QSqlQuery insertQuery(m_database);
            insertQuery.prepare(R"(
                INSERT OR REPLACE INTO geocoded_locations 
                (address, latitude, longitude, country, state, city)
                VALUES (?, ?, ?, ?, ?, ?)
            )");
            insertQuery.addBindValue(address);
            insertQuery.addBindValue(result["latitude"].toDouble());
            insertQuery.addBindValue(result["longitude"].toDouble());
            insertQuery.addBindValue(result["country"].toString());
            insertQuery.addBindValue(result["state"].toString());
            insertQuery.addBindValue(result["city"].toString());

            if (!insertQuery.exec()) {
                qWarning() << "Failed to cache geocoding result:" << insertQuery.lastError().text();
            }
        } else {
            result["error"] = "Address not found";
        }
    } else {
        result["error"] = "Geocoding service unavailable";
    }

    reply->deleteLater();
    return result;
}

QJsonObject LocationManager::reverseGeocode(double lat, double lon)
{
    QUrl url("https://nominatim.openstreetmap.org/reverse");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("lat", QString::number(lat, 'f', 6));
    urlQuery.addQueryItem("lon", QString::number(lon, 'f', 6));
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("addressdetails", "1");
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0 (contact@example.com)");

    QNetworkReply *reply = m_networkManager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(10000);

    loop.exec();

    QJsonObject result;
    if (reply->error() == QNetworkReply::NoError && timer.isActive()) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject responseData = doc.object();
        QJsonObject addressParts = responseData["address"].toObject();

        result["latitude"] = lat;
        result["longitude"] = lon;
        result["display_name"] = responseData["display_name"].toString();
        result["country"] = addressParts["country"].toString();
        result["state"] = addressParts["state"].toString();
        result["city"] = addressParts.value("city", addressParts.value("town")).toString();
        result["road"] = addressParts["road"].toString();
        result["postcode"] = addressParts["postcode"].toString();
    } else {
        result["error"] = "Location not found";
    }

    reply->deleteLater();
    return result;
}

int LocationManager::saveUserLocation(const QString &userId, const QString &name, double lat, double lon, bool isDefault)
{
    QSqlQuery query(m_database);

    // If this is set as default, unset other defaults for this user
    if (isDefault) {
        query.prepare("UPDATE user_locations SET is_default = FALSE WHERE user_id = ?");
        query.addBindValue(userId);
        if (!query.exec()) {
            qWarning() << "Failed to unset default locations:" << query.lastError().text();
        }
    }

    // Insert new location
    query.prepare(R"(
        INSERT INTO user_locations (user_id, name, latitude, longitude, is_default)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(userId);
    query.addBindValue(name);
    query.addBindValue(lat);
    query.addBindValue(lon);
    query.addBindValue(isDefault);

    if (query.exec()) {
        return query.lastInsertId().toInt();
    } else {
        qCritical() << "Failed to save user location:" << query.lastError().text();
        return -1;
    }
}

QJsonArray LocationManager::getUserLocations(const QString &userId)
{
    QJsonArray locations;

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT id, name, latitude, longitude, is_default, created_at
        FROM user_locations
        WHERE user_id = ?
        ORDER BY is_default DESC, created_at DESC
    )");
    query.addBindValue(userId);

    if (query.exec()) {
        while (query.next()) {
            QJsonObject location;
            location["id"] = query.value("id").toInt();
            location["name"] = query.value("name").toString();
            location["latitude"] = query.value("latitude").toDouble();
            location["longitude"] = query.value("longitude").toDouble();
            location["is_default"] = query.value("is_default").toBool();
            location["created_at"] = query.value("created_at").toString();
            locations.append(location);
        }
    } else {
        qCritical() << "Failed to get user locations:" << query.lastError().text();
    }

    return locations;
}

LocationService::LocationService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_locationManager(new LocationManager(this))
    , m_settings(new QSettings(this))
{
    qDebug() << "Location Service initialized";
}

bool LocationService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Location Service";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Geocode endpoint
    m_server->route("/geocode/<arg>", QHttpServerRequest::Method::Get,
                   [this](const QString &address) {
        QString decodedAddress = QUrl::fromPercentEncoding(address.toUtf8());
        QJsonObject result = m_locationManager->geocodeAddress(decodedAddress);

        int statusCode = result.contains("error") ? 404 : 200;
        QHttpServerResponse httpResponse(QJsonDocument(result).toJson(), QHttpServerResponse::StatusCode(statusCode));
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Reverse geocode endpoint
    m_server->route("/reverse/<arg>/<arg>", QHttpServerRequest::Method::Get,
                   [this](double lat, double lon) {
        QJsonObject result = m_locationManager->reverseGeocode(lat, lon);

        int statusCode = result.contains("error") ? 404 : 200;
        QHttpServerResponse httpResponse(QJsonDocument(result).toJson(), QHttpServerResponse::StatusCode(statusCode));
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Save user location
    m_server->route("/users/<arg>/locations", QHttpServerRequest::Method::Post,
                   [this](const QString &userId, const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject locationData = doc.object();

        QString name = locationData["name"].toString();
        double latitude = locationData["latitude"].toDouble();
        double longitude = locationData["longitude"].toDouble();
        bool isDefault = locationData["is_default"].toBool(false);

        if (name.isEmpty() || qIsNaN(latitude) || qIsNaN(longitude)) {
            QJsonObject error;
            error["error"] = "Name, latitude, and longitude required";
            error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::BadRequest);
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;
        }

        int locationId = m_locationManager->saveUserLocation(userId, name, latitude, longitude, isDefault);

        QJsonObject response;
        if (locationId > 0) {
            response["id"] = locationId;
            response["message"] = "Location saved successfully";
        } else {
            response["error"] = "Failed to save location";
        }
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        int statusCode = locationId > 0 ? 200 : 500;
        QHttpServerResponse httpResponse(QJsonDocument(response).toJson(), QHttpServerResponse::StatusCode(statusCode));
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Get user locations
    m_server->route("/users/<arg>/locations", QHttpServerRequest::Method::Get,
                   [this](const QString &userId) {
        QJsonArray locations = m_locationManager->getUserLocations(userId);

        QJsonObject response;
        response["locations"] = locations;
        response["count"] = locations.size();
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "location_service";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Location Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start Location Service on port" << port;
        return false;
    }
}

void LocationService::stop()
{
    m_server->disconnect();
    qDebug() << "Location Service stopped";
}

