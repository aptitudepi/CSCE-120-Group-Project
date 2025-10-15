
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
