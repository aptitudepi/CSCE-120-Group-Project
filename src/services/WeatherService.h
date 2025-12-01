#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QObject>
#include <QString>
#include "models/WeatherData.h"

/**
 * @brief Abstract base class for weather data services
 * 
 * This class defines the interface that all weather service implementations
 * must follow, enabling easy swapping of data sources.
 */
class WeatherService : public QObject
{
    Q_OBJECT
    
public:
    explicit WeatherService(QObject *parent = nullptr);
    virtual ~WeatherService() = default;
    
    /**
     * @brief Fetch forecast for a specific location
     * @param latitude Location latitude
     * @param longitude Location longitude
     */
    virtual void fetchForecast(double latitude, double longitude) = 0;
    
    /**
     * @brief Fetch current weather for a specific location
     * @param latitude Location latitude
     * @param longitude Location longitude
     */
    virtual void fetchCurrent(double latitude, double longitude) = 0;
    
    /**
     * @brief Get service name
     */
    virtual QString serviceName() const = 0;
    
    /**
     * @brief Check if service is available
     */
    virtual bool isAvailable() const { return true; }
    
    /**
     * @brief Cancel any in-flight network requests.
     * 
     * Used when the caller issues a new fetch before the previous request
     * completes.
     */
    virtual void cancelActiveRequests() {}
    
signals:
    /**
     * @brief Emitted when forecast data is ready
     */
    void forecastReady(QList<WeatherData*> data);
    
    /**
     * @brief Emitted when current weather data is ready
     */
    void currentReady(WeatherData* data);
    
    /**
     * @brief Emitted when an error occurs
     */
    void error(QString message);
    
protected:
    QString m_lastError;
};

#endif // WEATHERSERVICE_H

