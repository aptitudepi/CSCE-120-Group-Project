#include "services/NWSService.h"
#include "models/WeatherData.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QList>
#include <QStringList>

const QString NWSService::BASE_URL = "https://api.weather.gov";

NWSService::NWSService(QObject *parent)
    : WeatherService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Network error:" << reply->errorString();
            emit error(reply->errorString());
            reply->deleteLater();
            return;
        }
    });
}

NWSService::~NWSService() = default;

void NWSService::fetchForecast(double latitude, double longitude) {
    QString cacheKey = QString("%1_%2").arg(latitude, 0, 'f', 4).arg(longitude, 0, 'f', 4);
    
    // First, get gridpoint if we don't have it
    if (!m_gridpointCache.contains(cacheKey)) {
        fetchGridpoint(latitude, longitude);
        // Store lat/lon for later use
        m_gridpointCache[cacheKey].office = ""; // Placeholder
        return;
    }
    
    Gridpoint gridpoint = m_gridpointCache[cacheKey];
    if (!gridpoint.isValid()) {
        fetchGridpoint(latitude, longitude);
        return;
    }
    
    // Build forecast URL
    QString forecastUrl = QString("%1/gridpoints/%2/%3,%4/forecast")
        .arg(BASE_URL, gridpoint.office, QString::number(gridpoint.x), QString::number(gridpoint.y));
    
    QNetworkRequest request{QUrl(forecastUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    request.setRawHeader("Accept", "application/json");
    
    // Use conditional GET if we have last modified time
    QString lastModifiedKey = QString("forecast_%1").arg(cacheKey);
    if (m_lastModifiedCache.contains(lastModifiedKey)) {
        QDateTime lastModified = m_lastModifiedCache[lastModifiedKey];
        request.setRawHeader("If-Modified-Since", lastModified.toUTC().toString(Qt::RFC2822Date).toUtf8());
    }
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &NWSService::onForecastReplyFinished);
    reply->setProperty("latitude", latitude);
    reply->setProperty("longitude", longitude);
}

void NWSService::fetchCurrent(double latitude, double longitude) {
    // NWS doesn't have a direct "current" endpoint, use hourly forecast
    fetchForecast(latitude, longitude);
}

void NWSService::fetchGridpoint(double latitude, double longitude) {
    QString pointsUrl = QString("%1/points/%2,%3")
        .arg(BASE_URL, QString::number(latitude, 'f', 4), QString::number(longitude, 'f', 4));
    
    QNetworkRequest request{QUrl(pointsUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &NWSService::onPointsReplyFinished);
    reply->setProperty("latitude", latitude);
    reply->setProperty("longitude", longitude);
}

void NWSService::fetchAlerts(double latitude, double longitude) {
    QString alertsUrl = QString("%1/alerts/active?point=%2,%3")
        .arg(BASE_URL, QString::number(latitude, 'f', 4), QString::number(longitude, 'f', 4));
    
    QNetworkRequest request{QUrl(alertsUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    request.setRawHeader("Accept", "application/json");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &NWSService::onAlertsReplyFinished);
}

void NWSService::onPointsReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    double lat = reply->property("latitude").toDouble();
    double lon = reply->property("longitude").toDouble();
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Points request error:" << reply->errorString();
        emit error(reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    parsePointsResponse(data, lat, lon);
    
    // Now fetch forecast with the gridpoint
    fetchForecast(lat, lon);
    
    reply->deleteLater();
}

void NWSService::onForecastReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            // 304 Not Modified - use cached data
            qDebug() << "Forecast not modified, using cache";
        } else {
            qWarning() << "Forecast request error:" << reply->errorString();
            emit error(reply->errorString());
        }
        reply->deleteLater();
        return;
    }
    
    // Check for 304 Not Modified
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 304) {
        qDebug() << "Forecast not modified (304)";
        reply->deleteLater();
        return;
    }
    
    // Update Last-Modified cache
    QVariant lastModified = reply->header(QNetworkRequest::LastModifiedHeader);
    if (lastModified.isValid()) {
        QString cacheKey = QString("%1_%2").arg(
            reply->property("latitude").toDouble(), 0, 'f', 4)
            .arg(reply->property("longitude").toDouble(), 0, 'f', 4);
        m_lastModifiedCache[QString("forecast_%1").arg(cacheKey)] = lastModified.toDateTime();
    }
    
    double lat = reply->property("latitude").toDouble();
    double lon = reply->property("longitude").toDouble();
    QByteArray data = reply->readAll();
    parseForecastResponse(data, lat, lon);
    
    reply->deleteLater();
}

void NWSService::onHourlyReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Hourly forecast error:" << reply->errorString();
        emit error(reply->errorString());
        reply->deleteLater();
        return;
    }
    
    double lat = reply->property("latitude").toDouble();
    double lon = reply->property("longitude").toDouble();
    QByteArray data = reply->readAll();
    parseHourlyForecastResponse(data, lat, lon);
    
    reply->deleteLater();
}

void NWSService::onAlertsReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Alerts request error:" << reply->errorString();
        emit error(reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    parseAlertsResponse(data);
    
    reply->deleteLater();
}

void NWSService::onNetworkError(QNetworkReply::NetworkError networkError) {
    Q_UNUSED(networkError)
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        emit error(reply->errorString());
    }
}

