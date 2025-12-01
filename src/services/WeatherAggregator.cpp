#include "services/WeatherAggregator.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>
#include <QtMath>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QProcessEnvironment>

WeatherAggregator::WeatherAggregator(QObject *parent)
    : QObject(parent)
    , m_strategy(PrimaryOnly)
    , m_timeoutTimer(new QTimer(this))
    , m_movingAverageFilter(new MovingAverageFilter(this))
    , m_movingAverageEnabled(false)
    , m_defaultTimeoutMs(30000)
    , m_spatioTimeoutMs(60000)
    , m_totalRequests(0)
    , m_successfulRequests(0)
    , m_failedRequests(0)
    , m_startTime(QDateTime::currentDateTime())
    , m_spatioTemporalEngine(new SpatioTemporalEngine(this))
    , m_spatioTemporalEnabled(true)
    , m_spatioTemporalActive(false)
    , m_gridMatchTolerance(0.01)
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(m_defaultTimeoutMs);
    connect(m_timeoutTimer, &QTimer::timeout, this, &WeatherAggregator::onTimeout);
    
    // Configure moving average filter defaults
    m_movingAverageFilter->setWindowSize(10);
    m_movingAverageFilter->setAlpha(0.2);

    auto envDouble = [](const char* key, double fallback) {
        QByteArray value = qgetenv(key);
        if (value.isEmpty()) {
            return fallback;
        }
        bool ok = false;
        double v = value.toDouble(&ok);
        return ok ? v : fallback;
    };

    auto envInt = [](const char* key, int fallback) {
        bool ok = false;
        int v = qEnvironmentVariableIntValue(key, &ok);
        return ok ? v : fallback;
    };

    // Configure spatio-temporal defaults (overridable via env)
    SpatioTemporalEngine::GridConfig gridConfig = m_spatioTemporalEngine->gridConfig();
    gridConfig.offsetDistanceKm = envDouble("HLW_SPATIAL_OFFSET_KM", gridConfig.offsetDistanceKm);
    m_spatioTemporalEngine->setGridConfig(gridConfig);

    m_defaultTimeoutMs = qMax(1000, envInt("HLW_TIMEOUT_MS", m_defaultTimeoutMs));
    int defaultSpatioTimeout = qMax(gridConfig.pointCount * 6000, m_defaultTimeoutMs * 2);
    m_spatioTimeoutMs = qMax(defaultSpatioTimeout, envInt("HLW_SPATIOTEMPORAL_TIMEOUT_MS", defaultSpatioTimeout));
    m_timeoutTimer->setInterval(m_defaultTimeoutMs);

    SpatioTemporalEngine::TemporalConfig temporalConfig = m_spatioTemporalEngine->temporalConfig();
    temporalConfig.outputGranularityMinutes = envInt("HLW_TEMPORAL_GRANULARITY_MIN",
                                                     temporalConfig.outputGranularityMinutes);
    temporalConfig.smoothingWindowMinutes = envInt("HLW_TEMPORAL_SMOOTHING_MIN",
                                                   temporalConfig.smoothingWindowMinutes);
    int interpolationMode = envInt("HLW_TEMPORAL_METHOD", static_cast<int>(temporalConfig.method));
    if (interpolationMode >= static_cast<int>(TemporalInterpolator::Linear) &&
        interpolationMode <= static_cast<int>(TemporalInterpolator::StepFunction)) {
        temporalConfig.method = static_cast<TemporalInterpolator::InterpolationMethod>(interpolationMode);
    }
    m_spatioTemporalEngine->setTemporalConfig(temporalConfig);

    SpatioTemporalEngine::SpatialConfig spatialConfig = m_spatioTemporalEngine->spatialConfig();
    spatialConfig.idwPower = envDouble("HLW_SPATIAL_IDW_POWER", spatialConfig.idwPower);
    spatialConfig.strategy = static_cast<SpatialInterpolator::InterpolationStrategy>(
        envInt("HLW_SPATIAL_STRATEGY", static_cast<int>(spatialConfig.strategy)));
    spatialConfig.missingPointsThreshold = envInt("HLW_SPATIAL_MISSING_THRESHOLD",
                                                  spatialConfig.missingPointsThreshold);
    m_spatioTemporalEngine->setSpatialConfig(spatialConfig);

    SpatioTemporalEngine::APIWeights weights = m_spatioTemporalEngine->apiWeights();
    double nwsWeight = envDouble("HLW_API_WEIGHT_NWS", 0.5);
    double pirateWeight = envDouble("HLW_API_WEIGHT_PIRATE", 0.5);
    if (nwsWeight <= 0.0 && pirateWeight <= 0.0) {
        nwsWeight = 0.5;
        pirateWeight = 0.5;
    }
    weights.weights["NWS"] = nwsWeight;
    weights.weights["PirateWeather"] = pirateWeight;
    m_spatioTemporalEngine->setAPIWeights(weights);

    m_gridMatchTolerance = envDouble("HLW_GRID_TOLERANCE_DEG", m_gridMatchTolerance);
    bool ok = false;
    int disableFlag = qEnvironmentVariableIntValue("HLW_DISABLE_SPATIOTEMPORAL", &ok);
    if (ok && disableFlag == 1) {
        m_spatioTemporalEnabled = false;
    }
}

