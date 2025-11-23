#ifndef SPATIOTEMPORALENGINE_H
#define SPATIOTEMPORALENGINE_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QPointF>
#include "models/WeatherData.h"
#include "nowcast/SpatialInterpolator.h"
#include "nowcast/TemporalInterpolator.h"

class WeatherService;

/**
 * @brief Orchestrates spatio-temporal forecasting across multiple APIs and grid points
 * 
 * This engine coordinates:
 * 1. Generating a 5-point grid (center + N/S/E/W offsets)
 * 2. Querying multiple weather APIs at each grid point
 * 3. Spatially smoothing each API's forecast across the grid
 * 4. Temporally interpolating each API's forecast to common granularity
 * 5. Combining multiple APIs' forecasts into a single weighted result
 */
class SpatioTemporalEngine : public QObject
{
    Q_OBJECT
    
public:
    struct GridConfig {
        double offsetDistanceKm = 1.0;      // Distance to offset points in km
        int pointCount = 7;                  // Center + N/S/E/W + Up/Down
    };
    
    struct TemporalConfig {
        int outputGranularityMinutes = 1;    // Output forecast interval
        TemporalInterpolator::InterpolationMethod method = TemporalInterpolator::Linear;
        int smoothingWindowMinutes = 30;     // Post-interpolation smoothing
    };
    
    struct SpatialConfig {
        SpatialInterpolator::InterpolationStrategy strategy = SpatialInterpolator::InverseDistanceWeighting;
        double idwPower = 2.0;               // Power parameter for IDW
        int missingPointsThreshold = 2;      // Max missing points before fallback
    };
    
    struct APIWeights {
        QMap<QString, double> weights;        // API name -> weight (0.0-1.0)
    };
    
    explicit SpatioTemporalEngine(QObject *parent = nullptr);
    ~SpatioTemporalEngine() override;
    
    // Configuration
    void setGridConfig(const GridConfig& config) { m_gridConfig = config; }
    void setTemporalConfig(const TemporalConfig& config) { m_temporalConfig = config; }
    void setSpatialConfig(const SpatialConfig& config) { m_spatialConfig = config; }
    void setAPIWeights(const APIWeights& weights) { m_apiWeights = weights; }
    
    GridConfig gridConfig() const { return m_gridConfig; }
    TemporalConfig temporalConfig() const { return m_temporalConfig; }
    SpatialConfig spatialConfig() const { return m_spatialConfig; }
    APIWeights apiWeights() const { return m_apiWeights; }
    
    /**
     * @brief Generate multi-point grid around center location
     * @param centerLat Center latitude
     * @param centerLon Center longitude
     * @return List of (lat, lon) points: [center, N, S, E, W, up, down]
     */
    QList<QPointF> generateGrid(double centerLat, double centerLon);
    
    /**
     * @brief Apply spatial smoothing to forecast data at a single timestamp
     * @param gridForecasts Forecasts from all grid points at one timestamp
     * @param centerLat Center latitude for interpolation
     * @param centerLon Center longitude for interpolation
     * @return Spatially smoothed value at center point
     */
    WeatherData* applySpatialSmoothing(const QList<WeatherData*>& gridForecasts,
                                       double centerLat, double centerLon);
    
    /**
     * @brief Apply temporal interpolation to forecast timeline
     * @param apiForecasts Raw forecast data from one API
     * @return Interpolated forecast at configured granularity
     */
    QList<WeatherData*> applyTemporalInterpolation(const QList<WeatherData*>& apiForecasts);
    
    /**
     * @brief Combine forecasts from multiple APIs using weighted average
     * @param apiForecasts Map of API name -> forecast list
     * @return Combined forecast
     */
    QList<WeatherData*> combineAPIForecasts(const QMap<QString, QList<WeatherData*>>& apiForecasts);
    
signals:
    void gridGenerated(QList<QPointF> gridPoints);
    void spatialSmoothingComplete();
    void temporalInterpolationComplete();
    void forecastReady(QList<WeatherData*> forecast);
    void  error(QString message);
    
private:
    GridConfig m_gridConfig;
    TemporalConfig m_temporalConfig;
    SpatialConfig m_spatialConfig;
    APIWeights m_apiWeights;
    
    SpatialInterpolator* m_spatialInterpolator;
    TemporalInterpolator* m_temporalInterpolator;
    
    /**
     * @brief Convert distance offset to lat/lon degrees
     * @param distanceKm Distance in kilometers
     * @return Approximate degrees (latitude; longitude varies by latitude)
     */
    double kmToLatDegrees(double distanceKm);
    double kmToLonDegrees(double distanceKm, double latitude);
};

#endif // SPATIOTEMPORALENGINE_H
