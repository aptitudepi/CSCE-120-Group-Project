#include "services/WeatherbitService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QtMath>
#include <QTimeZone>

const QString WeatherbitService::API_BASE_URL = QStringLiteral("https://api.weatherbit.io/v2.0");

namespace {
static constexpr double MS_TO_MPH = 2.23693629;
static constexpr double KM_TO_MILES = 0.621371;
}

WeatherbitService::WeatherbitService(QObject* parent)
    : WeatherService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

WeatherbitService::~WeatherbitService()
{
    cancelActiveRequests();
}

void WeatherbitService::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey.trimmed();
}

void WeatherbitService::cancelActiveRequests()
{
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

QNetworkRequest WeatherbitService::buildRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    request.setRawHeader("Accept", "application/json");
    return request;
}

QUrl WeatherbitService::buildForecastUrl(double latitude, double longitude) const
{
    QUrl url(API_BASE_URL + "/forecast/hourly");
    QUrlQuery query;
    query.addQueryItem("lat", QString::number(latitude, 'f', 4));
    query.addQueryItem("lon", QString::number(longitude, 'f', 4));
    query.addQueryItem("key", m_apiKey);
    query.addQueryItem("hours", QString::number(48));
    query.addQueryItem("units", "I"); // Imperial units (Fahrenheit, mph)
    url.setQuery(query);
    return url;
}

QUrl WeatherbitService::buildCurrentUrl(double latitude, double longitude) const
{
    QUrl url(API_BASE_URL + "/current");
    QUrlQuery query;
    query.addQueryItem("lat", QString::number(latitude, 'f', 4));
    query.addQueryItem("lon", QString::number(longitude, 'f', 4));
    query.addQueryItem("key", m_apiKey);
    query.addQueryItem("units", "I");
    url.setQuery(query);
    return url;
}

void WeatherbitService::registerReply(QNetworkReply* reply,
                                      ReplyType type,
                                      double latitude,
                                      double longitude)
{
    if (!reply) {
        return;
    }

    reply->setProperty("replyType", static_cast<int>(type));
    reply->setProperty("latitude", latitude);
    reply->setProperty("longitude", longitude);

    m_activeReplies.insert(reply);
    reply->setParent(this);

    connect(reply, &QNetworkReply::finished, this, &WeatherbitService::onReplyFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &WeatherbitService::onNetworkError);
}

void WeatherbitService::unregisterReply(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }
    if (m_activeReplies.contains(reply)) {
        reply->disconnect(this);
        m_activeReplies.remove(reply);
    }
}

void WeatherbitService::fetchForecast(double latitude, double longitude)
{
    if (m_apiKey.isEmpty()) {
        emit error("Weatherbit API key not set");
        return;
    }

    QNetworkRequest request = buildRequest(buildForecastUrl(latitude, longitude));
    QNetworkReply* reply = m_networkManager->get(request);
    registerReply(reply, ForecastReply, latitude, longitude);
}

void WeatherbitService::fetchCurrent(double latitude, double longitude)
{
    if (m_apiKey.isEmpty()) {
        emit error("Weatherbit API key not set");
        return;
    }

    QNetworkRequest request = buildRequest(buildCurrentUrl(latitude, longitude));
    QNetworkReply* reply = m_networkManager->get(request);
    registerReply(reply, CurrentReply, latitude, longitude);
}

void WeatherbitService::onReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    unregisterReply(reply);

    const ReplyType type = static_cast<ReplyType>(reply->property("replyType").toInt());
    const double lat = reply->property("latitude").toDouble();
    const double lon = reply->property("longitude").toDouble();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
        reply->deleteLater();
        return;
    }

    const QByteArray payload = reply->readAll();
    if (payload.isEmpty()) {
        emit error("Weatherbit returned an empty response");
        reply->deleteLater();
        return;
    }

    if (type == ForecastReply) {
        QList<WeatherData*> forecasts = parseForecastPayload(payload, lat, lon);
        if (forecasts.isEmpty()) {
            emit error("Weatherbit forecast response did not include data");
        } else {
            emit forecastReady(forecasts);
        }
    } else {
        WeatherData* current = parseCurrentPayload(payload, lat, lon);
        if (!current) {
            emit error("Weatherbit current response did not include data");
        } else {
            emit currentReady(current);
        }
    }

    reply->deleteLater();
}

