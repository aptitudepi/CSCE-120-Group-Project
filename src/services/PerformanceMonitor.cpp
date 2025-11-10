#include "services/PerformanceMonitor.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent)
    , m_startTime(QDateTime::currentDateTime())
{
}

PerformanceMonitor::~PerformanceMonitor() = default;

void PerformanceMonitor::recordForecastRequest(const QString& requestId) {
    QElapsedTimer timer;
    timer.start();
    m_forecastTimers[requestId] = timer;
}

void PerformanceMonitor::recordForecastResponse(const QString& requestId, qint64 responseTimeMs) {
    if (m_forecastTimers.contains(requestId)) {
        m_forecastTimers.remove(requestId);
    }
    
    m_forecastResponseTimes.append(responseTimeMs);
    
    // Keep only last 1000 response times
    if (m_forecastResponseTimes.size() > 1000) {
        m_forecastResponseTimes.removeFirst();
    }
    
    // Check threshold
    if (!isForecastTimeAcceptable(responseTimeMs)) {
        emit performanceWarning("forecastResponseTime", responseTimeMs / 1000.0, 10.0);
    }
    
    emit metricsUpdated();
}

bool PerformanceMonitor::isForecastTimeAcceptable(qint64 timeMs) const {
    return timeMs < 10000; // 10 seconds
}

double PerformanceMonitor::averageForecastResponseTime() const {
    if (m_forecastResponseTimes.isEmpty()) return 0.0;
    
    qint64 sum = 0;
    for (qint64 time : m_forecastResponseTimes) {
        sum += time;
    }
    return sum / static_cast<double>(m_forecastResponseTimes.size()) / 1000.0; // Convert to seconds
}

void PerformanceMonitor::recordPrecipitationPrediction(const QString& location,
                                                       const QDateTime& predictedTime,
                                                       double predictedIntensity) {
    PrecipitationPrediction pred;
    pred.location = location;
    pred.predictedTime = predictedTime;
    pred.predictedIntensity = predictedIntensity;
    pred.verified = false;
    
    m_precipitationPredictions.append(pred);
    
    // Keep only last 1000 predictions
    if (m_precipitationPredictions.size() > 1000) {
        m_precipitationPredictions.removeFirst();
    }
}

void PerformanceMonitor::recordPrecipitationObservation(const QString& location,
                                                        const QDateTime& observedTime,
                                                        double observedIntensity) {
    // Match with predictions (within 30 minutes window)
    for (PrecipitationPrediction& pred : m_precipitationPredictions) {
        if (pred.location == location && !pred.verified) {
            qint64 timeDiff = qAbs(pred.predictedTime.secsTo(observedTime));
            if (timeDiff < 30 * 60) { // 30 minutes
                pred.verified = true;
                pred.observedTime = observedTime;
                pred.observedIntensity = observedIntensity;
                break;
            }
        }
    }
    
    checkThresholds();
    emit metricsUpdated();
}

double PerformanceMonitor::precipitationHitRate() const {
    if (m_precipitationPredictions.isEmpty()) return 0.0;
    
    int verified = 0;
    int hits = 0;
    
    for (const PrecipitationPrediction& pred : m_precipitationPredictions) {
        if (pred.verified) {
            verified++;
            // Consider a hit if intensity is within 50% or both > 0
            if ((pred.predictedIntensity > 0 && pred.observedIntensity > 0) ||
                qAbs(pred.predictedIntensity - pred.observedIntensity) / 
                (qMax(pred.predictedIntensity, pred.observedIntensity) + 0.001) < 0.5) {
                hits++;
            }
        }
    }
    
    if (verified == 0) return 0.0;
    return static_cast<double>(hits) / verified;
}

void PerformanceMonitor::recordServiceUp(const QString& serviceName) {
    if (!m_serviceStatus.contains(serviceName)) {
        ServiceStatus status;
        status.startTime = QDateTime::currentDateTime();
        m_serviceStatus[serviceName] = status;
    }
    
    ServiceStatus& status = m_serviceStatus[serviceName];
    status.statusHistory.append(qMakePair(QDateTime::currentDateTime(), true));
    updateServiceUptime(serviceName);
    
    checkThresholds();
    emit metricsUpdated();
}

void PerformanceMonitor::recordServiceDown(const QString& serviceName) {
    if (!m_serviceStatus.contains(serviceName)) {
        ServiceStatus status;
        status.startTime = QDateTime::currentDateTime();
        m_serviceStatus[serviceName] = status;
    }
    
    ServiceStatus& status = m_serviceStatus[serviceName];
    status.statusHistory.append(qMakePair(QDateTime::currentDateTime(), false));
    updateServiceUptime(serviceName);
    
    checkThresholds();
    emit metricsUpdated();
}

double PerformanceMonitor::serviceUptime() const {
    if (m_serviceStatus.isEmpty()) return 1.0;
    
    double totalUptime = 0.0;
    for (const QString& service : m_serviceStatus.keys()) {
        totalUptime += serviceUptime(service);
    }
    
    return totalUptime / m_serviceStatus.size();
}

double PerformanceMonitor::serviceUptime(const QString& serviceName) const {
    if (!m_serviceStatus.contains(serviceName)) return 1.0;
    
    const ServiceStatus& status = m_serviceStatus[serviceName];
    int total = status.totalUptimeSeconds + status.totalDowntimeSeconds;
    if (total == 0) return 1.0;
    
    return static_cast<double>(status.totalUptimeSeconds) / total;
}

