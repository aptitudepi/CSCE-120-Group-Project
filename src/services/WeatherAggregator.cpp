#include "services/WeatherAggregator.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>
#include <QtMath>
#include <QMap>

WeatherAggregator::WeatherAggregator(QObject *parent)
    : QObject(parent)
    , m_strategy(PrimaryOnly)
    , m_timeoutTimer(new QTimer(this))
    , m_movingAverageFilter(new MovingAverageFilter(this))
    , m_movingAverageEnabled(false)
    , m_totalRequests(0)
    , m_successfulRequests(0)
    , m_failedRequests(0)
    , m_startTime(QDateTime::currentDateTime())
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(10000); // 10 second timeout
    connect(m_timeoutTimer, &QTimer::timeout, this, &WeatherAggregator::onTimeout);
    
    // Configure moving average filter defaults
    m_movingAverageFilter->setWindowSize(10);
    m_movingAverageFilter->setAlpha(0.2);
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

void WeatherAggregator::setMovingAverageEnabled(bool enabled) {
    m_movingAverageEnabled = enabled;
}

void WeatherAggregator::setMovingAverageWindowSize(int windowSize) {
    if (m_movingAverageFilter) {
        m_movingAverageFilter->setWindowSize(windowSize);
    }
}

void WeatherAggregator::setMovingAverageType(MovingAverageFilter::MovingAverageType type) {
    if (m_movingAverageFilter) {
        m_movingAverageFilter->setType(type);
    }
}

void WeatherAggregator::setMovingAverageAlpha(double alpha) {
    if (m_movingAverageFilter) {
        m_movingAverageFilter->setAlpha(alpha);
    }
}

void WeatherAggregator::fetchForecast(double latitude, double longitude) {
    m_requestTimer.start();
    m_totalRequests++;
    
    QString cacheKey = QString("%1_%2").arg(latitude, 0, 'f', 4).arg(longitude, 0, 'f', 4);
    m_currentRequestKey = cacheKey;
    m_currentLat = latitude;
    m_currentLon = longitude;
    
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
    m_pendingForecasts[cacheKey] = QList<ForecastWithService>();
    m_pendingCurrentWeather[cacheKey] = QList<WeatherData*>();
    m_receivedServices[cacheKey] = QList<WeatherService*>();
    m_serviceResponseTimes[cacheKey] = QMap<WeatherService*, qint64>();
    
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
    if (!service) {
        return;
    }
    
    updateServiceAvailability(service, true, responseTime);
    m_successfulRequests++;
    
    // Handle based on strategy
    if (m_strategy == WeightedAverage || m_strategy == BestAvailable) {
        // Collect forecasts from multiple sources
        QString cacheKey = m_currentRequestKey;
        
        // Store service response time
        m_serviceResponseTimes[cacheKey][service] = responseTime;
        
        // Add this service's forecasts to pending list with service info
        ForecastWithService forecastEntry;
        forecastEntry.forecasts = data;
        forecastEntry.service = service;
        forecastEntry.responseTime = responseTime;
        m_pendingForecasts[cacheKey].append(forecastEntry);
        m_receivedServices[cacheKey].append(service);
        
        // Check if all services have responded
        QList<WeatherService*> expectedServices = m_pendingRequests[cacheKey];
        QList<WeatherService*> receivedServices = m_receivedServices[cacheKey];
        
        if (receivedServices.size() >= expectedServices.size()) {
            // All services have responded, merge the forecasts
            QList<WeatherData*> mergedForecasts;
            
            if (m_strategy == WeightedAverage) {
                mergedForecasts = mergeForecasts(m_pendingForecasts[cacheKey]);
                
                // Apply moving average smoothing if enabled
                if (m_movingAverageEnabled && !mergedForecasts.isEmpty()) {
                    // Note: Historical data would be retrieved separately
                    // For now, smooth using only the merged forecasts
                    mergedForecasts = m_movingAverageFilter->smoothForecast(mergedForecasts);
                }
            } else {
                // BestAvailable: use service with highest confidence
                WeatherService* bestService = nullptr;
                double bestConfidence = 0.0;
                
                for (WeatherService* svc : receivedServices) {
                    double confidence = calculateConfidence(svc);
                    if (confidence > bestConfidence) {
                        bestConfidence = confidence;
                        bestService = svc;
                    }
                }
                
                // Find forecasts from best service
                for (const ForecastWithService& entry : m_pendingForecasts[cacheKey]) {
                    if (entry.service == bestService) {
                        mergedForecasts = entry.forecasts;
                        break;
                    }
                }
                
                if (mergedForecasts.isEmpty() && !m_pendingForecasts[cacheKey].isEmpty()) {
                    mergedForecasts = m_pendingForecasts[cacheKey].first().forecasts;
                }
            }
            
            // Clean up
            m_pendingForecasts.remove(cacheKey);
            m_receivedServices.remove(cacheKey);
            m_serviceResponseTimes.remove(cacheKey);
            m_pendingRequests.remove(cacheKey);
            
            m_timeoutTimer->stop();
            emit forecastReady(mergedForecasts);
            emit metricsUpdated(getMetrics());
        }
        // Otherwise, wait for more services to respond
    } else {
        // PrimaryOnly or Fallback: emit immediately
        m_timeoutTimer->stop();
        emit forecastReady(data);
        emit metricsUpdated(getMetrics());
    }
}