void WeatherbitService::onNetworkError(QNetworkReply::NetworkError)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    unregisterReply(reply);
    emit error(reply->errorString());
    reply->deleteLater();
}

QList<WeatherData*> WeatherbitService::parseForecastPayload(const QByteArray& payload,
                                                            double latitude,
                                                            double longitude)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QString message = QStringLiteral("Invalid Weatherbit forecast payload");
        if (parseError.error != QJsonParseError::NoError) {
            message += QStringLiteral(": %1").arg(parseError.errorString());
        }
        emit error(message);
        return {};
    }

    QJsonArray dataArray = doc.object().value("data").toArray();
    QList<WeatherData*> results;
    results.reserve(dataArray.size());

    for (const QJsonValue& value : dataArray) {
        WeatherData* dataPoint = parseDataPoint(value.toObject(), latitude, longitude);
        if (dataPoint) {
            results.append(dataPoint);
        }
    }

    return results;
}

WeatherData* WeatherbitService::parseCurrentPayload(const QByteArray& payload,
                                                    double latitude,
                                                    double longitude)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QString message = QStringLiteral("Invalid Weatherbit current payload");
        if (parseError.error != QJsonParseError::NoError) {
            message += QStringLiteral(": %1").arg(parseError.errorString());
        }
        emit error(message);
        return nullptr;
    }

    QJsonArray dataArray = doc.object().value("data").toArray();
    if (dataArray.isEmpty()) {
        emit error("Weatherbit current payload is empty");
        return nullptr;
    }

    return parseDataPoint(dataArray.first().toObject(), latitude, longitude);
}

WeatherData* WeatherbitService::parseDataPoint(const QJsonObject& obj,
                                               double latitude,
                                               double longitude) const
{
    if (obj.isEmpty()) {
        return nullptr;
    }

    WeatherData* data = new WeatherData();
    data->setLatitude(latitude);
    data->setLongitude(longitude);

    // Timestamp handling
    QDateTime timestamp = QDateTime::fromString(obj.value("timestamp_utc").toString(), Qt::ISODate);
    if (!timestamp.isValid()) {
        const double tsSeconds = obj.value("ts").toDouble();
        if (tsSeconds > 0) {
            timestamp = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(tsSeconds), QTimeZone::UTC);
        }
    } else {
        timestamp.setTimeZone(QTimeZone::UTC);
    }
    if (!timestamp.isValid()) {
        timestamp = QDateTime::currentDateTimeUtc();
    }
    data->setTimestamp(timestamp.toLocalTime());

    data->setTemperature(obj.value("temp").toDouble());
    if (obj.contains("app_temp")) {
        data->setFeelsLike(obj.value("app_temp").toDouble());
    } else {
        data->setFeelsLike(data->temperature());
    }

    data->setHumidity(qFloor(obj.value("rh").toDouble()));
    data->setPressure(obj.value("pres").toDouble());
    data->setWindSpeed(obj.value("wind_spd").toDouble() * MS_TO_MPH);
    data->setWindDirection(obj.value("wind_dir").toInt());

    const double pop = obj.value("pop").toDouble();
    data->setPrecipProbability(qBound(0.0, pop / 100.0, 1.0));
    data->setPrecipIntensity(qMax(0.0, obj.value("precip").toDouble()));

    data->setCloudCover(qFloor(obj.value("clouds").toDouble()));
    data->setVisibility(qFloor(obj.value("vis").toDouble() * KM_TO_MILES));
    data->setUvIndex(qFloor(obj.value("uv").toDouble()));

    const QJsonObject weatherObj = obj.value("weather").toObject();
    const QString description = weatherObj.value("description").toString();
    data->setWeatherCondition(description);
    data->setWeatherDescription(description);

    return data;
}


