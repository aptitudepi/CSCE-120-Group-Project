
// DataProcessingService.h
#ifndef DATAPROCESSINGSERVICE_H
#define DATAPROCESSINGSERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDateTime>
#include <QDebug>
#include <QtMath>

class WeatherDataProcessor : public QObject
{
    Q_OBJECT

public:
    explicit WeatherDataProcessor(QObject *parent = nullptr);

    QJsonObject fuseData(const QJsonObject &multiSourceData);
    QJsonObject interpolateMissingData(const QJsonObject &data);
    QJsonObject validateData(const QJsonObject &data);
    double calculateQualityScore(const QJsonObject &data);

private:
    QHash<QString, double> m_sourceWeights;

    double extractTemperature(const QJsonObject &sourceData);
    double extractHumidity(const QJsonObject &sourceData);
    double extractPrecipitation(const QJsonObject &sourceData);
    double extractWindSpeed(const QJsonObject &sourceData);
    double weightedAverage(const QList<QPair<double, double>> &valuesWeights);
};

class DataProcessingService : public QObject
{
    Q_OBJECT

public:
    explicit DataProcessingService(QObject *parent = nullptr);
    bool start(int port = 8005);
    void stop();

private:
    QHttpServer *m_server;
    WeatherDataProcessor *m_processor;
    QSettings *m_settings;
};

#endif // DATAPROCESSINGSERVICE_H

// DataProcessingService.cpp
#include "DataProcessingService.h"
#include <QCoreApplication>

WeatherDataProcessor::WeatherDataProcessor(QObject *parent)
    : QObject(parent)
{
    // Initialize source weights
    m_sourceWeights["pirate_weather"] = 0.4;
    m_sourceWeights["nws"] = 0.4;
    m_sourceWeights["openmeteo"] = 0.2;

    qDebug() << "Weather Data Processor initialized";
}

QJsonObject WeatherDataProcessor::fuseData(const QJsonObject &multiSourceData)
{
    QJsonArray sources = multiSourceData["sources"].toArray();

    if (sources.isEmpty()) {
        QJsonObject error;
        error["error"] = "No source data available for fusion";
        return error;
    }

    // Extract values from all sources
    QList<QPair<double, double>> temperatures;
    QList<QPair<double, double>> humidities;
    QList<QPair<double, double>> precipitations;
    QList<QPair<double, double>> windSpeeds;

    QStringList sourceInfo;

    for (const auto &sourceValue : sources) {
        QJsonObject sourceData = sourceValue.toObject();
        QString source = sourceData["source"].toString();
        double weight = m_sourceWeights.value(source, 0.1);

        double temp = extractTemperature(sourceData);
        if (!qIsNaN(temp)) {
            temperatures.append(qMakePair(temp, weight));
        }

        double humidity = extractHumidity(sourceData);
        if (!qIsNaN(humidity)) {
            humidities.append(qMakePair(humidity, weight));
        }

        double precip = extractPrecipitation(sourceData);
        if (!qIsNaN(precip)) {
            precipitations.append(qMakePair(precip, weight));
        }

        double wind = extractWindSpeed(sourceData);
        if (!qIsNaN(wind)) {
            windSpeeds.append(qMakePair(wind, weight));
        }

        sourceInfo.append(source);
    }

    // Calculate weighted averages
    QJsonObject fusedData;
    fusedData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    fusedData["latitude"] = multiSourceData["latitude"];
    fusedData["longitude"] = multiSourceData["longitude"];

    double avgTemp = weightedAverage(temperatures);
    if (!qIsNaN(avgTemp)) {
        fusedData["temperature"] = avgTemp;
    }

    double avgHumidity = weightedAverage(humidities);
    if (!qIsNaN(avgHumidity)) {
        fusedData["humidity"] = avgHumidity;
    }

    double avgPrecip = weightedAverage(precipitations);
    if (!qIsNaN(avgPrecip)) {
        fusedData["precipitation"] = avgPrecip;
    }

    double avgWind = weightedAverage(windSpeeds);
    if (!qIsNaN(avgWind)) {
        fusedData["wind_speed"] = avgWind;
    }

    fusedData["sources_used"] = QJsonArray::fromStringList(sourceInfo);

    // Data quality metrics
    QJsonObject dataQuality;
    dataQuality["temperature_sources"] = temperatures.size();
    dataQuality["humidity_sources"] = humidities.size();
    dataQuality["precipitation_sources"] = precipitations.size();
    dataQuality["wind_sources"] = windSpeeds.size();
    fusedData["data_quality"] = dataQuality;

    return fusedData;
}