void WeatherAggregator::onServiceCurrentReady(WeatherData* data) {
    WeatherService* service = qobject_cast<WeatherService*>(sender());
    if (!service) {
        return;
    }
    
    qint64 responseTime = m_requestTimer.elapsed();
    updateServiceAvailability(service, true, responseTime);
    
    // Handle based on strategy
    if (m_strategy == WeightedAverage || m_strategy == BestAvailable) {
        // Collect current weather from multiple sources
        QString cacheKey = m_currentRequestKey;
        m_pendingCurrentWeather[cacheKey].append(data);
        
        // Check if all services have responded (use same logic as forecasts)
        QList<WeatherService*> expectedServices = m_pendingRequests[cacheKey];
        QList<WeatherService*> receivedServices = m_receivedServices[cacheKey];
        
        if (receivedServices.size() >= expectedServices.size() && 
            m_pendingCurrentWeather[cacheKey].size() >= expectedServices.size()) {
            // All services have responded, merge current weather
            WeatherData* mergedCurrent = nullptr;
            
            if (m_strategy == WeightedAverage) {
                mergedCurrent = mergeCurrentWeather(m_pendingCurrentWeather[cacheKey]);
            } else {
                // BestAvailable: use service with highest confidence
                WeatherService* bestService = nullptr;
                double bestConfidence = 0.0;
                
                for (WeatherService* svc : receivedServices) {
                    double confidence = calculateConfidence(svc);
                    if (confidence > bestConfidence) {
                        bestConfidence = confidence;
                        bestService = svc;
                    }
                }
                
                // Find current weather from best service
                // For now, use first available
                if (!m_pendingCurrentWeather[cacheKey].isEmpty()) {
                    mergedCurrent = m_pendingCurrentWeather[cacheKey].first();
                }
            }
            
            // Clean up
            m_pendingCurrentWeather.remove(cacheKey);
            
            if (mergedCurrent) {
                emit currentReady(mergedCurrent);
            }
        }
        // Otherwise, wait for more services to respond
    } else {
        // PrimaryOnly or Fallback: emit immediately
        emit currentReady(data);
    }
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

double WeatherAggregator::calculateWeight(WeatherService* service, qint64 responseTime) const {
    if (!service) {
        return 1.0;
    }
    
    // Find service entry
    const ServiceEntry* entry = nullptr;
    for (const ServiceEntry& e : m_services) {
        if (e.service == service) {
            entry = &e;
            break;
        }
    }
    
    if (!entry) {
        return 1.0;
    }
    
    // Calculate weight based on multiple factors
    // 1. Confidence (success rate): 0.0 to 1.0, contributes 40%
    double confidence = calculateConfidence(service);
    double confidenceWeight = confidence * 0.4;
    
    // 2. Priority: normalized to 0.0-1.0, contributes 20%
    // Find max priority for normalization
    int maxPriority = 0;
    for (const ServiceEntry& e : m_services) {
        if (e.priority > maxPriority) {
            maxPriority = e.priority;
        }
    }
    double priorityWeight = 0.2;
    if (maxPriority > 0) {
        priorityWeight = (static_cast<double>(entry->priority) / maxPriority) * 0.2;
    }
    
    // 3. Recency: newer data gets higher weight, contributes 20%
    double recencyWeight = 0.2;
    if (entry->lastSuccessTime.isValid()) {
        qint64 secondsSinceSuccess = entry->lastSuccessTime.secsTo(QDateTime::currentDateTime());
        // Decay factor: more recent = higher weight
        // Half-life of 1 hour (3600 seconds)
        double decayFactor = qExp(-static_cast<double>(secondsSinceSuccess) / 3600.0);
        recencyWeight = decayFactor * 0.2;
    }
    
    // 4. Response time: faster = higher weight, contributes 20%
    double responseWeight = 0.2;
    if (responseTime > 0) {
        // Normalize: faster response = higher weight
        // Assume 10 seconds is "slow", scale accordingly
        double normalizedTime = qMin(1.0, static_cast<double>(responseTime) / 10000.0);
        responseWeight = (1.0 - normalizedTime) * 0.2;
    }
    
    // Total weight is sum of all factors
    double totalWeight = confidenceWeight + priorityWeight + recencyWeight + responseWeight;
    
    // Ensure minimum weight (at least 0.1)
    return qMax(0.1, totalWeight);
}

QDateTime WeatherAggregator::binTimestamp(const QDateTime& timestamp, int binMinutes) const {
    if (!timestamp.isValid()) {
        return timestamp;
    }
    
    QDateTime binned = timestamp;
    
    // Round down to nearest bin
    int minutes = binned.time().minute();
    int roundedMinutes = (minutes / binMinutes) * binMinutes;
    
    QTime roundedTime = binned.time();
    roundedTime.setHMS(roundedTime.hour(), roundedMinutes, 0, 0);
    binned.setTime(roundedTime);
    
    return binned;
}

QList<WeatherData*> WeatherAggregator::mergeForecasts(const QList<ForecastWithService>& forecastsWithServices) {
    if (forecastsWithServices.isEmpty()) {
        return QList<WeatherData*>();
    }
    
    // If only one source, return it directly
    if (forecastsWithServices.size() == 1) {
        return forecastsWithServices.first().forecasts;
    }
    
    // Group forecasts by timestamp (bin to 30-minute intervals)
    QMap<QDateTime, QList<QPair<WeatherData*, double>>> timeBins; // timestamp -> list of (data, weight)
    
    // Collect all forecasts with their weights
    for (const ForecastWithService& forecastEntry : forecastsWithServices) {
        const QList<WeatherData*>& sourceForecasts = forecastEntry.forecasts;
        WeatherService* service = forecastEntry.service;
        qint64 responseTime = forecastEntry.responseTime;
        
        if (sourceForecasts.isEmpty()) {
            continue;
        }
        
        // Calculate weight for this service
        double weight = calculateWeight(service, responseTime);
        
        // Add each forecast to appropriate time bin
        for (WeatherData* data : sourceForecasts) {
            if (!data || !data->timestamp().isValid()) {
                continue;
            }
            
            QDateTime binnedTime = binTimestamp(data->timestamp(), 30);
            timeBins[binnedTime].append(qMakePair(data, weight));
        }
    }
    
    if (timeBins.isEmpty()) {
        // Fallback: return first source's forecasts
        return forecasts.first();
    }
    
    // Merge forecasts for each time bin
    QList<WeatherData*> mergedForecasts;
    QDateTimeList sortedTimes = timeBins.keys();
    std::sort(sortedTimes.begin(), sortedTimes.end());
    
    for (const QDateTime& binTime : sortedTimes) {
        const QList<QPair<WeatherData*, double>>& binData = timeBins[binTime];
        
        if (binData.isEmpty()) {
            continue;
        }
        
        // Calculate total weight
        double totalWeight = 0.0;
        for (const QPair<WeatherData*, double>& pair : binData) {
            totalWeight += pair.second;
        }
        
        if (totalWeight <= 0.0) {
            // Use first data point if no valid weights
            mergedForecasts.append(binData.first().first);
            continue;
        }
        
        // Create merged WeatherData
        WeatherData* merged = new WeatherData();
        
        // Use first data point's location and timestamp as base
        WeatherData* firstData = binData.first().first;
        merged->setLatitude(firstData->latitude());
        merged->setLongitude(firstData->longitude());
        merged->setTimestamp(binTime);
        
        // Weighted averages for numeric parameters
        double weightedTemp = 0.0;
        double weightedFeelsLike = 0.0;
        double weightedPressure = 0.0;
        double weightedWindSpeed = 0.0;
        double weightedPrecipProb = 0.0;
        double weightedPrecipIntensity = 0.0;
        double weightedCloudCover = 0.0;
        double weightedVisibility = 0.0;
        double weightedUvIndex = 0.0;
        
        // Vector average for wind direction
        double windX = 0.0;
        double windY = 0.0;
        double windWeight = 0.0;
        
        // Weighted average for humidity (already integer)
        double weightedHumidity = 0.0;
        
        // Collect weather conditions (use most common or highest weight)
        QMap<QString, double> conditionWeights;
        QString mostWeightedCondition;
        QString mostWeightedDescription;
        double maxConditionWeight = 0.0;
        
        for (const QPair<WeatherData*, double>& pair : binData) {
            WeatherData* data = pair.first;
            double weight = pair.second;
            double normalizedWeight = weight / totalWeight;
            
            // Temperature
            if (data->temperature() != 0.0 || qAbs(data->temperature()) > 0.01) {
                weightedTemp += data->temperature() * normalizedWeight;
            }
            
            // Feels like
            if (data->feelsLike() != 0.0 || qAbs(data->feelsLike()) > 0.01) {
                weightedFeelsLike += data->feelsLike() * normalizedWeight;
            }
            
            // Pressure
            if (data->pressure() > 0.0) {
                weightedPressure += data->pressure() * normalizedWeight;
            }
            
            // Wind speed
            weightedWindSpeed += data->windSpeed() * normalizedWeight;
            
            // Wind direction (vector average)
            if (data->windDirection() >= 0 && data->windSpeed() > 0) {
                double radians = qDegreesToRadians(static_cast<double>(data->windDirection()));
                windX += qCos(radians) * data->windSpeed() * normalizedWeight;
                windY += qSin(radians) * data->windSpeed() * normalizedWeight;
                windWeight += normalizedWeight;
            }
            
            // Precipitation
            weightedPrecipProb += data->precipProbability() * normalizedWeight;
            weightedPrecipIntensity += data->precipIntensity() * normalizedWeight;
            
            // Humidity
            if (data->humidity() > 0) {
                weightedHumidity += static_cast<double>(data->humidity()) * normalizedWeight;
            }
            
            // Cloud cover
            weightedCloudCover += static_cast<double>(data->cloudCover()) * normalizedWeight;
            
            // Visibility
            if (data->visibility() > 0) {
                weightedVisibility += static_cast<double>(data->visibility()) * normalizedWeight;
            }
            
            // UV Index
            weightedUvIndex += static_cast<double>(data->uvIndex()) * normalizedWeight;
            
            // Weather condition (weighted selection)
            QString condition = data->weatherCondition();
            if (!condition.isEmpty()) {
                conditionWeights[condition] += normalizedWeight;
                if (conditionWeights[condition] > maxConditionWeight) {
                    maxConditionWeight = conditionWeights[condition];
                    mostWeightedCondition = condition;
                }
            }
            
            QString description = data->weatherDescription();
            if (!description.isEmpty() && normalizedWeight > 0.5) {
                mostWeightedDescription = description; // Use from highest weight source
            }
        }
        
        // Set merged values
        merged->setTemperature(weightedTemp);
        merged->setFeelsLike(weightedFeelsLike > 0.0 ? weightedFeelsLike : weightedTemp);
        merged->setPressure(weightedPressure);
        merged->setWindSpeed(weightedWindSpeed);
        
        // Calculate wind direction from vector average
        if (windWeight > 0 && (qAbs(windX) > 0.001 || qAbs(windY) > 0.001)) {
            double avgDirectionRadians = qAtan2(windY, windX);
            int avgDirection = static_cast<int>(qRadiansToDegrees(avgDirectionRadians));
            if (avgDirection < 0) {
                avgDirection += 360;
            }
            merged->setWindDirection(avgDirection);
        } else if (!binData.isEmpty()) {
            merged->setWindDirection(binData.first().first->windDirection());
        }
        
        merged->setPrecipProbability(qMax(0.0, qMin(1.0, weightedPrecipProb)));
        merged->setPrecipIntensity(qMax(0.0, weightedPrecipIntensity));
        merged->setHumidity(static_cast<int>(qRound(weightedHumidity)));
        merged->setCloudCover(static_cast<int>(qRound(weightedCloudCover)));
        merged->setVisibility(static_cast<int>(qRound(weightedVisibility)));
        merged->setUvIndex(static_cast<int>(qRound(weightedUvIndex)));
        merged->setWeatherCondition(mostWeightedCondition.isEmpty() ? 
                                    (binData.first().first->weatherCondition()) : mostWeightedCondition);
        merged->setWeatherDescription(mostWeightedDescription.isEmpty() ?
                                     (binData.first().first->weatherDescription()) : mostWeightedDescription);
        
        mergedForecasts.append(merged);
    }
    
    return mergedForecasts;
}

WeatherData* WeatherAggregator::mergeCurrentWeather(const QList<WeatherData*>& currentData) {
    if (currentData.isEmpty()) {
        return nullptr;
    }
    
    // If only one source, return it directly
    if (currentData.size() == 1) {
        return currentData.first();
    }
    
    // Calculate weights for each source
    QList<double> weights;
    double totalWeight = 0.0;
    
    for (WeatherData* data : currentData) {
        // Find corresponding service for weight calculation
        WeatherService* service = nullptr;
        for (const ServiceEntry& entry : m_services) {
            // Try to match by service name or other characteristics
            // For simplicity, use service order
            service = entry.service;
            break;
        }
        
        qint64 responseTime = 0;
        for (const ServiceEntry& entry : m_services) {
            if (entry.service == service) {
                responseTime = entry.lastResponseTime;
                break;
            }
        }
        
        double weight = calculateWeight(service, responseTime);
        weights.append(weight);
        totalWeight += weight;
    }
    
    if (totalWeight <= 0.0) {
        // Fallback: equal weights
        for (int i = 0; i < weights.size(); ++i) {
            weights[i] = 1.0;
        }
        totalWeight = static_cast<double>(weights.size());
    }
    
    // Create merged WeatherData
    WeatherData* merged = new WeatherData();
    
    // Use first data point's location and timestamp as base
    WeatherData* firstData = currentData.first();
    merged->setLatitude(firstData->latitude());
    merged->setLongitude(firstData->longitude());
    merged->setTimestamp(firstData->timestamp());
    
    // Weighted averages
    double weightedTemp = 0.0;
    double weightedFeelsLike = 0.0;
    double weightedPressure = 0.0;
    double weightedWindSpeed = 0.0;
    double weightedPrecipProb = 0.0;
    double weightedPrecipIntensity = 0.0;
    double weightedCloudCover = 0.0;
    double weightedVisibility = 0.0;
    double weightedUvIndex = 0.0;
    double weightedHumidity = 0.0;
    
    // Vector average for wind direction
    double windX = 0.0;
    double windY = 0.0;
    
    // Weather condition from highest weight source
    QString mostWeightedCondition;
    QString mostWeightedDescription;
    double maxWeight = 0.0;
    int maxWeightIndex = 0;
    
    for (int i = 0; i < currentData.size(); ++i) {
        WeatherData* data = currentData[i];
        double weight = weights[i];
        double normalizedWeight = weight / totalWeight;
        
        if (normalizedWeight > maxWeight) {
            maxWeight = normalizedWeight;
            maxWeightIndex = i;
        }
        
        // Temperature
        if (data->temperature() != 0.0 || qAbs(data->temperature()) > 0.01) {
            weightedTemp += data->temperature() * normalizedWeight;
        }
        
        // Feels like
        if (data->feelsLike() != 0.0 || qAbs(data->feelsLike()) > 0.01) {
            weightedFeelsLike += data->feelsLike() * normalizedWeight;
        }
        
        // Pressure
        if (data->pressure() > 0.0) {
            weightedPressure += data->pressure() * normalizedWeight;
        }
        
        // Wind
        weightedWindSpeed += data->windSpeed() * normalizedWeight;
        if (data->windDirection() >= 0 && data->windSpeed() > 0) {
            double radians = qDegreesToRadians(static_cast<double>(data->windDirection()));
            windX += qCos(radians) * data->windSpeed() * normalizedWeight;
            windY += qSin(radians) * data->windSpeed() * normalizedWeight;
        }
        
        // Precipitation
        weightedPrecipProb += data->precipProbability() * normalizedWeight;
        weightedPrecipIntensity += data->precipIntensity() * normalizedWeight;
        
        // Humidity
        if (data->humidity() > 0) {
            weightedHumidity += static_cast<double>(data->humidity()) * normalizedWeight;
        }
        
        // Cloud cover
        weightedCloudCover += static_cast<double>(data->cloudCover()) * normalizedWeight;
        
        // Visibility
        if (data->visibility() > 0) {
            weightedVisibility += static_cast<double>(data->visibility()) * normalizedWeight;
        }
        
        // UV Index
        weightedUvIndex += static_cast<double>(data->uvIndex()) * normalizedWeight;
    }
    
    // Set merged values
    merged->setTemperature(weightedTemp);
    merged->setFeelsLike(weightedFeelsLike > 0.0 ? weightedFeelsLike : weightedTemp);
    merged->setPressure(weightedPressure);
    merged->setWindSpeed(weightedWindSpeed);
    
    // Calculate wind direction from vector average
    if (qAbs(windX) > 0.001 || qAbs(windY) > 0.001) {
        double avgDirectionRadians = qAtan2(windY, windX);
        int avgDirection = static_cast<int>(qRadiansToDegrees(avgDirectionRadians));
        if (avgDirection < 0) {
            avgDirection += 360;
        }
        merged->setWindDirection(avgDirection);
    } else {
        merged->setWindDirection(firstData->windDirection());
    }
    
    merged->setPrecipProbability(qMax(0.0, qMin(1.0, weightedPrecipProb)));
    merged->setPrecipIntensity(qMax(0.0, weightedPrecipIntensity));
    merged->setHumidity(static_cast<int>(qRound(weightedHumidity)));
    merged->setCloudCover(static_cast<int>(qRound(weightedCloudCover)));
    merged->setVisibility(static_cast<int>(qRound(weightedVisibility)));
    merged->setUvIndex(static_cast<int>(qRound(weightedUvIndex)));
    
    // Use weather condition from highest weight source
    WeatherData* maxWeightData = currentData[maxWeightIndex];
    merged->setWeatherCondition(maxWeightData->weatherCondition());
    merged->setWeatherDescription(maxWeightData->weatherDescription());
    
    return merged;
}