void PerformanceMonitor::updateServiceUptime(const QString& serviceName) {
    ServiceStatus& status = m_serviceStatus[serviceName];
    
    // Recalculate from history
    status.totalUptimeSeconds = 0;
    status.totalDowntimeSeconds = 0;
    
    for (int i = 0; i < status.statusHistory.size() - 1; i++) {
        QDateTime start = status.statusHistory[i].first;
        QDateTime end = status.statusHistory[i + 1].first;
        bool isUp = status.statusHistory[i].second;
        
        int seconds = start.secsTo(end);
        if (isUp) {
            status.totalUptimeSeconds += seconds;
        } else {
            status.totalDowntimeSeconds += seconds;
        }
    }
}

void PerformanceMonitor::recordAlertTriggered(const QString& alertId, const QDateTime& triggerTime) {
    AlertRecord record;
    record.alertId = alertId;
    record.triggerTime = triggerTime;
    
    // Find existing record or create new
    bool found = false;
    for (AlertRecord& r : m_alertRecords) {
        if (r.alertId == alertId && !r.eventTime.isValid()) {
            r.triggerTime = triggerTime;
            found = true;
            break;
        }
    }
    
    if (!found) {
        m_alertRecords.append(record);
    }
    
    // Keep only last 1000 records
    if (m_alertRecords.size() > 1000) {
        m_alertRecords.removeFirst();
    }
}

void PerformanceMonitor::recordAlertEvent(const QString& alertId, const QDateTime& eventTime) {
    for (AlertRecord& record : m_alertRecords) {
        if (record.alertId == alertId && !record.eventTime.isValid()) {
            record.eventTime = eventTime;
            record.leadTimeSeconds = record.triggerTime.secsTo(eventTime);
            break;
        }
    }
    
    checkThresholds();
    emit metricsUpdated();
}

double PerformanceMonitor::averageAlertLeadTime() const {
    if (m_alertRecords.isEmpty()) return 0.0;
    
    qint64 totalLeadTime = 0;
    int count = 0;
    
    for (const AlertRecord& record : m_alertRecords) {
        if (record.eventTime.isValid() && record.leadTimeSeconds > 0) {
            totalLeadTime += record.leadTimeSeconds;
            count++;
        }
    }
    
    if (count == 0) return 0.0;
    return static_cast<double>(totalLeadTime) / count / 60.0; // Convert to minutes
}

void PerformanceMonitor::recordTestCoverage(const QString& module, int linesCovered, int totalLines) {
    m_testCoverage[module] = qMakePair(linesCovered, totalLines);
    emit metricsUpdated();
}

double PerformanceMonitor::testCoverage() const {
    if (m_testCoverage.isEmpty()) return 0.0;
    
    int totalCovered = 0;
    int totalLines = 0;
    
    for (const QPair<int, int>& coverage : m_testCoverage.values()) {
        totalCovered += coverage.first;
        totalLines += coverage.second;
    }
    
    if (totalLines == 0) return 0.0;
    return static_cast<double>(totalCovered) / totalLines;
}

double PerformanceMonitor::testCoverage(const QString& module) const {
    if (!m_testCoverage.contains(module)) return 0.0;
    
    const QPair<int, int>& coverage = m_testCoverage[module];
    if (coverage.second == 0) return 0.0;
    
    return static_cast<double>(coverage.first) / coverage.second;
}

PerformanceMonitor::Metrics PerformanceMonitor::getMetrics() const {
    Metrics metrics;
    metrics.forecastResponseTime = averageForecastResponseTime();
    metrics.precipitationHitRate = precipitationHitRate();
    metrics.serviceUptime = serviceUptime();
    metrics.alertLeadTime = averageAlertLeadTime();
    metrics.testCoverage = testCoverage();
    metrics.totalForecastRequests = m_forecastResponseTimes.size();
    metrics.totalPrecipitationPredictions = m_precipitationPredictions.size();
    metrics.totalAlerts = m_alertRecords.size();
    return metrics;
}

void PerformanceMonitor::checkThresholds() {
    // Check forecast response time
    double avgTime = averageForecastResponseTime();
    if (avgTime > 10.0) {
        emit performanceWarning("forecastResponseTime", avgTime, 10.0);
    }
    
    // Check precipitation hit rate
    double hitRate = precipitationHitRate();
    if (hitRate < 0.75 && m_precipitationPredictions.size() > 10) {
        emit performanceWarning("precipitationHitRate", hitRate, 0.75);
    }
    
    // Check service uptime
    double uptime = serviceUptime();
    if (uptime < 0.95) {
        emit performanceWarning("serviceUptime", uptime, 0.95);
    }
    
    // Check alert lead time
    double leadTime = averageAlertLeadTime();
    if (leadTime < 5.0 && m_alertRecords.size() > 5) {
        emit performanceWarning("alertLeadTime", leadTime, 5.0);
    }
    
    // Check test coverage
    double coverage = testCoverage();
    if (coverage < 0.75 && !m_testCoverage.isEmpty()) {
        emit performanceWarning("testCoverage", coverage, 0.75);
    }
}

