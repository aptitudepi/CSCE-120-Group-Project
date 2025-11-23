#ifndef TEMPORALINTERPOLATOR_H
#define TEMPORALINTERPOLATOR_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include "models/WeatherData.h"

/**
 * @brief Handles temporal interpolation and smoothing of weather forecasts
 * 
 * Supports linear interpolation between forecast time steps and optional
 * moving average smoothing. Designed to work with variable time-step APIs
 * (e.g., hourly, 15-minute) and produce consistent output granularity.
 */
class TemporalInterpolator : public QObject
{
    Q_OBJECT
    
public:
    enum InterpolationMethod {
        Linear,        // Simple linear interpolation
        CubicSpline,   // Smooth cubic spline (future)
        StepFunction   // Nearest neighbor (no interpolation)
    };
    
    enum SmoothingMethod {
        None,
        SimpleMovingAverage,
        ExponentialMovingAverage
    };
    
    explicit TemporalInterpolator(QObject *parent = nullptr);
    ~TemporalInterpolator() override;
    
    /**
     * @brief Interpolate forecast data to a specified time granularity
     * @param sourceData Original forecast data with varying time steps
     * @param outputGranularityMinutes Desired output interval in minutes (e.g., 10)
     * @param method Interpolation method to use
     * @return Interpolated forecast data at regular intervals
     */
    QList<WeatherData*> interpolate(const QList<WeatherData*>& sourceData,
                                     int outputGranularityMinutes,
                                     InterpolationMethod method = Linear);
    
    /**
     * @brief Apply moving average smoothing to forecast data
     * @param data Forecast data to smooth
     * @param windowMinutes Size of smoothing window in minutes
     * @param method Smoothing method to use
     * @return Smoothed forecast data
     */
    QList<WeatherData*> smooth(const QList<WeatherData*>& data,
                               int windowMinutes,
                               SmoothingMethod method = SimpleMovingAverage);
    
    /**
     * @brief Align forecast data to a common time grid
     * @param data Forecast data to align
     * @param gridStartTime Start time of the grid
     * @param gridIntervalMinutes Interval between grid points
     * @param gridPointCount Number of grid points
     * @return Data aligned to grid (interpolated if necessary)
     */
    QList<WeatherData*> alignToGrid(const QList<WeatherData*>& data,
                                     const QDateTime& gridStartTime,
                                     int gridIntervalMinutes,
                                     int gridPointCount);
    
    // Configuration
    void setInterpolationMethod(InterpolationMethod method) { m_interpolationMethod = method; }
    void setSmoothingMethod(SmoothingMethod method) { m_smoothingMethod = method; }
    void setSmoothingWindow(int minutes) { m_smoothingWindowMinutes = minutes; }
    
    InterpolationMethod interpolationMethod() const { return m_interpolationMethod; }
    SmoothingMethod smoothingMethod() const { return m_smoothingMethod; }
    int smoothingWindow() const { return m_smoothingWindowMinutes; }
    
private:
    /**
     * @brief Linear interpolation between two weather data points
     */
    WeatherData* linearInterpolate(const WeatherData* before,
                                    const WeatherData* after,
                                    const QDateTime& targetTime);
    
    /**
     * @brief Interpolate a single numeric value
     */
    double interpolateValue(double valueBefore, double valueAfter,
                           qint64 timeBefore, qint64 timeAfter,
                           qint64 targetTime);
    
    /**
     * @brief Find the two data points bracketing a target time
     */
    bool findBracketingPoints(const QList<WeatherData*>& data,
                             const QDateTime& targetTime,
                             const WeatherData*& before,
                             const WeatherData*& after);
    
    InterpolationMethod m_interpolationMethod;
    SmoothingMethod m_smoothingMethod;
    int m_smoothingWindowMinutes;
};

#endif // TEMPORALINTERPOLATOR_H
