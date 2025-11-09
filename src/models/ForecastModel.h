#ifndef FORECASTMODEL_H
#define FORECASTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "models/WeatherData.h"

/**
 * @brief QAbstractListModel for managing forecast data in QML ListView
 * 
 * This model provides a standard Qt model interface for displaying
 * forecast data in QML ListViews with proper role-based access.
 */
class ForecastModel : public QAbstractListModel
{
    Q_OBJECT
    
public:
    enum ForecastRoles {
        LatitudeRole = Qt::UserRole + 1,
        LongitudeRole,
        TimestampRole,
        TemperatureRole,
        FeelsLikeRole,
        HumidityRole,
        PressureRole,
        WindSpeedRole,
        WindDirectionRole,
        PrecipProbabilityRole,
        PrecipIntensityRole,
        CloudCoverRole,
        VisibilityRole,
        UvIndexRole,
        WeatherConditionRole,
        WeatherDescriptionRole
    };
    
    explicit ForecastModel(QObject *parent = nullptr);
    ~ForecastModel() override;
    
    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    // Model manipulation
    void addForecast(WeatherData* forecast);
    void addForecasts(const QList<WeatherData*>& forecasts);
    void clear();
    WeatherData* get(int index) const;
    QList<WeatherData*> getAll() const;
    
    // Update existing forecast
    void updateForecast(int index, WeatherData* forecast);
    
signals:
    void forecastAdded(WeatherData* forecast);
    void forecastRemoved(int index);
    
private:
    QList<WeatherData*> m_forecasts;
};

#endif // FORECASTMODEL_H

