#ifndef WEATHERCLIENT_H
#define WEATHERCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <qqml.h>

class WeatherClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(WeatherAPI)

    Q_PROPERTY(QString temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(QString humidity READ humidity NOTIFY humidityChanged)
    Q_PROPERTY(QString precipitation READ precipitation NOTIFY precipitationChanged)
    Q_PROPERTY(double confidence READ confidence NOTIFY confidenceChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit WeatherClient(QObject *parent = nullptr);
    ~WeatherClient();

    QString temperature() const { return m_temperature; }
    QString humidity() const { return m_humidity; }
    QString precipitation() const { return m_precipitation; }
    double confidence() const { return m_confidence; }
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE void updateLocationData(double latitude, double longitude);
    Q_INVOKABLE void generateHyperlocalForecast(double latitude, double longitude);
    Q_INVOKABLE void authenticate(const QString &username, const QString &password);

signals:
    void temperatureChanged();
    void humidityChanged();
    void precipitationChanged();
    void confidenceChanged();
    void isLoadingChanged();
    void forecastReady(const QJsonObject &forecast);
    void alertTriggered(const QString &message);
    void authenticationSuccess(const QString &token);
    void authenticationFailed(const QString &error);
    void errorOccurred(const QString &error);

private slots:
    void handleAuthReply();
    void handleWeatherReply();
    void handleForecastReply();

private:
    void setTemperature(const QString &temp);
    void setHumidity(const QString &hum);
    void setPrecipitation(const QString &precip);
    void setConfidence(double conf);
    void setIsLoading(bool loading);

    void makeAuthenticatedRequest(const QString &endpoint, const QJsonObject &data = QJsonObject());
    
    QNetworkAccessManager *m_networkManager;
    QString m_apiBaseUrl;
    QString m_authToken;
    
    QString m_temperature;
    QString m_humidity;
    QString m_precipitation;
    double m_confidence;
    bool m_isLoading;
    
    double m_currentLatitude;
    double m_currentLongitude;
};

#endif // WEATHERCLIENT_H
