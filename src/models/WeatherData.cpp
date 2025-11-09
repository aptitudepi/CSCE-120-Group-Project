#include "models/WeatherData.h"
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

WeatherData::WeatherData(QObject *parent)
    : QObject(parent)
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_timestamp(QDateTime::currentDateTime())
    , m_temperature(0.0)
    , m_feelsLike(0.0)
    , m_humidity(0)
    , m_pressure(0.0)
    , m_windSpeed(0.0)
    , m_windDirection(0)
    , m_precipProbability(0.0)
    , m_precipIntensity(0.0)
    , m_cloudCover(0)
    , m_visibility(0)
    , m_uvIndex(0)
    , m_weatherCondition("")
    , m_weatherDescription("")
{
}

WeatherData::~WeatherData() = default;

void WeatherData::setLatitude(double lat) {
    if (qAbs(m_latitude - lat) > 0.0001) {
        m_latitude = lat;
        emit latitudeChanged();
    }
}

void WeatherData::setLongitude(double lon) {
    if (qAbs(m_longitude - lon) > 0.0001) {
        m_longitude = lon;
        emit longitudeChanged();
    }
}

void WeatherData::setTimestamp(const QDateTime& dt) {
    if (m_timestamp != dt) {
        m_timestamp = dt;
        emit timestampChanged();
    }
}

void WeatherData::setTemperature(double temp) {
    if (qAbs(m_temperature - temp) > 0.01) {
        m_temperature = temp;
        emit temperatureChanged();
    }
}

void WeatherData::setFeelsLike(double feels) {
    if (qAbs(m_feelsLike - feels) > 0.01) {
        m_feelsLike = feels;
        emit feelsLikeChanged();
    }
}

void WeatherData::setHumidity(int humid) {
    if (m_humidity != humid) {
        m_humidity = humid;
        emit humidityChanged();
    }
}

void WeatherData::setPressure(double press) {
    if (qAbs(m_pressure - press) > 0.1) {
        m_pressure = press;
        emit pressureChanged();
    }
}

void WeatherData::setWindSpeed(double speed) {
    if (qAbs(m_windSpeed - speed) > 0.01) {
        m_windSpeed = speed;
        emit windSpeedChanged();
    }
}

void WeatherData::setWindDirection(int direction) {
    if (m_windDirection != direction) {
        m_windDirection = direction;
        emit windDirectionChanged();
    }
}

void WeatherData::setPrecipProbability(double prob) {
    if (qAbs(m_precipProbability - prob) > 0.01) {
        m_precipProbability = prob;
        emit precipProbabilityChanged();
    }
}

void WeatherData::setPrecipIntensity(double intensity) {
    if (qAbs(m_precipIntensity - intensity) > 0.001) {
        m_precipIntensity = intensity;
        emit precipIntensityChanged();
    }
}

void WeatherData::setCloudCover(int cover) {
    if (m_cloudCover != cover) {
        m_cloudCover = cover;
        emit cloudCoverChanged();
    }
}

void WeatherData::setVisibility(int vis) {
    if (m_visibility != vis) {
        m_visibility = vis;
        emit visibilityChanged();
    }
}

void WeatherData::setUvIndex(int uv) {
    if (m_uvIndex != uv) {
        m_uvIndex = uv;
        emit uvIndexChanged();
    }
}

void WeatherData::setWeatherCondition(const QString& condition) {
    if (m_weatherCondition != condition) {
        m_weatherCondition = condition;
        emit weatherConditionChanged();
    }
}

void WeatherData::setWeatherDescription(const QString& description) {
    if (m_weatherDescription != description) {
        m_weatherDescription = description;
        emit weatherDescriptionChanged();
    }
}

QJsonObject WeatherData::toJson() const {
    QJsonObject obj;
    obj["latitude"] = m_latitude;
    obj["longitude"] = m_longitude;
    obj["timestamp"] = m_timestamp.toString(Qt::ISODate);
    obj["temperature"] = m_temperature;
    obj["feelsLike"] = m_feelsLike;
    obj["humidity"] = m_humidity;
    obj["pressure"] = m_pressure;
    obj["windSpeed"] = m_windSpeed;
    obj["windDirection"] = m_windDirection;
    obj["precipProbability"] = m_precipProbability;
    obj["precipIntensity"] = m_precipIntensity;
    obj["cloudCover"] = m_cloudCover;
    obj["visibility"] = m_visibility;
    obj["uvIndex"] = m_uvIndex;
    obj["weatherCondition"] = m_weatherCondition;
    obj["weatherDescription"] = m_weatherDescription;
    return obj;
}

WeatherData* WeatherData::fromJson(const QJsonObject& json, QObject* parent) {
    WeatherData* data = new WeatherData(parent);
    
    if (json.contains("latitude")) {
        data->setLatitude(json["latitude"].toDouble());
    }
    if (json.contains("longitude")) {
        data->setLongitude(json["longitude"].toDouble());
    }
    if (json.contains("timestamp")) {
        data->setTimestamp(QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate));
    }
    if (json.contains("temperature")) {
        data->setTemperature(json["temperature"].toDouble());
    }
    if (json.contains("feelsLike")) {
        data->setFeelsLike(json["feelsLike"].toDouble());
    }
    if (json.contains("humidity")) {
        data->setHumidity(json["humidity"].toInt());
    }
    if (json.contains("pressure")) {
        data->setPressure(json["pressure"].toDouble());
    }
    if (json.contains("windSpeed")) {
        data->setWindSpeed(json["windSpeed"].toDouble());
    }
    if (json.contains("windDirection")) {
        data->setWindDirection(json["windDirection"].toInt());
    }
    if (json.contains("precipProbability")) {
        data->setPrecipProbability(json["precipProbability"].toDouble());
    }
    if (json.contains("precipIntensity")) {
        data->setPrecipIntensity(json["precipIntensity"].toDouble());
    }
    if (json.contains("cloudCover")) {
        data->setCloudCover(json["cloudCover"].toInt());
    }
    if (json.contains("visibility")) {
        data->setVisibility(json["visibility"].toInt());
    }
    if (json.contains("uvIndex")) {
        data->setUvIndex(json["uvIndex"].toInt());
    }
    if (json.contains("weatherCondition")) {
        data->setWeatherCondition(json["weatherCondition"].toString());
    }
    if (json.contains("weatherDescription")) {
        data->setWeatherDescription(json["weatherDescription"].toString());
    }
    
    return data;
}

