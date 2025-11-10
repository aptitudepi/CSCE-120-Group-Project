#include "services/WeatherAggregator.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>

WeatherAggregator::WeatherAggregator(QObject *parent)
    : QObject(parent)
    , m_strategy(PrimaryOnly)
    , m_timeoutTimer(new QTimer(this))
    , m_totalRequests(0)
    , m_successfulRequests(0)
    , m_failedRequests(0)
    , m_startTime(QDateTime::currentDateTime())
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(10000); // 10 second timeout
    connect(m_timeoutTimer, &QTimer::timeout, this, &WeatherAggregator::onTimeout);
}

WeatherAggregator::~WeatherAggregator() = default;

void WeatherAggregator::addService(WeatherService* service, int priority) {
    if (!service) return;
    
    ServiceEntry entry;
    entry.service = service;
    entry.priority = priority;
    entry.available = true;
    entry.lastResponseTime = 0;
    entry.successCount = 0;
    entry.failureCount = 0;
    
    m_services.append(entry);
    
    // Sort by priority (higher priority first)
    std::sort(m_services.begin(), m_services.end(),
              [](const ServiceEntry& a, const ServiceEntry& b) {
                  return a.priority > b.priority;
              });
    
    // Connect signals
    connect(service, &WeatherService::forecastReady,
            this, &WeatherAggregator::onServiceForecastReady);
    connect(service, &WeatherService::currentReady,
            this, &WeatherAggregator::onServiceCurrentReady);
    connect(service, &WeatherService::error,
            this, &WeatherAggregator::onServiceError);
}

void WeatherAggregator::setStrategy(AggregationStrategy strategy) {
    m_strategy = strategy;
}

void WeatherAggregator::fetchForecast(double latitude, double longitude) {
    m_requestTimer.start();
    m_totalRequests++;
    
    QString cacheKey = QString("%1_%2").arg(latitude, 0, 'f', 4).arg(longitude, 0, 'f', 4);
    m_currentRequestKey = cacheKey;
    
    // Start timeout timer
    m_timeoutTimer->start();
    
    // Filter available services
    QList<WeatherService*> availableServices;
    for (const ServiceEntry& entry : m_services) {
        if (entry.service->isAvailable() && entry.available) {
            availableServices.append(entry.service);
        }
    }
    
    if (availableServices.isEmpty()) {
        emit error("No weather services available");
        m_failedRequests++;
        return;
    }
    
    // Track pending requests
    m_pendingRequests[cacheKey] = availableServices;
    
    // Execute based on strategy
    switch (m_strategy) {
        case PrimaryOnly:
        case Fallback:
            // Try first available service
            availableServices.first()->fetchForecast(latitude, longitude);
            break;
        case WeightedAverage:
        case BestAvailable:
            // Try all available services
            for (WeatherService* service : availableServices) {
                service->fetchForecast(latitude, longitude);
            }
            break;
    }
}

WeatherAggregator::PerformanceMetrics WeatherAggregator::getMetrics() const {
    PerformanceMetrics metrics;
    
    if (!m_responseTimes.isEmpty()) {
        qint64 sum = 0;
        for (qint64 time : m_responseTimes) {
            sum += time;
        }
        metrics.averageResponseTime = sum / m_responseTimes.size();
    } else {
        metrics.averageResponseTime = 0;
    }
    
    metrics.totalRequests = m_totalRequests;
    metrics.successfulRequests = m_successfulRequests;
    metrics.failedRequests = m_failedRequests;
    
    // Calculate uptime (simplified - based on success rate)
    if (m_totalRequests > 0) {
        metrics.serviceUptime = static_cast<double>(m_successfulRequests) / m_totalRequests;
    } else {
        metrics.serviceUptime = 1.0;
    }
    
    // Cache hit rate would need to be tracked separately
    metrics.cacheHitRate = 0.0;
    
    return metrics;
}

void WeatherAggregator::onServiceForecastReady(QList<WeatherData*> data) {
    qint64 responseTime = m_requestTimer.elapsed();
    m_responseTimes.append(responseTime);
    
    // Keep only last 100 response times
    if (m_responseTimes.size() > 100) {
        m_responseTimes.removeFirst();
    }
    
    WeatherService* service = qobject_cast<WeatherService*>(sender());
    if (service) {
        updateServiceAvailability(service, true, responseTime);
    }
    
    m_successfulRequests++;
    m_timeoutTimer->stop();
    
    emit forecastReady(data);
    emit metricsUpdated(getMetrics());
}

void WeatherAggregator::onServiceCurrentReady(WeatherData* data) {
    WeatherService* service = qobject_cast<WeatherService*>(sender());
    if (service) {
        updateServiceAvailability(service, true, m_requestTimer.elapsed());
    }
    
    emit currentReady(data);
}

void WeatherAggregator::onServiceError(QString message) {
    WeatherService* service = qobject_cast<WeatherService*>(sender());
    if (service) {
        updateServiceAvailability(service, false, 0);
    }
    
    m_failedRequests++;
    
    // Try fallback if strategy allows
    if (m_strategy == Fallback && !m_pendingRequests[m_currentRequestKey].isEmpty()) {
        m_pendingRequests[m_currentRequestKey].removeFirst();
        if (!m_pendingRequests[m_currentRequestKey].isEmpty()) {
            // Try next service
            // This would need location info - simplified for now
            return;
        }
    }
    
    emit error(message);
    emit metricsUpdated(getMetrics());
}

void WeatherAggregator::onTimeout() {
    m_failedRequests++;
    emit error("Request timeout");
    emit metricsUpdated(getMetrics());
}

void WeatherAggregator::updateServiceAvailability(WeatherService* service, bool success, qint64 responseTime) {
    for (ServiceEntry& entry : m_services) {
        if (entry.service == service) {
            entry.available = success;
            if (success) {
                entry.lastResponseTime = responseTime;
                entry.successCount++;
                entry.lastSuccessTime = QDateTime::currentDateTime();
            } else {
                entry.failureCount++;
                m_lastFailureTime[service] = QDateTime::currentDateTime();
            }
            break;
        }
    }
}

double WeatherAggregator::calculateConfidence(WeatherService* service) const {
    for (const ServiceEntry& entry : m_services) {
        if (entry.service == service) {
            int total = entry.successCount + entry.failureCount;
            if (total == 0) return 0.5; // Default confidence
            
            double successRate = static_cast<double>(entry.successCount) / total;
            return successRate;
        }
    }
    return 0.0;
}

QList<WeatherData*> WeatherAggregator::mergeForecasts(const QList<QList<WeatherData*>>& forecasts) {
    // Simplified merge - would need more sophisticated logic
    if (forecasts.isEmpty()) return QList<WeatherData*>();
    return forecasts.first(); // Return first for now
}

WeatherData* WeatherAggregator::mergeCurrentWeather(const QList<WeatherData*>& currentData) {
    if (currentData.isEmpty()) return nullptr;
    return currentData.first(); // Simplified
}

