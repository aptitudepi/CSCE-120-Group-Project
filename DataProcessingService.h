
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
