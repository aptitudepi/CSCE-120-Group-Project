#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMap>
#include <QList>

/**
 * @brief Performance monitoring and metrics tracking
 * 
 * Tracks key performance indicators (KPIs) for the application:
 * - Forecast delivery time (< 10 seconds)
 * - Precipitation hit rate (> 75%)
 * - Service uptime (95%)
 * - Alert lead time (≥ 5 minutes)
 */
class PerformanceMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double forecastResponseTime READ averageForecastResponseTime NOTIFY metricsUpdated)
    Q_PROPERTY(double precipitationHitRate READ precipitationHitRate NOTIFY metricsUpdated)
    Q_PROPERTY(double serviceUptime READ serviceUptime NOTIFY metricsUpdated)
    Q_PROPERTY(double alertLeadTime READ averageAlertLeadTime NOTIFY metricsUpdated)
    
public:
    explicit PerformanceMonitor(QObject *parent = nullptr);
    ~PerformanceMonitor() override;
    
    // Forecast performance
    void recordForecastRequest(const QString& requestId);
    void recordForecastResponse(const QString& requestId, qint64 responseTimeMs);
    double averageForecastResponseTime() const;
    bool isForecastTimeAcceptable(qint64 timeMs) const; // < 10 seconds
    
    // Precipitation accuracy
    void recordPrecipitationPrediction(const QString& location, 
                                      const QDateTime& predictedTime,
                                      double predictedIntensity);
    void recordPrecipitationObservation(const QString& location,
                                       const QDateTime& observedTime,
                                       double observedIntensity);
    double precipitationHitRate() const; // > 75% target
    
    // Service uptime
    void recordServiceUp(const QString& serviceName);
    void recordServiceDown(const QString& serviceName);
    double serviceUptime() const; // 95% target
    double serviceUptime(const QString& serviceName) const;
    
    // Alert performance
    void recordAlertTriggered(const QString& alertId, const QDateTime& triggerTime);
    void recordAlertEvent(const QString& alertId, const QDateTime& eventTime);
    double averageAlertLeadTime() const; // ≥ 5 minutes target
    
    // Test coverage tracking
    void recordTestCoverage(const QString& module, int linesCovered, int totalLines);
    double testCoverage() const; // 75% target on critical modules
    double testCoverage(const QString& module) const;
    
    // Get all metrics
    struct Metrics {
        double forecastResponseTime;
        double precipitationHitRate;
        double serviceUptime;
        double alertLeadTime;
        double testCoverage;
        int totalForecastRequests;
        int totalPrecipitationPredictions;
        int totalAlerts;
    };
    
    Metrics getMetrics() const;
    
signals:
    void metricsUpdated();
    void performanceWarning(const QString& metric, double value, double threshold);
    
private:
    // Forecast tracking
    QMap<QString, QElapsedTimer> m_forecastTimers;
    QList<qint64> m_forecastResponseTimes;
    
    // Precipitation tracking
    struct PrecipitationPrediction {
        QString location;
        QDateTime predictedTime;
        double predictedIntensity;
        bool verified;
        QDateTime observedTime;
        double observedIntensity;
    };
    QList<PrecipitationPrediction> m_precipitationPredictions;
    
    // Uptime tracking
    struct ServiceStatus {
        QDateTime startTime;
        QList<QPair<QDateTime, bool>> statusHistory; // time -> isUp
        int totalUptimeSeconds;
        int totalDowntimeSeconds;
    };
    QMap<QString, ServiceStatus> m_serviceStatus;
    
    // Alert tracking
    struct AlertRecord {
        QString alertId;
        QDateTime triggerTime;
        QDateTime eventTime;
        qint64 leadTimeSeconds;
    };
    QList<AlertRecord> m_alertRecords;
    
    // Test coverage
    QMap<QString, QPair<int, int>> m_testCoverage; // module -> (covered, total)
    
    QDateTime m_startTime;
    
    void checkThresholds();
    void updateServiceUptime(const QString& serviceName);
};

#endif // PERFORMANCEMONITOR_H

