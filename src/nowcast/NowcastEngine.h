#ifndef NOWCASTENGINE_H
#define NOWCASTENGINE_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include "models/WeatherData.h"

/**
 * @brief Precipitation nowcasting engine
 * 
 * Provides short-term (0-90 minute) precipitation forecasts using
 * radar extrapolation techniques. Phase 1 MVP uses simple persistence
 * and interpolation. Future phases will add optical flow and blending.
 */
class NowcastEngine : public QObject
{
    Q_OBJECT
    
public:
    explicit NowcastEngine(QObject *parent = nullptr);
    ~NowcastEngine() override;
    
    /**
     * @brief Generate nowcast for a location
     * @param latitude Location latitude
     * @param longitude Location longitude
     * @param currentData Current weather data
     * @param historicalData Historical weather data for motion estimation
     * @return List of nowcast data points (minute-by-minute)
     */
    QList<WeatherData*> generateNowcast(double latitude, double longitude,
                                       WeatherData* currentData,
                                       const QList<WeatherData*>& historicalData);
    
    /**
     * @brief Get precipitation start time prediction
     * @param nowcast Nowcast data
     * @return Predicted start time, or invalid QDateTime if no precipitation expected
     */
    QDateTime predictPrecipitationStart(const QList<WeatherData*>& nowcast);
    
    /**
     * @brief Get precipitation end time prediction
     * @param nowcast Nowcast data
     * @return Predicted end time, or invalid QDateTime if no end predicted
     */
    QDateTime predictPrecipitationEnd(const QList<WeatherData*>& nowcast);
    
    /**
     * @brief Get confidence score for nowcast (0.0 to 1.0)
     */
    double getConfidence(const QList<WeatherData*>& nowcast) const;
    
signals:
    void nowcastReady(QList<WeatherData*> nowcast);
    void error(QString message);
    
private:
    /**
     * @brief Simple persistence-based nowcast (Phase 1 MVP)
     */
    QList<WeatherData*> generatePersistenceNowcast(double latitude, double longitude,
                                                   WeatherData* current,
                                                   int minutes = 90);
    
    /**
     * @brief Estimate motion from historical data (future: optical flow)
     */
    void estimateMotion(const QList<WeatherData*>& historical, double& motionX, double& motionY);
    
    /**
     * @brief Interpolate precipitation intensity
     */
    double interpolatePrecipitation(double current, double target, double factor);
    
    double m_confidence;
};

#endif // NOWCASTENGINE_H

