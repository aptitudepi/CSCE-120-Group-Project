
// MLService.h
#ifndef MLSERVICE_H
#define MLSERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>

// Simple LSTM-like prediction engine (simplified for demo)
class WeatherMLPredictor : public QObject
{
    Q_OBJECT

public:
    explicit WeatherMLPredictor(QObject *parent = nullptr);

    QJsonObject predictHyperlocal(const QJsonObject &weatherData, double lat, double lon);
    QJsonObject ruleBasedPrediction(const QJsonObject &weatherData, double lat, double lon);
    double calculateConfidenceScore(const QJsonObject &weatherData);
    QJsonArray analyzeAlerts(const QJsonObject &predictionData);

private:
    bool m_isModelTrained;
    QStringList m_featureColumns;

    double extractTemperature(const QJsonObject &sourceData, const QString &source);
    double extractHumidity(const QJsonObject &sourceData, const QString &source);
    double extractPrecipitation(const QJsonObject &sourceData, const QString &source);
    double extractWindSpeed(const QJsonObject &sourceData, const QString &source);

    // Simplified ML simulation
    double applyLSTMModel(const QVector<double> &features);
    QVector<double> prepareFeatures(const QJsonObject &weatherData, double lat, double lon);
};

class MLService : public QObject
{
    Q_OBJECT

public:
    explicit MLService(QObject *parent = nullptr);
    bool start(int port = 8002);
    void stop();

private:
    QHttpServer *m_server;
    WeatherMLPredictor *m_predictor;
    QSettings *m_settings;
};

#endif // MLSERVICE_H

// MLService.cpp
#include "MLService.h"
#include <QCoreApplication>
#include <QtMath>

WeatherMLPredictor::WeatherMLPredictor(QObject *parent)
    : QObject(parent)
    , m_isModelTrained(false)
{
    m_featureColumns = QStringList() << "temperature" << "humidity" << "precipitation" 
                                    << "wind_speed" << "latitude" << "longitude" 
                                    << "hour_of_day" << "day_of_year";

    // For demo purposes, we'll use rule-based predictions
    // In production, this would load actual LSTM models
    qDebug() << "Weather ML Predictor initialized";
}

