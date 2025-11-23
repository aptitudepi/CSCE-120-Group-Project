#ifndef SPATIAL_INTERPOLATOR_H
#define SPATIAL_INTERPOLATOR_H

#include <QList>
#include <QtMath>

struct GridPoint {
    double latitude;
    double longitude;
    double value;
    bool isValid; // For handling missing data
    
    GridPoint(double lat, double lon, double val, bool valid = true) 
        : latitude(lat), longitude(lon), value(val), isValid(valid) {}
};

class SpatialInterpolator {
public:
    enum InterpolationStrategy {
        InverseDistanceWeighting,  // Weighted by 1/distance^power
        NearestNeighbor,           // Use closest point
        EqualWeight,               // Simple average of all points
        GaussianKernel             // Gaussian distance weighting
    };

    SpatialInterpolator();
    
    /**
     * @brief Interpolate a value at a specific location using known grid points
     * @param targetLat Target latitude
     * @param targetLon Target longitude
     * @param points List of known grid points with values
     * @param power Power parameter for IDW (default 2.0)
     * @return Interpolated value
     */
    double interpolate(double targetLat, double targetLon, 
                      const QList<GridPoint>& points, 
                      double power = 2.0) const;

    /**
     * @brief Interpolate with explicit strategy and parameters
     * @param targetLat Target latitude
     * @param targetLon Target longitude
     * @param points List of grid points (may include invalid/missing)
     * @param strategy Interpolation strategy to use
     * @param param Strategy-specific parameter (e.g., IDW power, Gaussian sigma)
     * @return Interpolated value
     */
    double interpolateWeighted(double targetLat, double targetLon,
                              const QList<GridPoint>& points,
                              InterpolationStrategy strategy,
                              double param = 2.0) const;

    /**
     * @brief Handle missing grid points gracefully
     * @param points All grid points (may include invalid)
     * @param missingPointThreshold Max number of missing points before fallback
     * @return Filtered list with only valid points, or center point if too many missing
     */
    QList<GridPoint> handleMissingPoints(const QList<GridPoint>& points,
                                         int missingPointThreshold = 2) const;

    void setStrategy(InterpolationStrategy strategy);
    InterpolationStrategy strategy() const;
    
    // Legacy compatibility
    void setMethod(InterpolationStrategy method) { setStrategy(method); }
    InterpolationStrategy method() const { return strategy(); }

private:
    InterpolationStrategy m_strategy;
    
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) const;
    double gaussianWeight(double distance, double sigma) const;
};

#endif // SPATIAL_INTERPOLATOR_H
