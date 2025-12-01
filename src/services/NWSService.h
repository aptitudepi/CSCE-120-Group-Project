#ifndef NWSSERVICE_H
#define NWSSERVICE_H

#include "services/WeatherService.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>

/**
 * @brief National Weather Service API integration
 * 
 * Implements weather data fetching from the NWS API:
 * - Points to gridpoint conversion
 * - Forecast and hourly forecast retrieval
 * - Active alerts retrieval
 * - Conditional GET support for caching
 */
class NWSService : public WeatherService
{
    Q_OBJECT
    
public:
    explicit NWSService(QObject *parent = nullptr);
    ~NWSService() override;
    
    void fetchForecast(double latitude, double longitude) override;
    void fetchCurrent(double latitude, double longitude) override;
    QString serviceName() const override { return "NWS"; }
    
    /**
     * @brief Fetch active alerts for a location
     */
    void fetchAlerts(double latitude, double longitude);
    
    /**
     * @brief Get gridpoint information for a location
     */
    void fetchGridpoint(double latitude, double longitude);
    
    void cancelActiveRequests() override;
    
signals:
    void alertsReady(QList<QJsonObject> alerts);
    void gridpointReady(QString office, int x, int y);
    
private slots:
    void onPointsReplyFinished();
    void onForecastReplyFinished();
    void onHourlyReplyFinished();
    void onAlertsReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError networkError);
    
private:
    struct Gridpoint {
        QString office;
        int x;
        int y;
        QDateTime lastModified;
        
        bool isValid() const {
            return !office.isEmpty() && x >= 0 && y >= 0;
        }
    };
    
    void parsePointsResponse(const QByteArray& data, double lat, double lon);
    void parseForecastResponse(const QByteArray& data, double lat, double lon);
    void parseHourlyForecastResponse(const QByteArray& data, double lat, double lon);
    void parseAlertsResponse(const QByteArray& data);
    QList<WeatherData*> parsePeriods(const QJsonArray& periods, double lat, double lon);
    WeatherData* parsePeriod(const QJsonObject& period, double lat, double lon);
    
    QNetworkAccessManager* m_networkManager;
    QMap<QString, Gridpoint> m_gridpointCache;
    QMap<QString, QDateTime> m_lastModifiedCache;
    QSet<QNetworkReply*> m_activeReplies;
    
    void unregisterReply(QNetworkReply* reply);
    
    static const QString BASE_URL;
};

#endif // NWSSERVICE_H

