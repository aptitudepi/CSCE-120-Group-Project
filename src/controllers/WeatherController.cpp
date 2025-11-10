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
    , m_serviceProvider(NWS)
{
    // Set Pirate Weather API key
    m_pirateService->setApiKey("WsdojoTv0GEHU60N9mwRXuXoXc9tOama");
    
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

QString WeatherController::serviceProvider() const {
    switch (m_serviceProvider) {
        case NWS:
            return "NWS";
        case PirateWeather:
            return "PirateWeather";
        default:
            return "NWS";
    }
}

void WeatherController::setServiceProvider(int provider) {
    ServiceProvider newProvider = static_cast<ServiceProvider>(provider);
    if (newProvider != m_serviceProvider) {
        m_serviceProvider = newProvider;
        emit serviceProviderChanged();
        qInfo() << "Service provider changed to:" << serviceProvider();
    }
}

void WeatherController::fetchForecast(double latitude, double longitude) {
    qInfo() << "Fetching forecast for" << latitude << longitude << "using" << serviceProvider();
    
    m_lastLat = latitude;
    m_lastLon = longitude;
    
    setLoading(true);
    setErrorMessage("");
    
    // Check cache first (cache key includes service provider)
    QString cacheKey = generateCacheKey(latitude, longitude);
    QList<WeatherData*> cachedData = loadFromCache(cacheKey);
    
    if (!cachedData.isEmpty()) {
        qDebug() << "Using cached forecast data";
        onForecastReady(cachedData);
        return;
    }
    
    // Cache miss - fetch from selected service
    switch (m_serviceProvider) {
        case NWS:
            qDebug() << "Cache miss - fetching from NWS API";
            m_nwsService->fetchForecast(latitude, longitude);
            break;
        case PirateWeather:
            if (m_pirateService->isAvailable()) {
                qDebug() << "Cache miss - fetching from Pirate Weather API";
                m_pirateService->fetchForecast(latitude, longitude);
            } else {
                setErrorMessage("Pirate Weather API key not available");
                setLoading(false);
            }
            break;
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
    
    // Set parent for all WeatherData objects to the model so it owns them
    for (WeatherData* item : data) {
        if (item) {
            item->setParent(m_forecastModel);
        }
    }
    
    // Clear current before clearing model to avoid dangling pointer
    m_current = nullptr;
    emit currentChanged();
    
    // Update model
    m_forecastModel->clear();
    m_forecastModel->addForecasts(data);
    
    // Set current weather (first item) - just a reference, model owns it
    m_current = data.first();
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
    
    // Set parent to controller since this is standalone current weather
    data->setParent(this);
    
    if (m_current) {
        m_current->deleteLater();
    }
    m_current = data;
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
    // Include service provider in cache key to avoid mixing data from different services
    QString service = serviceProvider().toLower();
    return CacheManager::generateKey(QString("forecast_%1").arg(service), lat, lon);
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

