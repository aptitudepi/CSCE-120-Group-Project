#include "services/MovingAverageFilter.h"
#include "models/WeatherData.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <QDateTime>

MovingAverageFilter::MovingAverageFilter(QObject *parent)
    : QObject(parent)
    , m_type(Simple)
    , m_defaultWindowSize(10)
    , m_alpha(0.2)
{
    // Set default window sizes per parameter type
    m_parameterWindowSizes["temperature"] = 10;
    m_parameterWindowSizes["precipitation"] = 5;  // More reactive to precip changes
    m_parameterWindowSizes["wind"] = 15;
    m_parameterWindowSizes["humidity"] = 10;
    m_parameterWindowSizes["pressure"] = 10;
    m_parameterWindowSizes["default"] = 10;
}

MovingAverageFilter::~MovingAverageFilter() {
    clear();
}

void MovingAverageFilter::setType(MovingAverageType type) {
    m_type = type;
}

void MovingAverageFilter::setWindowSize(int windowSize) {
    if (windowSize > 0) {
        m_defaultWindowSize = windowSize;
    }
}

void MovingAverageFilter::setWindowSize(const QString& parameter, int windowSize) {
    if (windowSize > 0) {
        m_parameterWindowSizes[parameter.toLower()] = windowSize;
    }
}

void MovingAverageFilter::setAlpha(double alpha) {
    if (alpha >= 0.0 && alpha <= 1.0) {
        m_alpha = alpha;
    }
}

void MovingAverageFilter::addDataPoint(WeatherData* data) {
    if (!data) {
        return;
    }
    
    // Create a copy to store
    WeatherData* copy = new WeatherData(this);
    copy->setLatitude(data->latitude());
    copy->setLongitude(data->longitude());
    copy->setTimestamp(data->timestamp());
    copy->setTemperature(data->temperature());
    copy->setFeelsLike(data->feelsLike());
    copy->setHumidity(data->humidity());
    copy->setPressure(data->pressure());
    copy->setWindSpeed(data->windSpeed());
    copy->setWindDirection(data->windDirection());
    copy->setPrecipProbability(data->precipProbability());
    copy->setPrecipIntensity(data->precipIntensity());
    copy->setCloudCover(data->cloudCover());
    copy->setVisibility(data->visibility());
    copy->setUvIndex(data->uvIndex());
    copy->setWeatherCondition(data->weatherCondition());
    copy->setWeatherDescription(data->weatherDescription());
    
    m_dataPoints.append(copy);
    
    // Limit stored data points to prevent memory growth
    const int MAX_STORED_POINTS = 1000;
    if (m_dataPoints.size() > MAX_STORED_POINTS) {
        delete m_dataPoints.takeFirst();
    }
}

