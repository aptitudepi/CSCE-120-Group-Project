#include "SpatialInterpolator.h"
#include <cmath>
#include <limits>

SpatialInterpolator::SpatialInterpolator()
    : m_strategy(InverseDistanceWeighting)
{
}

void SpatialInterpolator::setStrategy(InterpolationStrategy strategy) {
    m_strategy = strategy;
}

SpatialInterpolator::InterpolationStrategy SpatialInterpolator::strategy() const {
    return m_strategy;
}

double SpatialInterpolator::interpolate(double targetLat, double targetLon, 
                                      const QList<GridPoint>& points, 
                                      double power) const {
    return interpolateWeighted(targetLat, targetLon, points, m_strategy, power);
}

double SpatialInterpolator::interpolateWeighted(double targetLat, double targetLon,
                                               const QList<GridPoint>& points,
                                               InterpolationStrategy strategy,
                                               double param) const {
    if (points.isEmpty()) {
        return 0.0;
    }
    
    // Filter out invalid points
    QList<GridPoint> validPoints;
    for (const GridPoint& p : points) {
        if (p.isValid) {
            validPoints.append(p);
        }
    }
    
    if (validPoints.isEmpty()) {
        return 0.0;
    }
    
    if (validPoints.size() == 1) {
        return validPoints.first().value;
    }
    
    if (strategy == NearestNeighbor) {
        double minDist = std::numeric_limits<double>::max();
        double nearestValue = 0.0;
        
        for (const GridPoint& p : validPoints) {
            double dist = calculateDistance(targetLat, targetLon, p.latitude, p.longitude);
            if (dist < minDist) {
                minDist = dist;
                nearestValue = p.value;
            }
        }
        return nearestValue;
    }
    
    if (strategy == EqualWeight) {
        // Simple average
        double sum = 0.0;
        for (const GridPoint& p : validPoints) {
            sum += p.value;
        }
        return sum / validPoints.size();
    }
    
    if (strategy == GaussianKernel) {
        // Gaussian-weighted average
        double sigma = param; // param is sigma for Gaussian
        double sumWeights = 0.0;
        double sumWeightedValues = 0.0;
        
        for (const GridPoint& p : validPoints) {
            double dist = calculateDistance(targetLat, targetLon, p.latitude, p.longitude);
            
            if (dist < 0.0001) {
                return p.value; // Exact match
            }
            
            double weight = gaussianWeight(dist, sigma);
            sumWeights += weight;
            sumWeightedValues += weight * p.value;
        }
        
        return (sumWeights > 0.0) ? (sumWeightedValues / sumWeights) : 0.0;
    }
    
    // InverseDistanceWeighting (default)
    double power = param;
    double sumWeights = 0.0;
    double sumWeightedValues = 0.0;
    
    for (const GridPoint& p : validPoints) {
        double dist = calculateDistance(targetLat, targetLon, p.latitude, p.longitude);
        
        // If distance is very small, return the exact value to avoid division by zero
        if (dist < 0.0001) {
            return p.value;
        }
        
        double weight = 1.0 / std::pow(dist, power);
        sumWeights += weight;
        sumWeightedValues += weight * p.value;
    }
    
    if (sumWeights > 0.0) {
        return sumWeightedValues / sumWeights;
    }
    
    return 0.0;
}

QList<GridPoint> SpatialInterpolator::handleMissingPoints(const QList<GridPoint>& points,
                                                          int missingPointThreshold) const {
    int validCount = 0;
    GridPoint centerPoint(0, 0, 0, false);
    
    // Count valid points and find center point (first point)
    for (const GridPoint& p : points) {
        if (p.isValid) {
            validCount++;
            if (centerPoint.isValid == false) {
                centerPoint = p; // Assume first valid point is center
            }
        }
    }
    
    int missingCount = points.size() - validCount;
    
    if (missingCount > missingPointThreshold) {
        // Too many missing, fall back to center point only
        if (centerPoint.isValid) {
            return QList<GridPoint>() << centerPoint;
        }
        return QList<GridPoint>();
    }
    
    // Return only valid points (renormalized weights)
    QList<GridPoint> validPoints;
    for (const GridPoint& p : points) {
        if (p.isValid) {
            validPoints.append(p);
        }
    }
    
    return validPoints;
}

double SpatialInterpolator::calculateDistance(double lat1, double lon1, double lat2, double lon2) const {
    // Haversine formula for distance
    const double R = 6371.0; // Earth radius in km
    const double dLat = qDegreesToRadians(lat2 - lat1);
    const double dLon = qDegreesToRadians(lon2 - lon1);
    
    const double a = qSin(dLat / 2) * qSin(dLat / 2) +
                     qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2)) *
                     qSin(dLon / 2) * qSin(dLon / 2);
                     
    const double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    
    return R * c;
}

double SpatialInterpolator::gaussianWeight(double distance, double sigma) const {
    // Gaussian function: exp(-(distance^2) / (2 * sigma^2))
    return qExp(-(distance * distance) / (2.0 * sigma * sigma));
}
