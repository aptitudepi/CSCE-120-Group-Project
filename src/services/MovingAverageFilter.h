#ifndef MOVINGAVERAGEFILTER_H
#define MOVINGAVERAGEFILTER_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include "models/WeatherData.h"

/**
 * @brief Temporal moving average filter for weather data smoothing
 * 
 * Applies moving average smoothing to time-series weather data to reduce
 * noise and improve forecast quality. Supports both simple and exponential
 * moving averages with configurable window sizes per parameter type.
 */
class MovingAverageFilter : public QObject
{
    Q_OBJECT
    
public:
    enum MovingAverageType {
        Simple,      // Simple moving average (equal weights)
        Exponential  // Exponential moving average (decaying weights)
    };
    Q_ENUM(MovingAverageType)
    
    explicit MovingAverageFilter(QObject *parent = nullptr);
    ~MovingAverageFilter() override;
    
    /**
     * @brief Set moving average type
     */
    void setType(MovingAverageType type);
    
    /**
     * @brief Set window size for simple moving average
     * @param windowSize Number of data points to average
     */
    void setWindowSize(int windowSize);
    
    /**
     * @brief Set window size for specific parameter type
     * @param parameter Parameter name ("temperature", "precipitation", etc.)
     * @param windowSize Window size for that parameter
     */
    void setWindowSize(const QString& parameter, int windowSize);
    
    /**
     * @brief Set alpha (smoothing factor) for exponential moving average
     * @param alpha Smoothing factor (0.0 to 1.0, higher = more responsive)
     */
    void setAlpha(double alpha);
    
    /**
     * @brief Add a data point to the time series
     * @param data Weather data point to add
     */
    void addDataPoint(WeatherData* data);
    
    /**
     * @brief Get moving average over last N points
     * @param windowSize Window size (overrides configured size)
     * @return Smoothed weather data point
     */
    WeatherData* getMovingAverage(int windowSize = -1);
    
    /**
     * @brief Get exponential moving average
     * @param alpha Smoothing factor (overrides configured alpha)
     * @return Smoothed weather data point
     */
    WeatherData* getExponentialMovingAverage(double alpha = -1.0);
    
    /**
     * @brief Apply moving average to a list of forecasts
     * @param forecasts List of weather data points to smooth
     * @param historicalData Historical data to use for smoothing (optional)
     * @return List of smoothed weather data points
     */
    QList<WeatherData*> smoothForecast(const QList<WeatherData*>& forecasts,
                                       const QList<WeatherData*>& historicalData = QList<WeatherData*>());
    
    /**
     * @brief Clear all stored data points
     */
    void clear();
    
    /**
     * @brief Get number of stored data points
     */
    int dataPointCount() const { return m_dataPoints.size(); }
    
signals:
    void error(QString message);
    
private:
    double calculateSimpleAverage(const QList<double>& values, int windowSize) const;
    double calculateExponentialAverage(const QList<double>& values, double alpha) const;
    int windDirectionAverage(const QList<int>& directions, const QList<double>& speeds, int windowSize) const;
    
    QList<WeatherData*> m_dataPoints;
    MovingAverageType m_type;
    int m_defaultWindowSize;
    double m_alpha;
    QMap<QString, int> m_parameterWindowSizes;
};

#endif // MOVINGAVERAGEFILTER_H