WeatherData* MovingAverageFilter::getMovingAverage(int windowSize) {
    if (m_dataPoints.isEmpty()) {
        return nullptr;
    }
    
    if (windowSize < 0) {
        windowSize = m_defaultWindowSize;
    }
    
    // Use minimum of available points and window size
    int actualWindowSize = qMin(windowSize, m_dataPoints.size());
    if (actualWindowSize <= 0) {
        return m_dataPoints.last();
    }
    
    // Get last N points
    QList<WeatherData*> recentPoints = m_dataPoints.mid(m_dataPoints.size() - actualWindowSize);
    
    // Create averaged data point
    WeatherData* averaged = new WeatherData(this);
    
    // Use latest point's location and timestamp as base
    WeatherData* latest = recentPoints.last();
    averaged->setLatitude(latest->latitude());
    averaged->setLongitude(latest->longitude());
    averaged->setTimestamp(latest->timestamp());
    
    // Calculate averages for each parameter
    QList<double> temps, feelsLike, pressures, windSpeeds, precipProbs, precipIntensities;
    QList<int> windDirections, humidities, cloudCovers, visibilities, uvIndices;
    QList<double> windSpeedsForDirection;
    
    for (WeatherData* data : recentPoints) {
        temps.append(data->temperature());
        feelsLike.append(data->feelsLike());
        pressures.append(data->pressure());
        windSpeeds.append(data->windSpeed());
        precipProbs.append(data->precipProbability());
        precipIntensities.append(data->precipIntensity());
        windDirections.append(data->windDirection());
        humidities.append(data->humidity());
        cloudCovers.append(data->cloudCover());
        visibilities.append(data->visibility());
        uvIndices.append(data->uvIndex());
        
        if (data->windSpeed() > 0) {
            windSpeedsForDirection.append(data->windSpeed());
        }
    }
    
    // Simple moving averages
    int tempWindow = m_parameterWindowSizes.value("temperature", m_defaultWindowSize);
    averaged->setTemperature(calculateSimpleAverage(temps, qMin(tempWindow, temps.size())));
    
    int precipWindow = m_parameterWindowSizes.value("precipitation", m_defaultWindowSize);
    averaged->setPrecipProbability(calculateSimpleAverage(precipProbs, qMin(precipWindow, precipProbs.size())));
    averaged->setPrecipIntensity(calculateSimpleAverage(precipIntensities, qMin(precipWindow, precipIntensities.size())));
    
    int windWindow = m_parameterWindowSizes.value("wind", m_defaultWindowSize);
    averaged->setWindSpeed(calculateSimpleAverage(windSpeeds, qMin(windWindow, windSpeeds.size())));
    averaged->setWindDirection(windDirectionAverage(windDirections, windSpeeds, qMin(windWindow, windDirections.size())));
    
    averaged->setFeelsLike(calculateSimpleAverage(feelsLike, qMin(actualWindowSize, feelsLike.size())));
    averaged->setPressure(calculateSimpleAverage(pressures, qMin(actualWindowSize, pressures.size())));
    averaged->setHumidity(static_cast<int>(qRound(calculateSimpleAverage(
        QList<double>(humidities.begin(), humidities.end()), qMin(actualWindowSize, humidities.size())))));
    averaged->setCloudCover(static_cast<int>(qRound(calculateSimpleAverage(
        QList<double>(cloudCovers.begin(), cloudCovers.end()), qMin(actualWindowSize, cloudCovers.size())))));
    averaged->setVisibility(static_cast<int>(qRound(calculateSimpleAverage(
        QList<double>(visibilities.begin(), visibilities.end()), qMin(actualWindowSize, visibilities.size())))));
    averaged->setUvIndex(static_cast<int>(qRound(calculateSimpleAverage(
        QList<double>(uvIndices.begin(), uvIndices.end()), qMin(actualWindowSize, uvIndices.size())))));
    
    // Use latest point's weather condition/description
    averaged->setWeatherCondition(latest->weatherCondition());
    averaged->setWeatherDescription(latest->weatherDescription());
    
    return averaged;
}

WeatherData* MovingAverageFilter::getExponentialMovingAverage(double alpha) {
    if (m_dataPoints.isEmpty()) {
        return nullptr;
    }
    
    if (alpha < 0.0) {
        alpha = m_alpha;
    }
    
    // Create averaged data point
    WeatherData* averaged = new WeatherData(this);
    
    // Use latest point's location and timestamp as base
    WeatherData* latest = m_dataPoints.last();
    averaged->setLatitude(latest->latitude());
    averaged->setLongitude(latest->longitude());
    averaged->setTimestamp(latest->timestamp());
    
    // Collect all values
    QList<double> temps, feelsLike, pressures, windSpeeds, precipProbs, precipIntensities;
    QList<int> windDirections, humidities, cloudCovers, visibilities, uvIndices;
    QList<double> windSpeedsForDirection;
    
    for (WeatherData* data : m_dataPoints) {
        temps.append(data->temperature());
        feelsLike.append(data->feelsLike());
        pressures.append(data->pressure());
        windSpeeds.append(data->windSpeed());
        precipProbs.append(data->precipProbability());
        precipIntensities.append(data->precipIntensity());
        windDirections.append(data->windDirection());
        humidities.append(data->humidity());
        cloudCovers.append(data->cloudCover());
        visibilities.append(data->visibility());
        uvIndices.append(data->uvIndex());
        
        if (data->windSpeed() > 0) {
            windSpeedsForDirection.append(data->windSpeed());
        }
    }
    
    // Exponential moving averages
    averaged->setTemperature(calculateExponentialAverage(temps, alpha));
    averaged->setFeelsLike(calculateExponentialAverage(feelsLike, alpha));
    averaged->setPressure(calculateExponentialAverage(pressures, alpha));
    averaged->setWindSpeed(calculateExponentialAverage(windSpeeds, alpha));
    averaged->setPrecipProbability(qMax(0.0, qMin(1.0, calculateExponentialAverage(precipProbs, alpha))));
    averaged->setPrecipIntensity(qMax(0.0, calculateExponentialAverage(precipIntensities, alpha)));
    averaged->setHumidity(static_cast<int>(qRound(calculateExponentialAverage(
        QList<double>(humidities.begin(), humidities.end()), alpha))));
    averaged->setCloudCover(static_cast<int>(qRound(calculateExponentialAverage(
        QList<double>(cloudCovers.begin(), cloudCovers.end()), alpha))));
    averaged->setVisibility(static_cast<int>(qRound(calculateExponentialAverage(
        QList<double>(visibilities.begin(), visibilities.end()), alpha))));
    averaged->setUvIndex(static_cast<int>(qRound(calculateExponentialAverage(
        QList<double>(uvIndices.begin(), uvIndices.end()), alpha))));
    
    // Wind direction: use simple average for EMA (vector average would be complex)
    if (!windDirections.isEmpty()) {
        int windWindow = qMin(10, windDirections.size());
        averaged->setWindDirection(windDirectionAverage(windDirections, windSpeeds, windWindow));
    } else {
        averaged->setWindDirection(latest->windDirection());
    }
    
    // Use latest point's weather condition/description
    averaged->setWeatherCondition(latest->weatherCondition());
    averaged->setWeatherDescription(latest->weatherDescription());
    
    return averaged;
}

