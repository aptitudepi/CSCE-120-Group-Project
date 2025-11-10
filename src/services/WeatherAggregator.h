#ifndef WEATHERAGGREGATOR_H
#define WEATHERAGGREGATOR_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QElapsedTimer>
#include "services/WeatherService.h"
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
    
    void updateServiceAvailability(WeatherService* service, bool success, qint64 responseTime);
    QList<WeatherData*> mergeForecasts(const QList<QList<WeatherData*>>& forecasts);
    WeatherData* mergeCurrentWeather(const QList<WeatherData*>& currentData);
    double calculateConfidence(WeatherService* service) const;
    
    QList<ServiceEntry> m_services;
    AggregationStrategy m_strategy;
    QElapsedTimer m_requestTimer;
    QTimer* m_timeoutTimer;
    
    // Performance tracking
    QList<qint64> m_responseTimes;
    int m_totalRequests;
    int m_successfulRequests;
    int m_failedRequests;
    QDateTime m_startTime;
    QMap<WeatherService*, QDateTime> m_lastFailureTime;
    
    // Request tracking
    QMap<QString, QList<WeatherService*>> m_pendingRequests; // cacheKey -> services
    QString m_currentRequestKey;
};

#endif // WEATHERAGGREGATOR_H

