#include "services/PirateWeatherService.h"
#include "models/WeatherData.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QList>
#include <QVariant>
#include <QJsonParseError>
#include <QtGlobal>

const QString PirateWeatherService::BASE_URL = "https://api.pirateweather.net/forecast";

PirateWeatherService::PirateWeatherService(QObject *parent)
    : WeatherService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Try to get API key from environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("PIRATE_WEATHER_API_KEY")) {
        m_apiKey = env.value("PIRATE_WEATHER_API_KEY");
    }
}

PirateWeatherService::~PirateWeatherService() {
    abortActiveRequests();
}

void PirateWeatherService::setApiKey(const QString& apiKey) {
    m_apiKey = apiKey;
}

void PirateWeatherService::cancelActiveRequests() {
    abortActiveRequests();
}

void PirateWeatherService::fetchForecast(double latitude, double longitude) {
    if (m_apiKey.isEmpty()) {
        emit error("Pirate Weather API key not set");
        return;
    }
    
    QString url = QString("%1/%2/%3,%4")
        .arg(BASE_URL, m_apiKey, QString::number(latitude, 'f', 4), QString::number(longitude, 'f', 4));
    
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(30000); // 30 second timeout
    
    // abortActiveRequests(); // REMOVED: Do not abort concurrent requests for grid processing
    
    QNetworkReply* reply = m_networkManager->get(request);
    if (!reply) {
        emit error("Failed to start Pirate Weather request");
        return;
    }
    
    reply->setParent(this);
    m_activeReplies.insert(reply);
    
    connect(reply, &QNetworkReply::finished,
            this, &PirateWeatherService::onForecastReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred,
            this, &PirateWeatherService::onNetworkError);
    reply->setProperty("latitude", latitude);
    reply->setProperty("longitude", longitude);
}

void PirateWeatherService::fetchCurrent(double latitude, double longitude) {
    // Same as forecast for Pirate Weather
    fetchForecast(latitude, longitude);
}

void PirateWeatherService::onForecastReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    unregisterReply(reply);
    
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }
    
    double lat = reply->property("latitude").toDouble();
    double lon = reply->property("longitude").toDouble();
    
    QByteArray data = reply->readAll();
    
    // Check receivers in main thread
    const bool hasMinuteReceivers = receivers(SIGNAL(minuteForecastReady(QList<WeatherData*>))) > 0;
    
    // Offload parsing to background thread
    (void)QtConcurrent::run([this, data, lat, lon, hasMinuteReceivers]() {
        parseForecastResponse(data, lat, lon, hasMinuteReceivers);
    });
    
    reply->deleteLater();
}

void PirateWeatherService::onNetworkError(QNetworkReply::NetworkError networkError) {
    Q_UNUSED(networkError)
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        unregisterReply(reply);
        emit error(reply->errorString());
        reply->deleteLater();
    }
}

void PirateWeatherService::parseForecastResponse(const QByteArray& data, double lat, double lon, bool hasMinuteReceivers) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QString message = "Invalid forecast response";
        if (parseError.error != QJsonParseError::NoError) {
            message += QString(": %1").arg(parseError.errorString());
        }
        emit error(message);
        return;
    }
    
    QJsonObject obj = doc.object();
    
    QList<WeatherData*> forecasts;
    
    // Parse hourly forecast
    if (obj.contains("hourly") && obj["hourly"].isObject()) {
        QJsonObject hourly = obj["hourly"].toObject();
        if (hourly.contains("data") && hourly["data"].isArray()) {
            QList<WeatherData*> hourlyData = parseHourlyData(hourly["data"].toArray(), lat, lon);
            forecasts.append(hourlyData);
        }
    }
    
    // Parse minutely forecast for nowcasting
    // const bool hasMinuteReceivers = receivers(SIGNAL(minuteForecastReady(QList<WeatherData*>))) > 0; // Moved to main thread
    
    if (hasMinuteReceivers && obj.contains("minutely") && obj["minutely"].isObject()) {
        QJsonObject minutely = obj["minutely"].toObject();
        if (minutely.contains("data") && minutely["data"].isArray()) {
            QList<WeatherData*> minutelyData = parseMinutelyData(minutely["data"].toArray(), lat, lon);
            emit minuteForecastReady(minutelyData);
        }
    }
    
    // Parse currently (if available)
    if (obj.contains("currently") && obj["currently"].isObject()) {
        WeatherData* current = parseDataPoint(obj["currently"].toObject(), lat, lon);
        if (current) {
            emit currentReady(current);
        }
    }
    
    if (!forecasts.isEmpty()) {
        emit forecastReady(forecasts);
    }
}