WeatherAggregator::~WeatherAggregator() {
    resetSpatioTemporalState();
}

void WeatherAggregator::addService(WeatherService* service, int priority) {
    if (!service) return;
    
    ServiceEntry entry;
    entry.service = service;
    entry.priority = priority;
    entry.available = true;
    entry.lastResponseTime = 0;
    entry.successCount = 0;
    entry.failureCount = 0;
    entry.consecutiveFailures = 0;
    
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
    
    const bool useSpatio = shouldUseSpatioTemporal();
    int timeoutMs = useSpatio ? m_spatioTimeoutMs : m_defaultTimeoutMs;
    m_timeoutTimer->start(timeoutMs);
    
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
    
    if (useSpatio) {
        startSpatioTemporalRequest(latitude, longitude, availableServices);
        return;
    }

    // Track pending requests for legacy aggregation pipeline
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

    if (m_spatioTemporalActive && m_spatioContexts.contains(service)) {
        handleSpatioTemporalForecast(service, data);
        return;
    }

    const QString cacheKey = m_currentRequestKey;
    if (!m_pendingForecasts.contains(cacheKey)) {
        qDebug() << "WeatherAggregator ignoring forecast response from"
                 << service->serviceName() << "because no aggregation request is active";
        return;
    }
    
    updateServiceAvailability(service, true, responseTime);
    m_successfulRequests++;
    
    // Handle based on strategy
    if (m_strategy == WeightedAverage || m_strategy == BestAvailable) {
        // Collect forecasts from multiple sources
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
        QList<WeatherService*> expectedServices = m_pendingRequests.value(cacheKey);
        QList<WeatherService*> receivedServices = m_receivedServices.value(cacheKey);
        
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
    if (m_spatioTemporalActive) {
        // Current conditions are derived from the spatio-temporal pipeline
        // once the combined forecast is ready.
        return;
    }

    WeatherService* service = qobject_cast<WeatherService*>(sender());
    if (!service) {
        return;
    }

    const QString cacheKey = m_currentRequestKey;
    if (!m_pendingCurrentWeather.contains(cacheKey)) {
        qDebug() << "WeatherAggregator ignoring current-weather response from"
                 << service->serviceName() << "because no aggregation request is active";
        return;
    }
    
    qint64 responseTime = m_requestTimer.elapsed();
    updateServiceAvailability(service, true, responseTime);
    
    // Handle based on strategy
    if (m_strategy == WeightedAverage || m_strategy == BestAvailable) {
        // Collect current weather from multiple sources
        m_pendingCurrentWeather[cacheKey].append(data);
        
        // Check if all services have responded (use same logic as forecasts)
        QList<WeatherService*> expectedServices = m_pendingRequests.value(cacheKey);
        QList<WeatherService*> receivedServices = m_receivedServices.value(cacheKey);
        
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
        if (m_spatioTemporalActive && m_spatioContexts.contains(service)) {
            markServiceGridError(service, message);
            finalizeSpatioTemporalResult();
            return;
        }
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
    if (m_spatioTemporalActive) {
        qWarning() << "Spatio-temporal request timed out. Status:";
        for (auto it = m_spatioContexts.constBegin(); it != m_spatioContexts.constEnd(); ++it) {
            int completed = 0;
            for (const SpatioGridPointState& state : it.value().gridStates) {
                if (state.completed) completed++;
            }
            qWarning() << "  Service" << it.value().apiName << ":" 
                       << completed << "/" << it.value().gridStates.size() << "completed,"
                       << "hasTemporalResult:" << it.value().hasTemporalResult
                       << "hasError:" << it.value().hasError;
        }
        
        // Attempt to salvage partial results
        finalizeSpatioTemporalResult();
        if (!m_spatioTemporalActive) return; // Successfully finalized
        
        m_failedRequests++;
        emit error("Request timeout");
        resetSpatioTemporalState();
        return;
    }
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
                entry.consecutiveFailures = 0;
                entry.lastSuccessTime = QDateTime::currentDateTime();
            } else {
                entry.failureCount++;
                entry.consecutiveFailures++;
                m_lastFailureTime[service] = QDateTime::currentDateTime();
                
                if (entry.consecutiveFailures >= 10) {
                    qWarning() << "Service" << service->serviceName() << "failed" << entry.consecutiveFailures << "times consecutively. Triggering fallback.";
                    emit error(QString("Service %1 failed %2 times consecutively").arg(service->serviceName()).arg(entry.consecutiveFailures));
                }
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
        return forecastsWithServices.first().forecasts;
    }
    
    // Merge forecasts for each time bin
    QList<WeatherData*> mergedForecasts;
    QList<QDateTime> sortedTimes = timeBins.keys();
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

bool WeatherAggregator::shouldUseSpatioTemporal() const {
    if (!m_spatioTemporalEnabled) {
        return false;
    }
    if (m_strategy != WeightedAverage) {
        return false;
    }
    return !m_services.isEmpty();
}

void WeatherAggregator::startSpatioTemporalRequest(double latitude, double longitude,
                                                   const QList<WeatherService*>& services) {
    resetSpatioTemporalState();
    m_spatioTemporalActive = true;
    m_spatioGrid = m_spatioTemporalEngine->generateGrid(latitude, longitude);
    if (m_spatioGrid.isEmpty()) {
        emit error("Unable to generate spatial grid for request");
        m_spatioTemporalActive = false;
        return;
    }

    for (WeatherService* service : services) {
        if (!service) {
            continue;
        }
        SpatioServiceContext ctx;
        ctx.service = service;
        ctx.apiName = service->serviceName();
        for (const QPointF& point : m_spatioGrid) {
            SpatioGridPointState state;
            state.coordinate = point;
            ctx.gridStates.append(state);
        }
        m_spatioContexts.insert(service, ctx);

        for (const QPointF& point : m_spatioGrid) {
            service->fetchForecast(point.x(), point.y());
        }
    }
}

void WeatherAggregator::handleSpatioTemporalForecast(WeatherService* service, QList<WeatherData*> data) {
    if (!m_spatioContexts.contains(service)) {
        qWarning() << "Spatio-temporal response from unknown service" << service->serviceName();
        qDeleteAll(data);
        return;
    }

    updateServiceAvailability(service, true, m_requestTimer.elapsed());

    if (data.isEmpty()) {
        qWarning() << "Received empty forecast data for spatio-temporal grid from"
                   << service->serviceName();
        return;
    }

    double responseLat = data.first()->latitude();
    double responseLon = data.first()->longitude();
    int gridIndex = matchGridIndex(m_spatioGrid, responseLat, responseLon);
    if (gridIndex < 0) {
        qWarning() << "Unable to match grid point for" << responseLat << responseLon
                   << "from" << service->serviceName();
        qDeleteAll(data);
        return;
    }

    SpatioServiceContext& ctx = m_spatioContexts[service];
    if (gridIndex >= ctx.gridStates.size()) {
        qWarning() << "Grid index mismatch for service" << service->serviceName();
        qDeleteAll(data);
        return;
    }

    qDeleteAll(ctx.gridStates[gridIndex].forecasts);
    ctx.gridStates[gridIndex].forecasts = data;
    ctx.gridStates[gridIndex].completed = true;

    bool serviceComplete = std::all_of(ctx.gridStates.begin(), ctx.gridStates.end(),
        [](const SpatioGridPointState& state) { return state.completed; });

    if (serviceComplete) {
        processSpatioTemporalService(service);
    }
}

void WeatherAggregator::processSpatioTemporalService(WeatherService* service) {
    if (!m_spatioContexts.contains(service)) {
        return;
    }

    SpatioServiceContext& ctx = m_spatioContexts[service];
    if (ctx.hasTemporalResult || ctx.hasError) {
        return;
    }

    QSet<QDateTime> timestamps;
    for (const SpatioGridPointState& state : ctx.gridStates) {
        for (WeatherData* entry : state.forecasts) {
            if (entry) {
                timestamps.insert(entry->timestamp());
            }
        }
    }

    QList<QDateTime> sortedTimes = timestamps.values();
    std::sort(sortedTimes.begin(), sortedTimes.end());

    QList<WeatherData*> spatialTimeline;
    for (const QDateTime& ts : sortedTimes) {
        QList<WeatherData*> samples = buildSpatialSamples(ctx.gridStates, ts);
        if (samples.isEmpty()) {
            continue;
        }
        WeatherData* smoothed = m_spatioTemporalEngine->applySpatialSmoothing(samples, m_currentLat, m_currentLon);
        if (smoothed) {
            spatialTimeline.append(smoothed);
        }
    }

    ctx.spatialTimeline = spatialTimeline;
    ctx.temporalTimeline = m_spatioTemporalEngine->applyTemporalInterpolation(ctx.spatialTimeline);
    ctx.hasTemporalResult = true;

    for (SpatioGridPointState& state : ctx.gridStates) {
        qDeleteAll(state.forecasts);
        state.forecasts.clear();
        state.completed = false;
    }
    ctx.gridStates.clear();

    finalizeSpatioTemporalResult();
}

void WeatherAggregator::finalizeSpatioTemporalResult() {
    if (!m_spatioTemporalActive) {
        return;
    }

    bool waitingForService = false;
    for (auto it = m_spatioContexts.cbegin(); it != m_spatioContexts.cend(); ++it) {
        if (!it.value().hasTemporalResult && !it.value().hasError) {
            waitingForService = true;
            break;
        }
    }

    if (waitingForService) {
        // If we are here due to timeout, force completion of whatever we have
        if (!m_timeoutTimer->isActive()) {
            qDebug() << "Forcing finalization of spatio-temporal result due to timeout";
        } else {
            return;
        }
    }

    QMap<QString, QList<WeatherData*>> apiForecasts;
    for (auto it = m_spatioContexts.begin(); it != m_spatioContexts.end(); ++it) {
        const SpatioServiceContext& ctx = it.value();
        if (ctx.hasTemporalResult && !ctx.temporalTimeline.isEmpty()) {
            apiForecasts.insert(ctx.apiName, ctx.temporalTimeline);
        }
    }

    if (apiForecasts.isEmpty()) {
        emit error("No API data available for spatio-temporal aggregation");
        resetSpatioTemporalState();
        return;
    }

    QList<WeatherData*> combined = m_spatioTemporalEngine->combineAPIForecasts(apiForecasts);
    if (combined.isEmpty()) {
        emit error("Failed to combine API forecasts");
        resetSpatioTemporalState();
        return;
    }

    m_timeoutTimer->stop();
    m_successfulRequests++;
    emit forecastReady(combined);
    emit metricsUpdated(getMetrics());

    resetSpatioTemporalState();
    m_spatioTemporalActive = false;
}

void WeatherAggregator::resetSpatioTemporalState(bool deleteTimelines) {
    for (auto it = m_spatioContexts.begin(); it != m_spatioContexts.end(); ++it) {
        // Cancel any pending network requests for this service to prevent
        // responses from matching against a future (different) grid.
        it.key()->cancelActiveRequests();
        
        for (SpatioGridPointState& state : it.value().gridStates) {
            qDeleteAll(state.forecasts);
            state.forecasts.clear();
        }
        if (deleteTimelines) {
            qDeleteAll(it.value().spatialTimeline);
            qDeleteAll(it.value().temporalTimeline);
        }
        it.value().spatialTimeline.clear();
        it.value().temporalTimeline.clear();
        it.value().gridStates.clear();
        it.value().hasTemporalResult = false;
        it.value().hasError = false;
    }
    m_spatioContexts.clear();
    m_spatioGrid.clear();
    m_spatioTemporalActive = false;
}

void WeatherAggregator::cancelSpatioTemporalRequests() {
    if (!m_spatioTemporalActive) {
        return;
    }
    resetSpatioTemporalState();
}

int WeatherAggregator::matchGridIndex(const QList<QPointF>& grid, double lat, double lon) const {
    for (int i = 0; i < grid.size(); ++i) {
        if (qAbs(grid[i].x() - lat) <= m_gridMatchTolerance &&
            qAbs(grid[i].y() - lon) <= m_gridMatchTolerance) {
            return i;
        }
    }
    return -1;
}

QList<WeatherData*> WeatherAggregator::buildSpatialSamples(const QVector<SpatioGridPointState>& states,
                                                           const QDateTime& timestamp) const {
    QList<WeatherData*> samples;
    for (const SpatioGridPointState& state : states) {
        for (WeatherData* entry : state.forecasts) {
            if (entry && entry->timestamp() == timestamp) {
                samples.append(entry);
                break;
            }
        }
    }
    return samples;
}

void WeatherAggregator::markServiceGridError(WeatherService* service, const QString& errorMessage) {
    if (!m_spatioContexts.contains(service)) {
        return;
    }
    SpatioServiceContext& ctx = m_spatioContexts[service];
    ctx.hasError = true;
    qWarning() << "Spatio-temporal request failed for" << ctx.apiName << ":" << errorMessage;
}

