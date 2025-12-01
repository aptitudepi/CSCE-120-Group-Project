#ifndef PIRATEWEATHERSERVICE_H
#define PIRATEWEATHERSERVICE_H

#include "services/WeatherService.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QSet>
#include <QtConcurrent>

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
    
    /**
     * @brief Cancel any in-flight network requests.
     * 
     * Used when the caller issues a new fetch before the previous request
     * completes (e.g., rapid GPS updates). Ensures we don't process stale
     * replies that can lead to inconsistent state.
     */
    void cancelActiveRequests() override;
    
signals:
    void minuteForecastReady(QList<WeatherData*> data);
    
    // Expose for testing
    friend class PirateWeatherServiceTest;
    
private slots:
    void onForecastReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError networkError);
    
private:
    void parseForecastResponse(const QByteArray& data, double lat, double lon, bool hasMinuteReceivers);
    QList<WeatherData*> parseHourlyData(const QJsonArray& hourly, double lat, double lon);
    QList<WeatherData*> parseMinutelyData(const QJsonArray& minutely, double lat, double lon);
    WeatherData* parseDataPoint(const QJsonObject& point, double lat, double lon);
    void abortActiveRequests();
    void unregisterReply(QNetworkReply* reply);
    
    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QSet<QNetworkReply*> m_activeReplies;
    
    static const QString BASE_URL;
};

#endif // PIRATEWEATHERSERVICE_H

