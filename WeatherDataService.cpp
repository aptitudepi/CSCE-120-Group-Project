// WeatherDataService.cpp
#include "WeatherDataService.h"
#include <QCoreApplication>
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>

WeatherDataCollector::WeatherDataCollector(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pendingRequests(0)
{
    QSettings settings;
    m_pirateWeatherKey = settings.value("api_keys/pirate_weather", "demo-key").toString();
}

void WeatherDataCollector::collectAllSources(double lat, double lon)
{
    m_currentLat = lat;
    m_currentLon = lon;
    m_sourcesData = QJsonArray();
    m_pendingRequests = 0;

    // Start all requests
    QJsonObject pirateData = getPirateWeatherData(lat, lon);
    if (!pirateData.isEmpty()) {
        QJsonObject sourceData;
        sourceData["source"] = "pirate_weather";
        sourceData["data"] = pirateData;
        m_sourcesData.append(sourceData);
    }

    QJsonObject nwsData = getNWSData(lat, lon);
    if (!nwsData.isEmpty()) {
        QJsonObject sourceData;
        sourceData["source"] = "nws";
        sourceData["data"] = nwsData;
        m_sourcesData.append(sourceData);
    }

    QJsonObject openMeteoData = getOpenMeteoData(lat, lon);
    if (!openMeteoData.isEmpty()) {
        QJsonObject sourceData;
        sourceData["source"] = "openmeteo";
        sourceData["data"] = openMeteoData;
        m_sourcesData.append(sourceData);
    }

    // Emit collected data
    QJsonObject result;
    result["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    result["latitude"] = lat;
    result["longitude"] = lon;
    result["sources"] = m_sourcesData;

    emit dataReady(result);
}

QJsonObject WeatherDataCollector::getPirateWeatherData(double lat, double lon)
{
    if (m_pirateWeatherKey == "demo-key") {
        qWarning() << "Using demo Pirate Weather key - set proper key in settings";
        return QJsonObject(); // Return empty if no valid key
    }

    QString urlString = QString("https://api.pirateweather.net/forecast/%1/%2,%3")
                           .arg(m_pirateWeatherKey)
                           .arg(lat, 0, 'f', 6)
                           .arg(lon, 0, 'f', 6);

    QUrl url(urlString);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");

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
        result = doc.object();
    } else {
        qWarning() << "Pirate Weather API error:" << reply->errorString();
    }

    reply->deleteLater();
    return result;
}

QJsonObject WeatherDataCollector::getNWSData(double lat, double lon)
{
    // NWS API - get grid point first
    QString pointsUrl = QString("https://api.weather.gov/points/%1,%2")
                           .arg(lat, 0, 'f', 6)
                           .arg(lon, 0, 'f', 6);

    QNetworkRequest request(QUrl(pointsUrl));
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
        QJsonObject pointsData = doc.object();

        // Try to get observation stations
        QJsonObject properties = pointsData["properties"].toObject();
        if (properties.contains("observationStations")) {
            QString stationsUrl = properties["observationStations"].toString();

            // Get stations and latest observation (simplified)
            QNetworkRequest stationRequest(QUrl(stationsUrl));
            stationRequest.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0 (contact@example.com)");

            QNetworkReply *stationReply = m_networkManager->get(stationRequest);
            connect(stationReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            timer.start(10000);
            loop.exec();

            if (stationReply->error() == QNetworkReply::NoError && timer.isActive()) {
                QJsonDocument stationDoc = QJsonDocument::fromJson(stationReply->readAll());
                result = stationDoc.object();
            }
            stationReply->deleteLater();
        }
    } else {
        qWarning() << "NWS API error:" << reply->errorString();
    }

    reply->deleteLater();
    return result;
}

QJsonObject WeatherDataCollector::getOpenMeteoData(double lat, double lon)
{
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery query;
    query.addQueryItem("latitude", QString::number(lat, 'f', 6));
    query.addQueryItem("longitude", QString::number(lon, 'f', 6));
    query.addQueryItem("current", "temperature_2m,relative_humidity_2m,precipitation,pressure_msl,wind_speed_10m");
    query.addQueryItem("hourly", "temperature_2m,precipitation_probability,precipitation,wind_speed_10m");
    query.addQueryItem("daily", "temperature_2m_max,temperature_2m_min,precipitation_sum,precipitation_probability_max");
    query.addQueryItem("timezone", "auto");

    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");

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
        result = doc.object();
    } else {
        qWarning() << "Open-Meteo API error:" << reply->errorString();
    }

    reply->deleteLater();
    return result;
}

WeatherDataService::WeatherDataService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_collector(new WeatherDataCollector(this))
    , m_settings(new QSettings(this))
{
    qDebug() << "Weather Data Service initialized";
}

bool WeatherDataService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Weather Data Collection Service";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Current weather endpoint
    m_server->route("/current/<arg>/<arg>", QHttpServerRequest::Method::Get,
                   [this](double lat, double lon) {
        m_collector->collectAllSources(lat, lon);

        // Wait for data (simplified synchronous version)
        QEventLoop loop;
        QJsonObject responseData;

        connect(m_collector, &WeatherDataCollector::dataReady, [&](const QJsonObject &data) {
            responseData = data;
            loop.quit();
        });

        connect(m_collector, &WeatherDataCollector::errorOccurred, [&](const QString &error) {
            responseData["error"] = error;
            loop.quit();
        });

        QTimer::singleShot(15000, &loop, &QEventLoop::quit); // 15 second timeout
        loop.exec();

        if (responseData.isEmpty()) {
            responseData["error"] = "Request timeout";
        }

        QHttpServerResponse httpResponse(QJsonDocument(responseData).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Forecast endpoint (same as current for now)
    m_server->route("/forecast/<arg>/<arg>", QHttpServerRequest::Method::Get,
                   [this](double lat, double lon) {
        m_collector->collectAllSources(lat, lon);

        QEventLoop loop;
        QJsonObject responseData;

        connect(m_collector, &WeatherDataCollector::dataReady, [&](const QJsonObject &data) {
            responseData = data;
            loop.quit();
        });

        connect(m_collector, &WeatherDataCollector::errorOccurred, [&](const QString &error) {
            responseData["error"] = error;
            loop.quit();
        });

        QTimer::singleShot(15000, &loop, &QEventLoop::quit);
        loop.exec();

        if (responseData.isEmpty()) {
            responseData["error"] = "Request timeout";
        }

        QHttpServerResponse httpResponse(QJsonDocument(responseData).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "weather_data";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Weather Data Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start Weather Data Service on port" << port;
        return false;
    }
}

void WeatherDataService::stop()
{
    m_server->disconnect();
    qDebug() << "Weather Data Service stopped";
}

