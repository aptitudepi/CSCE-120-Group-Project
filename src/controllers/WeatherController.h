#ifndef WEATHERCONTROLLER_H
#define WEATHERCONTROLLER_H

#include <QObject>
#include <QList>
#include <QVariant>
#include "models/WeatherData.h"
#include "models/ForecastModel.h"
#include "services/NWSService.h"
#include "services/PirateWeatherService.h"
#include "services/WeatherbitService.h"
#include "services/CacheManager.h"
#include "services/WeatherAggregator.h"
#include "services/PerformanceMonitor.h"
#include "services/HistoricalDataManager.h"
#include "nowcast/NowcastEngine.h"

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
    Q_PROPERTY(QString serviceProvider READ serviceProvider NOTIFY serviceProviderChanged)
    Q_PROPERTY(PerformanceMonitor* performanceMonitor READ performanceMonitor NOTIFY performanceMonitorChanged)
    Q_PROPERTY(bool useAggregation READ useAggregation WRITE setUseAggregation NOTIFY useAggregationChanged)
    
public:
    enum ServiceProvider {
        NWS = 0,
        PirateWeather = 1,
        Aggregated = 2,
        Weatherbit = 3
    };
    Q_ENUM(ServiceProvider)
    
    explicit WeatherController(QObject *parent = nullptr);
    ~WeatherController() override;
    
    ForecastModel* forecastModel() const { return m_forecastModel; }
    WeatherData* current() const { return m_current; }
    bool loading() const { return m_loading; }
    QString errorMessage() const { return m_errorMessage; }
    QString serviceProvider() const;
    PerformanceMonitor* performanceMonitor() const { return m_performanceMonitor; }
    bool useAggregation() const { return m_useAggregation; }
    void setUseAggregation(bool use);
    
    Q_INVOKABLE void fetchForecast(double latitude, double longitude);
    Q_INVOKABLE void refreshForecast();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void setServiceProvider(int provider);
    Q_INVOKABLE void fetchNowcast(double latitude, double longitude);
    Q_INVOKABLE void saveLocation(const QString& name, double latitude, double longitude);
    Q_INVOKABLE QVariantList getSavedLocations();
    Q_INVOKABLE void deleteLocation(int locationId);
    Q_INVOKABLE void loadLocation(int locationId);
    
signals:
    void forecastModelChanged();
    void currentChanged();
    void loadingChanged();
    void errorMessageChanged();
    void forecastUpdated();
    void serviceProviderChanged();
    void performanceMonitorChanged();
    void useAggregationChanged();
    void nowcastReady(QList<WeatherData*> nowcast);
    
private slots:
    void onForecastReady(QList<WeatherData*> data);
    void onCurrentReady(WeatherData* data);
    void onServiceError(QString error);
    void onAggregatorForecastReady(QList<WeatherData*> data);
    void onAggregatorError(QString error);
    
private:
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    QString generateCacheKey(double lat, double lon) const;
    QList<WeatherData*> loadFromCache(const QString& key);
    void saveToCache(const QString& key, const QList<WeatherData*>& data);
    bool isValidCoordinate(double latitude, double longitude) const;
    bool shouldProcessServiceResponse(QObject* sender, bool& callerOwnsData) const;
    
    ForecastModel* m_forecastModel;
    WeatherData* m_current;
    NWSService* m_nwsService;
    PirateWeatherService* m_pirateService;
    WeatherbitService* m_weatherbitService;
    CacheManager* m_cache;
    WeatherAggregator* m_aggregator;
    PerformanceMonitor* m_performanceMonitor;
    HistoricalDataManager* m_historicalManager;
    NowcastEngine* m_nowcastEngine;
    
    bool m_loading;
    QString m_errorMessage;
    double m_lastLat;
    double m_lastLon;
    ServiceProvider m_serviceProvider;
    bool m_useAggregation;
};

#endif // WEATHERCONTROLLER_H

