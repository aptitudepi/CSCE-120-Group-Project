
// WeatherDataService.h
#ifndef WEATHERDATASERVICE_H
#define WEATHERDATASERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QSettings>
#include <QEventLoop>

class WeatherDataCollector : public QObject
{
    Q_OBJECT

public:
    explicit WeatherDataCollector(QObject *parent = nullptr);

    void collectAllSources(double lat, double lon);

signals:
    void dataReady(const QJsonObject &data);
    void errorOccurred(const QString &error);

private slots:
    void onPirateWeatherFinished();
    void onNWSFinished();
    void onOpenMeteoFinished();

private:
    QJsonObject getPirateWeatherData(double lat, double lon);
    QJsonObject getNWSData(double lat, double lon);
    QJsonObject getOpenMeteoData(double lat, double lon);

    QNetworkAccessManager *m_networkManager;
    QString m_pirateWeatherKey;

    // Current request data
    double m_currentLat;
    double m_currentLon;
    QJsonArray m_sourcesData;
    int m_pendingRequests;
};

class WeatherDataService : public QObject
{
    Q_OBJECT

public:
    explicit WeatherDataService(QObject *parent = nullptr);
    bool start(int port = 8001);
    void stop();

private:
    QHttpServer *m_server;
    WeatherDataCollector *m_collector;
    QSettings *m_settings;
};

#endif // WEATHERDATASERVICE_H