QJsonObject WeatherDataProcessor::interpolateMissingData(const QJsonObject &data)
{
    QJsonObject result = data;

    // Simple interpolation - in production, use more sophisticated methods
    if (!data.contains("temperature") && data.contains("humidity")) {
        // Estimate temperature based on humidity (very basic)
        double humidity = data["humidity"].toDouble();
        double estimatedTemp = 25.0 - (humidity - 50.0) * 0.2;
        result["temperature"] = estimatedTemp;
        result["temperature_estimated"] = true;
    }

    if (!data.contains("humidity") && data.contains("temperature")) {
        // Estimate humidity based on temperature (very basic)
        double temp = data["temperature"].toDouble();
        double estimatedHumidity = 50.0 + (25.0 - temp) * 2.0;
        result["humidity"] = qMax(0.0, qMin(100.0, estimatedHumidity));
        result["humidity_estimated"] = true;
    }

    return result;
}

QJsonObject WeatherDataProcessor::validateData(const QJsonObject &data)
{
    QJsonObject validated = data;

    // Temperature validation (-50°C to 60°C)
    if (data.contains("temperature")) {
        double temp = data["temperature"].toDouble();
        if (temp < -50.0 || temp > 60.0) {
            qWarning() << "Temperature out of range:" << temp;
            validated.remove("temperature");
        }
    }

    // Humidity validation (0% to 100%)
    if (data.contains("humidity")) {
        double humidity = data["humidity"].toDouble();
        validated["humidity"] = qMax(0.0, qMin(100.0, humidity));
    }

    // Precipitation validation (>= 0)
    if (data.contains("precipitation")) {
        double precip = data["precipitation"].toDouble();
        validated["precipitation"] = qMax(0.0, precip);
    }

    // Wind speed validation (>= 0, < 200 m/s)
    if (data.contains("wind_speed")) {
        double wind = data["wind_speed"].toDouble();
        if (wind < 0.0 || wind > 200.0) {
            qWarning() << "Wind speed out of range:" << wind;
            validated.remove("wind_speed");
        }
    }

    return validated;
}

double WeatherDataProcessor::calculateQualityScore(const QJsonObject &data)
{
    QJsonObject quality = data["data_quality"].toObject();
    int totalMetrics = 4; // temperature, humidity, precipitation, wind

    int availableMetrics = 0;
    if (quality["temperature_sources"].toInt() > 0) availableMetrics++;
    if (quality["humidity_sources"].toInt() > 0) availableMetrics++;
    if (quality["precipitation_sources"].toInt() > 0) availableMetrics++;
    if (quality["wind_sources"].toInt() > 0) availableMetrics++;

    double baseScore = double(availableMetrics) / totalMetrics;

    // Bonus for multiple sources
    double avgSources = (quality["temperature_sources"].toInt() + 
                        quality["humidity_sources"].toInt() + 
                        quality["precipitation_sources"].toInt() + 
                        quality["wind_sources"].toInt()) / 4.0;

    double multiSourceBonus = qMin(0.2, (avgSources - 1.0) * 0.1);

    return qMin(1.0, baseScore + multiSourceBonus);
}

double WeatherDataProcessor::extractTemperature(const QJsonObject &sourceData)
{
    QString source = sourceData["source"].toString();
    QJsonObject data = sourceData["data"].toObject();

    if (source == "pirate_weather") {
        return data["currently"].toObject()["temperature"].toDouble(qQNaN());
    } else if (source == "nws") {
        QJsonObject props = data["properties"].toObject();
        return props["temperature"].toObject()["value"].toDouble(qQNaN());
    } else if (source == "openmeteo") {
        return data["current"].toObject()["temperature_2m"].toDouble(qQNaN());
    }

    return qQNaN();
}

