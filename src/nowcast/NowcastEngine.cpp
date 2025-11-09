#include "nowcast/NowcastEngine.h"
#include "models/WeatherData.h"
#include <QDebug>
#include <QDateTime>
#include <QtMath>
#include <QList>

NowcastEngine::NowcastEngine(QObject *parent)
    : QObject(parent)
    , m_confidence(0.75) // Default confidence for MVP
{
}

NowcastEngine::~NowcastEngine() = default;

QList<WeatherData*> NowcastEngine::generateNowcast(double latitude, double longitude,
                                                   WeatherData* currentData,
                                                   const QList<WeatherData*>& historicalData) {
    if (!currentData) {
        emit error("No current data provided");
        return QList<WeatherData*>();
    }
    
    // Phase 1 MVP: Use simple persistence
    // Future phases will use motion estimation and blending
    return generatePersistenceNowcast(latitude, longitude, currentData, 90);
}

QList<WeatherData*> NowcastEngine::generatePersistenceNowcast(double latitude, double longitude,
                                                              WeatherData* current, int minutes) {
    QList<WeatherData*> nowcast;
    
    if (!current) {
        return nowcast;
    }
    
    QDateTime baseTime = current->timestamp();
    if (!baseTime.isValid()) {
        baseTime = QDateTime::currentDateTime();
    }
    
    // Generate minute-by-minute forecast
    for (int i = 1; i <= minutes; ++i) {
        WeatherData* forecast = new WeatherData();
        forecast->setLatitude(latitude);
        forecast->setLongitude(longitude);
        forecast->setTimestamp(baseTime.addSecs(i * 60));
        
        // Persist current conditions with slight decay
        double decayFactor = 1.0 - (i / (minutes * 2.0)); // Gradual decay
        
        // Temperature: slight decrease over time (simplified)
        forecast->setTemperature(current->temperature() - (i * 0.1 * decayFactor));
        forecast->setFeelsLike(current->feelsLike() - (i * 0.1 * decayFactor));
        
        // Precipitation: persist with decay
        double precipProb = current->precipProbability() * decayFactor;
        forecast->setPrecipProbability(qMax(0.0, precipProb));
        
        double precipIntensity = current->precipIntensity() * decayFactor;
        forecast->setPrecipIntensity(qMax(0.0, precipIntensity));
        
        // Other parameters: persist with minor variations
        forecast->setHumidity(current->humidity());
        forecast->setPressure(current->pressure());
        forecast->setWindSpeed(current->windSpeed());
        forecast->setWindDirection(current->windDirection());
        forecast->setCloudCover(current->cloudCover());
        forecast->setVisibility(current->visibility());
        forecast->setUvIndex(current->uvIndex());
        forecast->setWeatherCondition(current->weatherCondition());
        forecast->setWeatherDescription(current->weatherDescription());
        
        nowcast.append(forecast);
    }
    
    return nowcast;
}

QDateTime NowcastEngine::predictPrecipitationStart(const QList<WeatherData*>& nowcast) {
    for (WeatherData* data : nowcast) {
        if (data->precipProbability() > 0.3 || data->precipIntensity() > 0.1) {
            return data->timestamp();
        }
    }
    return QDateTime(); // No precipitation predicted
}

QDateTime NowcastEngine::predictPrecipitationEnd(const QList<WeatherData*>& nowcast) {
    QDateTime lastPrecip;
    bool inPrecip = false;
    
    for (WeatherData* data : nowcast) {
        bool hasPrecip = data->precipProbability() > 0.3 || data->precipIntensity() > 0.1;
        
        if (hasPrecip) {
            inPrecip = true;
            lastPrecip = data->timestamp();
        } else if (inPrecip) {
            // Precipitation ended
            return data->timestamp();
        }
    }
    
    // If still in precipitation at end of forecast
    if (inPrecip) {
        return lastPrecip;
    }
    
    return QDateTime(); // No precipitation end predicted
}

double NowcastEngine::getConfidence(const QList<WeatherData*>& nowcast) const {
    Q_UNUSED(nowcast)
    // Phase 1: Fixed confidence
    // Future: Calculate based on data quality, historical accuracy, etc.
    return m_confidence;
}

void NowcastEngine::estimateMotion(const QList<WeatherData*>& historical, double& motionX, double& motionY) {
    Q_UNUSED(historical)
    // Phase 1: No motion estimation
    // Future: Implement optical flow or block matching
    motionX = 0.0;
    motionY = 0.0;
}

double NowcastEngine::interpolatePrecipitation(double current, double target, double factor) {
    return current + (target - current) * factor;
}

