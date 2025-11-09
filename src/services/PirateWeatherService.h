#ifndef PIRATEWEATHERSERVICE_H
#define PIRATEWEATHERSERVICE_H

#include "services/WeatherService.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

/**
 * @brief Pirate Weather API integration
 * 
 * Implements weather data fetching from the Pirate Weather API
 * (Dark Sky API replacement). Provides minute-by-minute precipitation
 * forecasts for nowcasting.
 */
class PirateWeatherService : public WeatherService
{
    Q_OBJECT
    
public:
    explicit PirateWeatherService(QObject *parent = nullptr);
    ~PirateWeatherService() override;
    
    void fetchForecast(double latitude, double longitude) override;
    void fetchCurrent(double latitude, double longitude) override;
    QString serviceName() const override { return "PirateWeather"; }
    
    /**
     * @brief Set API key
     */
    void setApiKey(const QString& apiKey);
    
    /**
     * @brief Check if API key is set
     */
    bool hasApiKey() const { return !m_apiKey.isEmpty(); }
    
    bool isAvailable() const override { return hasApiKey(); }
    
signals:
    void minuteForecastReady(QList<WeatherData*> data);
    
private slots:
    void onForecastReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError networkError);
    
private:
    void parseForecastResponse(const QByteArray& data, double lat, double lon);
    QList<WeatherData*> parseHourlyData(const QJsonArray& hourly, double lat, double lon);
    QList<WeatherData*> parseMinutelyData(const QJsonArray& minutely, double lat, double lon);
    WeatherData* parseDataPoint(const QJsonObject& point, double lat, double lon);
    
    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    
    static const QString BASE_URL;
};

#endif // PIRATEWEATHERSERVICE_H

