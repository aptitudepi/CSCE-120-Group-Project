#include "models/ForecastModel.h"
#include <QDebug>

ForecastModel::ForecastModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ForecastModel::~ForecastModel() {
    qDeleteAll(m_forecasts);
    m_forecasts.clear();
}

int ForecastModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_forecasts.size();
}

QVariant ForecastModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_forecasts.size()) {
        return QVariant();
    }
    
    WeatherData* forecast = m_forecasts.at(index.row());
    if (!forecast) {
        return QVariant();
    }
    
    switch (role) {
        case LatitudeRole:
            return forecast->latitude();
        case LongitudeRole:
            return forecast->longitude();
        case TimestampRole:
            return forecast->timestamp();
        case TemperatureRole:
            return forecast->temperature();
        case FeelsLikeRole:
            return forecast->feelsLike();
        case HumidityRole:
            return forecast->humidity();
        case PressureRole:
            return forecast->pressure();
        case WindSpeedRole:
            return forecast->windSpeed();
        case WindDirectionRole:
            return forecast->windDirection();
        case PrecipProbabilityRole:
            return forecast->precipProbability();
        case PrecipIntensityRole:
            return forecast->precipIntensity();
        case CloudCoverRole:
            return forecast->cloudCover();
        case VisibilityRole:
            return forecast->visibility();
        case UvIndexRole:
            return forecast->uvIndex();
        case WeatherConditionRole:
            return forecast->weatherCondition();
        case WeatherDescriptionRole:
            return forecast->weatherDescription();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ForecastModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[LatitudeRole] = "latitude";
    roles[LongitudeRole] = "longitude";
    roles[TimestampRole] = "timestamp";
    roles[TemperatureRole] = "temperature";
    roles[FeelsLikeRole] = "feelsLike";
    roles[HumidityRole] = "humidity";
    roles[PressureRole] = "pressure";
    roles[WindSpeedRole] = "windSpeed";
    roles[WindDirectionRole] = "windDirection";
    roles[PrecipProbabilityRole] = "precipProbability";
    roles[PrecipIntensityRole] = "precipIntensity";
    roles[CloudCoverRole] = "cloudCover";
    roles[VisibilityRole] = "visibility";
    roles[UvIndexRole] = "uvIndex";
    roles[WeatherConditionRole] = "weatherCondition";
    roles[WeatherDescriptionRole] = "weatherDescription";
    return roles;
}

void ForecastModel::addForecast(WeatherData* forecast) {
    if (!forecast) {
        return;
    }
    
    beginInsertRows(QModelIndex(), m_forecasts.size(), m_forecasts.size());
    m_forecasts.append(forecast);
    endInsertRows();
    
    emit forecastAdded(forecast);
}

void ForecastModel::addForecasts(const QList<WeatherData*>& forecasts) {
    if (forecasts.isEmpty()) {
        return;
    }
    
    beginInsertRows(QModelIndex(), m_forecasts.size(), m_forecasts.size() + forecasts.size() - 1);
    for (WeatherData* forecast : forecasts) {
        if (forecast) {
            m_forecasts.append(forecast);
        }
    }
    endInsertRows();
    
    for (WeatherData* forecast : forecasts) {
        if (forecast) {
            emit forecastAdded(forecast);
        }
    }
}

void ForecastModel::clear() {
    if (m_forecasts.isEmpty()) {
        return;
    }
    
    beginRemoveRows(QModelIndex(), 0, m_forecasts.size() - 1);
    qDeleteAll(m_forecasts);
    m_forecasts.clear();
    endRemoveRows();
}

WeatherData* ForecastModel::get(int index) const {
    if (index < 0 || index >= m_forecasts.size()) {
        return nullptr;
    }
    return m_forecasts.at(index);
}

QList<WeatherData*> ForecastModel::getAll() const {
    return m_forecasts;
}

void ForecastModel::updateForecast(int index, WeatherData* forecast) {
    if (index < 0 || index >= m_forecasts.size() || !forecast) {
        return;
    }
    
    delete m_forecasts[index];
    m_forecasts[index] = forecast;
    
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
}

