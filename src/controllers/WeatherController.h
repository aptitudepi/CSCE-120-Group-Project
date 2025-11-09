#ifndef WEATHERCONTROLLER_H
#define WEATHERCONTROLLER_H

#include <QObject>
#include <QList>
#include "models/WeatherData.h"
#include "models/ForecastModel.h"
#include "services/NWSService.h"
#include "services/PirateWeatherService.h"
#include "services/CacheManager.h"

/**
 * @brief Main controller for weather data management
 * 
 * Coordinates between UI, services, and cache to provide
 * weather forecast data to the QML frontend.
 */
class WeatherController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ForecastModel* forecastModel READ forecastModel NOTIFY forecastModelChanged)
    Q_PROPERTY(WeatherData* current READ current NOTIFY currentChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    
public:
    explicit WeatherController(QObject *parent = nullptr);
    ~WeatherController() override;
    
    ForecastModel* forecastModel() const { return m_forecastModel; }
    WeatherData* current() const { return m_current; }
    bool loading() const { return m_loading; }
    QString errorMessage() const { return m_errorMessage; }
    
    Q_INVOKABLE void fetchForecast(double latitude, double longitude);
    Q_INVOKABLE void refreshForecast();
    Q_INVOKABLE void clearError();
    
signals:
    void forecastModelChanged();
    void currentChanged();
    void loadingChanged();
    void errorMessageChanged();
    void forecastUpdated();
    
private slots:
    void onForecastReady(QList<WeatherData*> data);
    void onCurrentReady(WeatherData* data);
    void onServiceError(QString error);
    
private:
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    QString generateCacheKey(double lat, double lon) const;
    QList<WeatherData*> loadFromCache(const QString& key);
    void saveToCache(const QString& key, const QList<WeatherData*>& data);
    
    ForecastModel* m_forecastModel;
    WeatherData* m_current;
    NWSService* m_nwsService;
    PirateWeatherService* m_pirateService;
    CacheManager* m_cache;
    
    bool m_loading;
    QString m_errorMessage;
    double m_lastLat;
    double m_lastLon;
};

#endif // WEATHERCONTROLLER_H