QList<WeatherData*> MovingAverageFilter::smoothForecast(const QList<WeatherData*>& forecasts,
                                                          const QList<WeatherData*>& historicalData) {
    QList<WeatherData*> smoothed;
    
    if (forecasts.isEmpty()) {
        return smoothed;
    }
    
    // Add historical data to context (if provided)
    QList<WeatherData*> allData;
    allData.append(historicalData);
    allData.append(forecasts);
    
    // Sort by timestamp
    std::sort(allData.begin(), allData.end(),
              [](WeatherData* a, WeatherData* b) {
                  return a->timestamp() < b->timestamp();
              });
    
    // Apply moving average to each forecast point
    for (WeatherData* forecast : forecasts) {
        if (!forecast) {
            continue;
        }
        
        // Find position in sorted list
        int pos = allData.indexOf(forecast);
        if (pos < 0) {
            // Forecast not in list, use as-is
            smoothed.append(forecast);
            continue;
        }
        
        // Get window of data around this point
        int windowSize = m_defaultWindowSize;
        int startPos = qMax(0, pos - windowSize / 2);
        int endPos = qMin(allData.size(), pos + windowSize / 2 + 1);
        QList<WeatherData*> windowData = allData.mid(startPos, endPos - startPos);
        
        if (windowData.isEmpty()) {
            smoothed.append(forecast);
            continue;
        }
        
        // Calculate moving average for this window
        QList<double> temps, feelsLike, pressures, windSpeeds, precipProbs, precipIntensities;
        QList<int> windDirections, humidities, cloudCovers, visibilities, uvIndices;
        
        for (WeatherData* data : windowData) {
            temps.append(data->temperature());
            feelsLike.append(data->feelsLike());
            pressures.append(data->pressure());
            windSpeeds.append(data->windSpeed());
            precipProbs.append(data->precipProbability());
            precipIntensities.append(data->precipIntensity());
            windDirections.append(data->windDirection());
            humidities.append(data->humidity());
            cloudCovers.append(data->cloudCover());
            visibilities.append(data->visibility());
            uvIndices.append(data->uvIndex());
        }
        
        // Create smoothed data point
        WeatherData* smoothedData = new WeatherData();
        smoothedData->setLatitude(forecast->latitude());
        smoothedData->setLongitude(forecast->longitude());
        smoothedData->setTimestamp(forecast->timestamp());
        
        if (m_type == Exponential) {
            // Use exponential moving average
            smoothedData->setTemperature(calculateExponentialAverage(temps, m_alpha));
            smoothedData->setFeelsLike(calculateExponentialAverage(feelsLike, m_alpha));
            smoothedData->setPressure(calculateExponentialAverage(pressures, m_alpha));
            smoothedData->setWindSpeed(calculateExponentialAverage(windSpeeds, m_alpha));
            smoothedData->setPrecipProbability(qMax(0.0, qMin(1.0, calculateExponentialAverage(precipProbs, m_alpha))));
            smoothedData->setPrecipIntensity(qMax(0.0, calculateExponentialAverage(precipIntensities, m_alpha)));
        } else {
            // Use simple moving average
            smoothedData->setTemperature(calculateSimpleAverage(temps, windowData.size()));
            smoothedData->setFeelsLike(calculateSimpleAverage(feelsLike, windowData.size()));
            smoothedData->setPressure(calculateSimpleAverage(pressures, windowData.size()));
            smoothedData->setWindSpeed(calculateSimpleAverage(windSpeeds, windowData.size()));
            smoothedData->setPrecipProbability(qMax(0.0, qMin(1.0, calculateSimpleAverage(precipProbs, windowData.size()))));
            smoothedData->setPrecipIntensity(qMax(0.0, calculateSimpleAverage(precipIntensities, windowData.size())));
        }
        
        smoothedData->setWindDirection(windDirectionAverage(windDirections, windSpeeds, windowData.size()));
        smoothedData->setHumidity(static_cast<int>(qRound(calculateSimpleAverage(
            QList<double>(humidities.begin(), humidities.end()), windowData.size()))));
        smoothedData->setCloudCover(static_cast<int>(qRound(calculateSimpleAverage(
            QList<double>(cloudCovers.begin(), cloudCovers.end()), windowData.size()))));
        smoothedData->setVisibility(static_cast<int>(qRound(calculateSimpleAverage(
            QList<double>(visibilities.begin(), visibilities.end()), windowData.size()))));
        smoothedData->setUvIndex(static_cast<int>(qRound(calculateSimpleAverage(
            QList<double>(uvIndices.begin(), uvIndices.end()), windowData.size()))));
        
        smoothedData->setWeatherCondition(forecast->weatherCondition());
        smoothedData->setWeatherDescription(forecast->weatherDescription());
        
        smoothed.append(smoothedData);
    }
    
    return smoothed;
}

