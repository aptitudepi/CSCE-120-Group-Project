#include "controllers/WeatherController.h"
#include "models/WeatherData.h"
#include "models/ForecastModel.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QVariant>
#include <QByteArray>
#include <QMetaType>

WeatherController::WeatherController(QObject *parent)
    : QObject(parent)
    , m_forecastModel(new ForecastModel(this))
    , m_current(nullptr)
    , m_nwsService(new NWSService(this))
    , m_pirateService(new PirateWeatherService(this))
    , m_cache(new CacheManager(50, this))
    , m_loading(false)
    , m_lastLat(0.0)
    , m_lastLon(0.0)
{
    // Connect NWS service
    connect(m_nwsService, &NWSService::forecastReady,
            this, &WeatherController::onForecastReady);
    connect(m_nwsService, &NWSService::error,
            this, &WeatherController::onServiceError);
            
    // Connect Pirate Weather service
    connect(m_pirateService, &PirateWeatherService::forecastReady,
            this, &WeatherController::onForecastReady);
    connect(m_pirateService, &PirateWeatherService::currentReady,
            this, &WeatherController::onCurrentReady);
    connect(m_pirateService, &PirateWeatherService::error,
            this, &WeatherController::onServiceError);
}

WeatherController::~WeatherController() {
    // Models and services deleted by Qt parent-child relationship
}

void WeatherController::fetchForecast(double latitude, double longitude) {
    qInfo() << "Fetching forecast for" << latitude << longitude;
    
    m_lastLat = latitude;
    m_lastLon = longitude;
    
    setLoading(true);
    setErrorMessage("");
    
    // Check cache first
    QString cacheKey = generateCacheKey(latitude, longitude);
    QList<WeatherData*> cachedData = loadFromCache(cacheKey);
    
    if (!cachedData.isEmpty()) {
        qDebug() << "Using cached forecast data";
        onForecastReady(cachedData);
        return;
    }
    
    // Cache miss - fetch from API (prefer NWS, fallback to Pirate Weather)
    qDebug() << "Cache miss - fetching from NWS API";
    m_nwsService->fetchForecast(latitude, longitude);
    
    // Also try Pirate Weather if available for nowcasting
    if (m_pirateService->isAvailable()) {
        m_pirateService->fetchForecast(latitude, longitude);
    }
}

void WeatherController::refreshForecast() {
    if (m_lastLat != 0.0 || m_lastLon != 0.0) {
        // Clear cache for this location to force refresh
        QString cacheKey = generateCacheKey(m_lastLat, m_lastLon);
        m_cache->remove(cacheKey);
        
        fetchForecast(m_lastLat, m_lastLon);
    }
}

void WeatherController::clearError() {
    setErrorMessage("");
}

void WeatherController::onForecastReady(QList<WeatherData*> data) {
    qInfo() << "Received" << data.size() << "forecast periods";
    
    if (data.isEmpty()) {
        setErrorMessage("No forecast data available");
        setLoading(false);
        return;
    }
    
    // Update model
    m_forecastModel->clear();
    m_forecastModel->addForecasts(data);
    
    // Set current weather (first item)
    if (m_current) {
        m_current->deleteLater();
    }
    m_current = data.first();
    m_current->setParent(this);
    emit currentChanged();
    
    // Cache the data
    QString cacheKey = generateCacheKey(m_lastLat, m_lastLon);
    saveToCache(cacheKey, data);
    
    setLoading(false);
    emit forecastUpdated();
}

void WeatherController::onCurrentReady(WeatherData* data) {
    if (!data) {
        return;
    }
    
    if (m_current) {
        m_current->deleteLater();
    }
    m_current = data;
    m_current->setParent(this);
    emit currentChanged();
}

void WeatherController::onServiceError(QString error) {
    qWarning() << "Service error:" << error;
    setErrorMessage(error);
    setLoading(false);
}

void WeatherController::setLoading(bool loading) {
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}

void WeatherController::setErrorMessage(const QString& message) {
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

QString WeatherController::generateCacheKey(double lat, double lon) const {
    return CacheManager::generateKey("forecast", lat, lon);
}

QList<WeatherData*> WeatherController::loadFromCache(const QString& key) {
    QVariant cached = m_cache->get(key);
    if (!cached.isValid()) {
        return QList<WeatherData*>();
    }
    
    QByteArray cacheData;
    if (cached.userType() == QMetaType::fromType<QByteArray>().id()) {
        cacheData = cached.toByteArray();
    } else if (cached.userType() == QMetaType::fromType<QString>().id()) {
        cacheData = cached.toString().toUtf8();
    } else {
        return QList<WeatherData*>();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(cacheData);
    if (doc.isNull() || !doc.isObject()) {
        return QList<WeatherData*>();
    }
    
    QJsonObject cacheObj = doc.object();
    QJsonArray forecastArray = cacheObj["forecasts"].toArray();
    QList<WeatherData*> forecasts;
    
    for (const QJsonValue& val : forecastArray) {
        WeatherData* data = WeatherData::fromJson(val.toObject(), this);
        if (data) {
            forecasts.append(data);
        }
    }
    
    return forecasts;
}

void WeatherController::saveToCache(const QString& key, const QList<WeatherData*>& data) {
    QJsonObject cacheObj;
    QJsonArray forecastArray;
    
    for (WeatherData* forecast : data) {
        forecastArray.append(forecast->toJson());
    }
    
    cacheObj["forecasts"] = forecastArray;
    cacheObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(cacheObj);
    m_cache->put(key, doc.toJson(), 3600); // 1 hour TTL
}