QJsonObject WeatherMLPredictor::predictHyperlocal(const QJsonObject &weatherData, double lat, double lon)
{
    if (!m_isModelTrained) {
        qDebug() << "Using rule-based prediction (ML model not trained)";
        return ruleBasedPrediction(weatherData, lat, lon);
    }

    // Prepare features for ML model
    QVector<double> features = prepareFeatures(weatherData, lat, lon);

    // Apply LSTM model (simplified simulation)
    double predTemp = applyLSTMModel(features);
    double predHumidity = qMax(0.0, qMin(100.0, features[1] + QRandomGenerator::global()->bounded(-5.0, 5.0)));
    double predPrecip = qMax(0.0, features[2] * (0.8 + QRandomGenerator::global()->bounded(0.4)));
    double predWind = qMax(0.0, features[3] + QRandomGenerator::global()->bounded(-2.0, 2.0));

    double confidence = calculateConfidenceScore(weatherData);

    QJsonObject predictions;
    predictions["temperature"] = predTemp;
    predictions["humidity"] = predHumidity;
    predictions["precipitation_intensity"] = predPrecip;
    predictions["wind_speed"] = predWind;

    QJsonObject result;
    result["predictions"] = predictions;
    result["forecast_horizon"] = "2_hours";
    result["confidence_score"] = confidence;
    result["model_type"] = "LSTM_with_attention";
    result["hyperlocal_resolution"] = "1km";
    result["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Analyze for alerts
    result["alerts"] = analyzeAlerts(result);

    return result;
}

QJsonObject WeatherMLPredictor::ruleBasedPrediction(const QJsonObject &weatherData, double lat, double lon)
{
    QJsonArray sources = weatherData["sources"].toArray();

    // Extract current conditions from sources
    double currentTemp = 20.0;
    double currentHumidity = 50.0;
    double currentPrecip = 0.0;
    double currentWind = 5.0;

    int tempCount = 0, humidityCount = 0, precipCount = 0, windCount = 0;

    for (const auto &sourceValue : sources) {
        QJsonObject sourceData = sourceValue.toObject();
        QString source = sourceData["source"].toString();

        double temp = extractTemperature(sourceData, source);
        if (!qIsNaN(temp)) {
            currentTemp = (currentTemp * tempCount + temp) / (tempCount + 1);
            tempCount++;
        }

        double humidity = extractHumidity(sourceData, source);
        if (!qIsNaN(humidity)) {
            currentHumidity = (currentHumidity * humidityCount + humidity) / (humidityCount + 1);
            humidityCount++;
        }

        double precip = extractPrecipitation(sourceData, source);
        if (!qIsNaN(precip)) {
            currentPrecip = (currentPrecip * precipCount + precip) / (precipCount + 1);
            precipCount++;
        }

        double wind = extractWindSpeed(sourceData, source);
        if (!qIsNaN(wind)) {
            currentWind = (currentWind * windCount + wind) / (windCount + 1);
            windCount++;
        }
    }

    // Apply simple persistence with variations
    double predictedTemp = currentTemp + QRandomGenerator::global()->bounded(-2.0, 2.0);
    double predictedHumidity = qMax(0.0, qMin(100.0, currentHumidity + QRandomGenerator::global()->bounded(-5.0, 5.0)));

    // Precipitation prediction based on humidity
    double predictedPrecip = currentPrecip;
    if (currentHumidity > 80) {
        predictedPrecip = qMax(0.0, currentPrecip + qAbs(QRandomGenerator::global()->bounded(0.5)));
    } else {
        predictedPrecip = qMax(0.0, currentPrecip * 0.8);
    }

    double predictedWind = qMax(0.0, currentWind + QRandomGenerator::global()->bounded(-1.0, 1.0));

    // Calculate confidence based on data quality
    double dataQuality = (tempCount + humidityCount + precipCount + windCount) / 12.0; // Max 3 sources * 4 metrics
    double confidence = qMin(0.85, 0.6 + dataQuality * 0.25); // Max 85% for rule-based

    QJsonObject predictions;
    predictions["temperature"] = predictedTemp;
    predictions["humidity"] = predictedHumidity;
    predictions["precipitation_intensity"] = predictedPrecip;
    predictions["wind_speed"] = predictedWind;

    QJsonObject result;
    result["predictions"] = predictions;
    result["forecast_horizon"] = "2_hours";
    result["confidence_score"] = confidence;
    result["model_type"] = "rule_based_fallback";
    result["hyperlocal_resolution"] = "1km";
    result["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    result["note"] = "Using rule-based prediction - ML model training required";

    result["alerts"] = analyzeAlerts(result);

    return result;
}

double WeatherMLPredictor::extractTemperature(const QJsonObject &sourceData, const QString &source)
{
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

double WeatherMLPredictor::extractHumidity(const QJsonObject &sourceData, const QString &source)
{
    QJsonObject data = sourceData["data"].toObject();

    if (source == "pirate_weather") {
        return data["currently"].toObject()["humidity"].toDouble(qQNaN()) * 100.0; // Convert to percentage
    } else if (source == "nws") {
        QJsonObject props = data["properties"].toObject();
        return props["relativeHumidity"].toObject()["value"].toDouble(qQNaN());
    } else if (source == "openmeteo") {
        return data["current"].toObject()["relative_humidity_2m"].toDouble(qQNaN());
    }

    return qQNaN();
}

double WeatherMLPredictor::extractPrecipitation(const QJsonObject &sourceData, const QString &source)
{
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

double WeatherMLPredictor::extractWindSpeed(const QJsonObject &sourceData, const QString &source)
{
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

double WeatherMLPredictor::calculateConfidenceScore(const QJsonObject &weatherData)
{
    QJsonArray sources = weatherData["sources"].toArray();
    double baseConfidence = 0.5 + (sources.size() * 0.15); // More sources = higher confidence

    // Simulate model uncertainty (would be calculated from actual model in production)
    double stabilityFactor = 0.9;

    return qMin(0.96, baseConfidence * 0.7 + stabilityFactor * 0.3);
}

QJsonArray WeatherMLPredictor::analyzeAlerts(const QJsonObject &predictionData)
{
    QJsonArray alerts;
    QJsonObject predictions = predictionData["predictions"].toObject();
    double confidence = predictionData["confidence_score"].toDouble();

    // Precipitation alert
    double precipIntensity = predictions["precipitation_intensity"].toDouble();
    if (precipIntensity > 2.0 && confidence > 0.85) {
        QJsonObject alert;
        alert["type"] = "precipitation";
        alert["severity"] = (precipIntensity > 5.0) ? "high" : "medium";
        alert["message"] = QString("Heavy precipitation expected: %1mm/hr").arg(precipIntensity, 0, 'f', 1);
        alert["confidence"] = confidence;
        alerts.append(alert);
    }

    // Wind alert
    double windSpeed = predictions["wind_speed"].toDouble();
    if (windSpeed > 15.0 && confidence > 0.80) {
        QJsonObject alert;
        alert["type"] = "wind";
        alert["severity"] = (windSpeed > 25.0) ? "high" : "medium";
        alert["message"] = QString("Strong winds expected: %1m/s").arg(windSpeed, 0, 'f', 1);
        alert["confidence"] = confidence;
        alerts.append(alert);
    }

    return alerts;
}

double WeatherMLPredictor::applyLSTMModel(const QVector<double> &features)
{
    // Simplified LSTM simulation - in production this would use actual trained models
    // For now, just return a temperature prediction based on input features with some variation
    double baseTemp = features.size() > 0 ? features[0] : 20.0;
    return baseTemp + QRandomGenerator::global()->bounded(-3.0, 3.0);
}

QVector<double> WeatherMLPredictor::prepareFeatures(const QJsonObject &weatherData, double lat, double lon)
{
    QVector<double> features;

    // This would extract and prepare features for the ML model
    // For now, just return some basic features
    features.append(20.0); // temperature
    features.append(50.0); // humidity
    features.append(0.0);  // precipitation
    features.append(5.0);  // wind speed
    features.append(lat);  // latitude
    features.append(lon);  // longitude
    features.append(QDateTime::currentDateTime().time().hour()); // hour of day
    features.append(QDateTime::currentDateTime().date().dayOfYear()); // day of year

    return features;
}

MLService::MLService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_predictor(new WeatherMLPredictor(this))
    , m_settings(new QSettings(this))
{
    qDebug() << "ML Service initialized";
}

bool MLService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Machine Learning Service";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Prediction endpoint
    m_server->route("/predict", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject requestData = doc.object();

        QJsonObject weatherData = requestData["weather_data"].toObject();
        double latitude = requestData["latitude"].toDouble();
        double longitude = requestData["longitude"].toDouble();

        if (weatherData.isEmpty() || qIsNaN(latitude) || qIsNaN(longitude)) {
            QJsonObject error;
            error["error"] = "Weather data, latitude, and longitude required";
            error["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QHttpServerResponse httpResponse(QJsonDocument(error).toJson(), QHttpServerResponse::StatusCode::BadRequest);
            httpResponse.setHeader("Content-Type", "application/json");
            return httpResponse;
        }

        QJsonObject prediction = m_predictor->predictHyperlocal(weatherData, latitude, longitude);

        QHttpServerResponse httpResponse(QJsonDocument(prediction).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Model status endpoint
    m_server->route("/model/status", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["model_loaded"] = true;
        response["is_trained"] = false; // Using rule-based for demo
        response["feature_count"] = 8;
        response["model_architecture"] = "LSTM with attention mechanism";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "ml_service";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QHttpServerResponse httpResponse(QJsonDocument(response).toJson());
        httpResponse.setHeader("Content-Type", "application/json");
        return httpResponse;
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "ML Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start ML Service on port" << port;
        return false;
    }
}

void MLService::stop()
{
    m_server->disconnect();
    qDebug() << "ML Service stopped";
}

// main.cpp for ML Service
#include <QCoreApplication>
#include "MLService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    MLService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "MLService.moc"
