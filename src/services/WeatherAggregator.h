#ifndef WEATHERAGGREGATOR_H
#define WEATHERAGGREGATOR_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QMap>
#include "services/WeatherService.h"
#include "services/MovingAverageFilter.h"
#include "models/WeatherData.h"

/**
 * @brief Aggregator service for multiple weather data sources
 * 
 * Implements the aggregator pattern to combine data from multiple
 * weather APIs (NWS, Pirate Weather, etc.) with intelligent fallback,
 * data fusion, and performance tracking.
 */
class WeatherAggregator : public QObject
{
    Q_OBJECT
    
public:
    enum AggregationStrategy {
        PrimaryOnly,      // Use primary source only
        Fallback,         // Try primary, fallback to secondary
        WeightedAverage,  // Weighted average of all sources
        BestAvailable     // Use best available source based on confidence
    };
    Q_ENUM(AggregationStrategy)
    
    explicit WeatherAggregator(QObject *parent = nullptr);
    ~WeatherAggregator() override;
    
    /**
     * @brief Add a weather service to the aggregator
     */
    void addService(WeatherService* service, int priority = 0);
    
    /**
     * @brief Set aggregation strategy
     */
    void setStrategy(AggregationStrategy strategy);
    
    /**
     * @brief Enable/disable moving average smoothing
     */
    void setMovingAverageEnabled(bool enabled);
    
    /**
     * @brief Set moving average window size
     */
    void setMovingAverageWindowSize(int windowSize);
    
    /**
     * @brief Set moving average type
     */
    void setMovingAverageType(MovingAverageFilter::MovingAverageType type);
    
    /**
     * @brief Set EMA alpha (smoothing factor)
     */
    void setMovingAverageAlpha(double alpha);
    
    /**
     * @brief Fetch forecast using aggregation strategy
     */
    void fetchForecast(double latitude, double longitude);
    
    /**
     * @brief Get performance metrics
     */
    struct PerformanceMetrics {
        qint64 averageResponseTime;  // milliseconds
        double cacheHitRate;         // 0.0 to 1.0
        double serviceUptime;        // 0.0 to 1.0
        int totalRequests;
        int successfulRequests;
        int failedRequests;
    };
    
    PerformanceMetrics getMetrics() const;
    
signals:
    void forecastReady(QList<WeatherData*> data);
    void currentReady(WeatherData* data);
    void error(QString message);
    void metricsUpdated(PerformanceMetrics metrics);
    
private slots:
    void onServiceForecastReady(QList<WeatherData*> data);
    void onServiceCurrentReady(WeatherData* data);
    void onServiceError(QString message);
    void onTimeout();
    
private:
    struct ServiceEntry {
        WeatherService* service;
        int priority;
        bool available;
        qint64 lastResponseTime;
        int successCount;
        int failureCount;
        QDateTime lastSuccessTime;
    };
    
    struct ForecastWithService;
    
    void updateServiceAvailability(WeatherService* service, bool success, qint64 responseTime);
    QList<WeatherData*> mergeForecasts(const QList<ForecastWithService>& forecastsWithServices);
    WeatherData* mergeCurrentWeather(const QList<WeatherData*>& currentData);
    double calculateConfidence(WeatherService* service) const;
    double calculateWeight(WeatherService* service, qint64 responseTime) const;
    QDateTime binTimestamp(const QDateTime& timestamp, int binMinutes = 30) const;
    
    struct ForecastWithService {
        QList<WeatherData*> forecasts;
        WeatherService* service;
        qint64 responseTime;
    };
    
    QList<ServiceEntry> m_services;
    AggregationStrategy m_strategy;
    QElapsedTimer m_requestTimer;
    QTimer* m_timeoutTimer;
    MovingAverageFilter* m_movingAverageFilter;
    bool m_movingAverageEnabled;
    
    // Performance tracking
    QList<qint64> m_responseTimes;
    int m_totalRequests;
    int m_successfulRequests;
    int m_failedRequests;
    QDateTime m_startTime;
    QMap<WeatherService*, QDateTime> m_lastFailureTime;
    
    // Request tracking
    QMap<QString, QList<WeatherService*>> m_pendingRequests; // cacheKey -> services
    QMap<QString, QList<ForecastWithService>> m_pendingForecasts; // cacheKey -> list of (forecasts, service, responseTime)
    QMap<QString, QList<WeatherData*>> m_pendingCurrentWeather; // cacheKey -> list of current weather
    QMap<QString, QList<WeatherService*>> m_receivedServices; // cacheKey -> services that have responded
    QMap<QString, QMap<WeatherService*, qint64>> m_serviceResponseTimes; // cacheKey -> service -> responseTime
    QString m_currentRequestKey;
    double m_currentLat;
    double m_currentLon;
};

#endif // WEATHERAGGREGATOR_H