double WeatherDataProcessor::extractHumidity(const QJsonObject &sourceData)
{
    QString source = sourceData["source"].toString();
    QJsonObject data = sourceData["data"].toObject();

    if (source == "pirate_weather") {
        return data["currently"].toObject()["humidity"].toDouble(qQNaN()) * 100.0;
    } else if (source == "nws") {
        QJsonObject props = data["properties"].toObject();
        return props["relativeHumidity"].toObject()["value"].toDouble(qQNaN());
    } else if (source == "openmeteo") {
        return data["current"].toObject()["relative_humidity_2m"].toDouble(qQNaN());
    }

    return qQNaN();
}

double WeatherDataProcessor::extractPrecipitation(const QJsonObject &sourceData)
{
    QString source = sourceData["source"].toString();
    QJsonObject data = sourceData["data"].toObject();

    if (source == "pirate_weather") {
        return data["currently"].toObject()["precipIntensity"].toDouble(0.0);
    } else if (source == "nws") {
        return 0.0; // NWS doesn't always have real-time precipitation
    } else if (source == "openmeteo") {
        return data["current"].toObject()["precipitation"].toDouble(0.0);
    }

    return 0.0;
}

double WeatherDataProcessor::extractWindSpeed(const QJsonObject &sourceData)
{
    QString source = sourceData["source"].toString();
    QJsonObject data = sourceData["data"].toObject();

    if (source == "pirate_weather") {
        return data["currently"].toObject()["windSpeed"].toDouble(qQNaN());
    } else if (source == "nws") {
        QJsonObject props = data["properties"].toObject();
        return props["windSpeed"].toObject()["value"].toDouble(qQNaN());
    } else if (source == "openmeteo") {
        return data["current"].toObject()["wind_speed_10m"].toDouble(qQNaN());
    }

    return qQNaN();
}

double WeatherDataProcessor::weightedAverage(const QList<QPair<double, double>> &valuesWeights)
{
    if (valuesWeights.isEmpty()) {
        return qQNaN();
    }

    double totalWeight = 0.0;
    double weightedSum = 0.0;

    for (const auto &pair : valuesWeights) {
        weightedSum += pair.first * pair.second;
        totalWeight += pair.second;
    }

    if (totalWeight == 0.0) {
        return qQNaN();
    }

    return weightedSum / totalWeight;
}

DataProcessingService::DataProcessingService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_processor(new WeatherDataProcessor(this))
    , m_settings(new QSettings(this))
{
    qDebug() << "Data Processing Service initialized";
}

bool DataProcessingService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Data Processing Service";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Process endpoint
    m_server->route("/process", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject weatherData = doc.object();

        try {
            // Fuse data from multiple sources
            QJsonObject fusedData = m_processor->fuseData(weatherData);
            if (fusedData.contains("error")) {
                QHttpServerResponse httpResponse(QJsonDocument(fusedData).toJson(), QHttpServerResponse::StatusCode::InternalServerError);
                httpResponse.setHeader("Content-Type", "application/json");
                return httpResponse;
            }

            // Interpolate missing data
            QJsonObject interpolatedData = m_processor->interpolateMissingData(fusedData);

            // Validate the results
            QJsonObject validatedData = m_processor->validateData(interpolatedData);

            // Add processing metadata
            QJsonObject processing;
            processing["fused_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            processing["sources_count"] = weatherData["sources"].toArray().size();
            processing["quality_score"] = m_processor->calculateQualityScore(validatedData);
            validatedData["processing"] = processing;

            QHttpServerResponse httpResponse(QJsonDocument(validatedData).toJson());
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;

        } catch (const std::exception &e) {
            QJsonObject error;
            error["error"] = QString("Processing error: %1").arg(e.what());
            error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::InternalServerError);
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;
        }
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "data_processing";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Data Processing Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start Data Processing Service on port" << port;
        return false;
    }
}

void DataProcessingService::stop()
{
    m_server->disconnect();
    qDebug() << "Data Processing Service stopped";
}

// main.cpp for Data Processing Service
#include <QCoreApplication>
#include "DataProcessingService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DataProcessingService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "DataProcessingService.moc"