QList<WeatherData*> PirateWeatherService::parseHourlyData(const QJsonArray& hourly, double lat, double lon) {
    QList<WeatherData*> forecasts;
    
    for (const QJsonValue& value : hourly) {
        QJsonObject point = value.toObject();
        WeatherData* data = parseDataPoint(point, lat, lon);
        if (data) {
            forecasts.append(data);
        }
    }
    
    return forecasts;
}

QList<WeatherData*> PirateWeatherService::parseMinutelyData(const QJsonArray& minutely, double lat, double lon) {
    QList<WeatherData*> forecasts;
    
    for (const QJsonValue& value : minutely) {
        QJsonObject point = value.toObject();
        WeatherData* data = parseDataPoint(point, lat, lon);
        if (data) {
            forecasts.append(data);
        }
    }
    
    return forecasts;
}

WeatherData* PirateWeatherService::parseDataPoint(const QJsonObject& point, double lat, double lon) {
    WeatherData* data = new WeatherData();
    data->setLatitude(lat);
    data->setLongitude(lon);
    
    // Parse timestamp
    if (point.contains("time")) {
        qint64 timestamp = point["time"].toVariant().toLongLong();
        QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
        data->setTimestamp(dt);
    }
    
    // Temperature
    if (point.contains("temperature")) {
        data->setTemperature(point["temperature"].toDouble());
    }
    
    // Apparent temperature
    if (point.contains("apparentTemperature")) {
        data->setFeelsLike(point["apparentTemperature"].toDouble());
    }
    
    // Humidity
    if (point.contains("humidity")) {
        data->setHumidity(static_cast<int>(point["humidity"].toDouble() * 100.0));
    }
    
    // Pressure
    if (point.contains("pressure")) {
        data->setPressure(point["pressure"].toDouble());
    }
    
    // Wind
    if (point.contains("windSpeed")) {
        data->setWindSpeed(point["windSpeed"].toDouble());
    }
    if (point.contains("windBearing")) {
        data->setWindDirection(point["windBearing"].toInt());
    }
    
    // Precipitation
    if (point.contains("precipProbability")) {
        data->setPrecipProbability(point["precipProbability"].toDouble());
    }
    if (point.contains("precipIntensity")) {
        data->setPrecipIntensity(point["precipIntensity"].toDouble());
    }
    
    // Cloud cover
    if (point.contains("cloudCover")) {
        data->setCloudCover(static_cast<int>(point["cloudCover"].toDouble() * 100.0));
    }
    
    // Visibility
    if (point.contains("visibility")) {
        data->setVisibility(static_cast<int>(point["visibility"].toDouble() * 10.0)); // Convert km to 0.1km units
    }
    
    // UV Index
    if (point.contains("uvIndex")) {
        data->setUvIndex(point["uvIndex"].toInt());
    }
    
    // Weather condition
    if (point.contains("summary")) {
        data->setWeatherDescription(point["summary"].toString());
        data->setWeatherCondition(point["summary"].toString());
    }
    if (point.contains("icon")) {
        QString icon = point["icon"].toString();
        data->setWeatherCondition(icon);
    }
    
    return data;
}

void PirateWeatherService::abortActiveRequests() {
    if (m_activeReplies.isEmpty()) {
        return;
    }
    
    const auto replies = m_activeReplies;
    for (QNetworkReply* reply : replies) {
        if (!reply) {
            continue;
        }
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    m_activeReplies.clear();
}

void PirateWeatherService::unregisterReply(QNetworkReply* reply) {
    if (!reply) {
        return;
    }
    if (m_activeReplies.contains(reply)) {
        reply->disconnect(this);
        m_activeReplies.remove(reply);
    }
}