void NWSService::parsePointsResponse(const QByteArray& data, double lat, double lon) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid points response");
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonObject props = obj["properties"].toObject();
    
    QString forecastUrl = props["forecast"].toString();
    if (forecastUrl.isEmpty()) {
        emit error("No forecast URL in response");
        return;
    }
    
    // Extract office, x, y from forecast URL
    // Format: https://api.weather.gov/gridpoints/{office}/{x},{y}/forecast
    QStringList parts = forecastUrl.split('/');
    if (parts.size() < 6) {
        emit error("Invalid forecast URL format");
        return;
    }
    
    QString office = parts[parts.size() - 3];
    QStringList grid = parts[parts.size() - 2].split(',');
    if (grid.size() != 2) {
        emit error("Invalid gridpoint format");
        return;
    }
    
    bool ok;
    int x = grid[0].toInt(&ok);
    if (!ok) {
        emit error("Invalid gridpoint X coordinate");
        return;
    }
    int y = grid[1].toInt(&ok);
    if (!ok) {
        emit error("Invalid gridpoint Y coordinate");
        return;
    }
    
    QString cacheKey = QString("%1_%2").arg(lat, 0, 'f', 4).arg(lon, 0, 'f', 4);
    Gridpoint gridpoint;
    gridpoint.office = office;
    gridpoint.x = x;
    gridpoint.y = y;
    gridpoint.lastModified = QDateTime::currentDateTime();
    m_gridpointCache[cacheKey] = gridpoint;
    
    emit gridpointReady(office, x, y);
}

void NWSService::parseForecastResponse(const QByteArray& data, double lat, double lon) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid forecast response");
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonObject props = obj["properties"].toObject();
    QJsonArray periods = props["periods"].toArray();
    
    QList<WeatherData*> forecasts = parsePeriods(periods, lat, lon);
    
    // Set coordinates for all forecasts
    for (WeatherData* forecast : forecasts) {
        forecast->setLatitude(lat);
        forecast->setLongitude(lon);
    }
    
    emit forecastReady(forecasts);
}

void NWSService::parseHourlyForecastResponse(const QByteArray& data, double lat, double lon) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid hourly forecast response");
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonObject props = obj["properties"].toObject();
    QJsonArray periods = props["periods"].toArray();
    
    QList<WeatherData*> forecasts = parsePeriods(periods, lat, lon);
    for (WeatherData* forecast : forecasts) {
        forecast->setLatitude(lat);
        forecast->setLongitude(lon);
    }
    
    emit forecastReady(forecasts);
}

void NWSService::parseAlertsResponse(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid alerts response");
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonArray features = obj["features"].toArray();
    
    QList<QJsonObject> alerts;
    for (const QJsonValue& value : features) {
        alerts.append(value.toObject());
    }
    
    emit alertsReady(alerts);
}

QList<WeatherData*> NWSService::parsePeriods(const QJsonArray& periods, double lat, double lon) {
    QList<WeatherData*> forecasts;
    
    for (const QJsonValue& value : periods) {
        QJsonObject period = value.toObject();
        WeatherData* forecast = parsePeriod(period, lat, lon);
        if (forecast) {
            forecasts.append(forecast);
        }
    }
    
    return forecasts;
}

WeatherData* NWSService::parsePeriod(const QJsonObject& period, double lat, double lon) {
    WeatherData* data = new WeatherData();
    data->setLatitude(lat);
    data->setLongitude(lon);
    
    // Parse timestamp
    QString startTimeStr = period["startTime"].toString();
    QDateTime startTime = QDateTime::fromString(startTimeStr, Qt::ISODate);
    data->setTimestamp(startTime);
    
    // Temperature
    if (period.contains("temperature")) {
        data->setTemperature(period["temperature"].toDouble());
    }
    
    // Wind
    QString windSpeed = period["windSpeed"].toString();
    // Parse wind speed (e.g., "5 to 10 mph")
    QStringList speedParts = windSpeed.split(" ");
    if (!speedParts.isEmpty()) {
        bool ok;
        double speed = speedParts[0].toDouble(&ok);
        if (ok) {
            data->setWindSpeed(speed);
        }
    }
    
    QString windDirection = period["windDirection"].toString();
    // Convert direction string to degrees (simplified)
    if (windDirection == "N") data->setWindDirection(0);
    else if (windDirection == "NE") data->setWindDirection(45);
    else if (windDirection == "E") data->setWindDirection(90);
    else if (windDirection == "SE") data->setWindDirection(135);
    else if (windDirection == "S") data->setWindDirection(180);
    else if (windDirection == "SW") data->setWindDirection(225);
    else if (windDirection == "W") data->setWindDirection(270);
    else if (windDirection == "NW") data->setWindDirection(315);
    
    // Precipitation
    if (period.contains("probabilityOfPrecipitation")) {
        QJsonObject pop = period["probabilityOfPrecipitation"].toObject();
        data->setPrecipProbability(pop["value"].toDouble() / 100.0);
    }
    
    // Weather condition
    QString shortForecast = period["shortForecast"].toString();
    data->setWeatherCondition(shortForecast);
    
    QString detailedForecast = period["detailedForecast"].toString();
    data->setWeatherDescription(detailedForecast);
    
    // Humidity (if available)
    if (period.contains("relativeHumidity")) {
        QJsonObject rh = period["relativeHumidity"].toObject();
        data->setHumidity(rh["value"].toInt());
    }
    
    return data;
}

