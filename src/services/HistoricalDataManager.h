#ifndef HISTORICALDATAMANAGER_H
#define HISTORICALDATAMANAGER_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QString>
#include "models/WeatherData.h"

/**
 * @brief Manages historical weather data storage for time-series analysis
 * 
 * Stores weather forecasts and observations from multiple sources in SQLite
 * database to enable moving average calculations and historical analysis.
 */
class HistoricalDataManager : public QObject
{
    Q_OBJECT
    
public:
    explicit HistoricalDataManager(QObject *parent = nullptr);
    ~HistoricalDataManager() override;
    
    /**
     * @brief Initialize the database schema
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Store a single weather data point
     * @param latitude Location latitude
     * @param longitude Location longitude
     * @param data Weather data to store
     * @param source Source identifier ("nws", "pirate", "merged", etc.)
     * @return true if successful
     */
    bool storeForecast(double latitude, double longitude, WeatherData* data, const QString& source);
    
    /**
     * @brief Store multiple weather forecasts
     * @param latitude Location latitude
     * @param longitude Location longitude
     * @param forecasts List of weather data to store
     * @param source Source identifier
     * @return true if successful
     */
    bool storeForecasts(double latitude, double longitude, const QList<WeatherData*>& forecasts, const QString& source);
    
    /**
     * @brief Retrieve historical data for a location and time range
     * @param latitude Location latitude
     * @param longitude Location longitude
     * @param startTime Start time (inclusive)
     * @param endTime End time (inclusive)
     * @param source Source filter (empty string for all sources)
     * @return List of weather data points
     */
    QList<WeatherData*> getHistoricalData(double latitude, double longitude,
                                          const QDateTime& startTime,
                                          const QDateTime& endTime,
                                          const QString& source = QString());
    
    /**
     * @brief Get recent historical data for a location
     * @param latitude Location latitude
     * @param longitude Location longitude
     * @param hours Number of hours back to retrieve
     * @param source Source filter (empty string for all sources)
     * @return List of weather data points, sorted by timestamp
     */
    QList<WeatherData*> getRecentData(double latitude, double longitude,
                                      int hours,
                                      const QString& source = QString());
    
    /**
     * @brief Clean up old data
     * @param daysToKeep Number of days of data to keep
     * @return Number of records deleted
     */
    int cleanupOldData(int daysToKeep = 7);
    
    /**
     * @brief Get data retention period in days
     */
    int retentionDays() const { return m_retentionDays; }
    
    /**
     * @brief Set data retention period in days
     */
    void setRetentionDays(int days) { m_retentionDays = days; }
    
signals:
    void dataStored(int count);
    void cleanupComplete(int deletedCount);
    void error(QString message);
    
private:
    bool createTableIfNotExists();
    QString generateLocationKey(double latitude, double longitude, double precision = 0.0001) const;
    int m_retentionDays;
};

#endif // HISTORICALDATAMANAGER_H