void MovingAverageFilter::clear() {
    for (WeatherData* data : m_dataPoints) {
        delete data;
    }
    m_dataPoints.clear();
}

double MovingAverageFilter::calculateSimpleAverage(const QList<double>& values, int windowSize) const {
    if (values.isEmpty() || windowSize <= 0) {
        return 0.0;
    }
    
    int actualSize = qMin(windowSize, values.size());
    if (actualSize == 0) {
        return 0.0;
    }
    
    double sum = 0.0;
    int count = 0;
    
    // Use last N values
    int startIdx = qMax(0, values.size() - actualSize);
    for (int i = startIdx; i < values.size(); ++i) {
        if (qIsFinite(values[i])) {
            sum += values[i];
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}

double MovingAverageFilter::calculateExponentialAverage(const QList<double>& values, double alpha) const {
    if (values.isEmpty() || alpha < 0.0 || alpha > 1.0) {
        return values.isEmpty() ? 0.0 : values.last();
    }
    
    // EMA: EMA_today = alpha * value_today + (1 - alpha) * EMA_yesterday
    double ema = values.first();
    
    for (int i = 1; i < values.size(); ++i) {
        if (qIsFinite(values[i])) {
            ema = alpha * values[i] + (1.0 - alpha) * ema;
        }
    }
    
    return ema;
}

int MovingAverageFilter::windDirectionAverage(const QList<int>& directions, const QList<double>& speeds, int windowSize) const {
    if (directions.isEmpty() || windowSize <= 0) {
        return directions.isEmpty() ? 0 : directions.last();
    }
    
    int actualSize = qMin(windowSize, directions.size());
    if (actualSize == 0) {
        return 0;
    }
    
    // Vector average for wind direction
    double windX = 0.0;
    double windY = 0.0;
    int count = 0;
    
    int startIdx = qMax(0, directions.size() - actualSize);
    for (int i = startIdx; i < directions.size(); ++i) {
        if (i < speeds.size() && speeds[i] > 0 && directions[i] >= 0) {
            double radians = qDegreesToRadians(static_cast<double>(directions[i]));
            windX += qCos(radians) * speeds[i];
            windY += qSin(radians) * speeds[i];
            count++;
        }
    }
    
    if (count == 0 || (qAbs(windX) < 0.001 && qAbs(windY) < 0.001)) {
        return directions.last();
    }
    
    double avgDirectionRadians = qAtan2(windY, windX);
    int avgDirection = static_cast<int>(qRadiansToDegrees(avgDirectionRadians));
    if (avgDirection < 0) {
        avgDirection += 360;
    }
    
    return avgDirection;
}

