#include "controllers/WeatherController.h"
#include "models/WeatherData.h"
#include "models/ForecastModel.h"
#include "database/DatabaseManager.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QVariant>
#include <QByteArray>
#include <QMetaType>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QVariantList>
#include "utils/EnvLoader.h"
#include <QProcessEnvironment>
#include <cmath>

WeatherController::WeatherController(QObject *parent)
    : QObject(parent)
    , m_forecastModel(new ForecastModel(this))
    , m_current(nullptr)
    , m_nwsService(new NWSService(this))
    , m_pirateService(new PirateWeatherService(this))
    , m_weatherbitService(new WeatherbitService(this))
    , m_cache(new CacheManager(50, this))
    , m_aggregator(new WeatherAggregator(this))
    , m_performanceMonitor(new PerformanceMonitor(this))
    , m_historicalManager(new HistoricalDataManager(this))
    , m_nowcastEngine(new NowcastEngine(this))
    , m_loading(false)
    , m_lastLat(0.0)
    , m_lastLon(0.0)
    , m_serviceProvider(NWS)
    , m_useAggregation(false)
{
    EnvLoader::loadFromFile();

    // Set Pirate Weather API key from environment (overrides service default)
    QString pirateKey = qEnvironmentVariable("PIRATE_WEATHER_API_KEY");
    if (!pirateKey.isEmpty()) {
        m_pirateService->setApiKey(pirateKey);
    } else if (!m_pirateService->hasApiKey()) {
        qWarning() << "PIRATE_WEATHER_API_KEY is not set. Pirate Weather requests will fail.";
    }

    QString weatherbitKey = qEnvironmentVariable("WEATHERBIT_API_KEY");
    if (!weatherbitKey.isEmpty()) {
        m_weatherbitService->setApiKey(weatherbitKey);
    } else if (!m_weatherbitService->hasApiKey()) {
        qWarning() << "WEATHERBIT_API_KEY is not set. Weatherbit requests will be skipped.";
    }
    
    // Initialize historical data manager
    m_historicalManager->initialize();
    
    // Setup aggregator with services
    m_aggregator->addService(m_nwsService, 10); // Higher priority for NWS
    m_aggregator->addService(m_weatherbitService, 7);
    m_aggregator->addService(m_pirateService, 5);
    m_aggregator->setStrategy(WeatherAggregator::WeightedAverage); // Use weighted average strategy
    m_aggregator->setMovingAverageEnabled(true); // Enable moving average smoothing
    m_aggregator->setMovingAverageWindowSize(10);
    m_aggregator->setMovingAverageType(MovingAverageFilter::Exponential);
    m_aggregator->setMovingAverageAlpha(0.2);
    
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

    // Connect Weatherbit service
    connect(m_weatherbitService, &WeatherbitService::forecastReady,
            this, &WeatherController::onForecastReady);
    connect(m_weatherbitService, &WeatherbitService::currentReady,
            this, &WeatherController::onCurrentReady);
    connect(m_weatherbitService, &WeatherbitService::error,
            this, &WeatherController::onServiceError);
    
    // Connect aggregator
    connect(m_aggregator, &WeatherAggregator::forecastReady,
            this, &WeatherController::onAggregatorForecastReady);
    connect(m_aggregator, &WeatherAggregator::currentReady,
            this, &WeatherController::onCurrentReady);
    connect(m_aggregator, &WeatherAggregator::error,
            this, &WeatherController::onAggregatorError);
    
    // Connect performance monitor
    connect(m_performanceMonitor, &PerformanceMonitor::metricsUpdated,
            this, &WeatherController::performanceMonitorChanged);

    // Default to aggregated mode only if API keys are available
    if (m_pirateService->hasApiKey() || m_weatherbitService->hasApiKey()) {
        setUseAggregation(true);
    } else {
        qInfo() << "No third-party API keys found. Defaulting to NWS only.";
        setUseAggregation(false);
        setServiceProvider(NWS);
    }
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
        case Aggregated:
            return "Aggregated";
        case Weatherbit:
            return "Weatherbit";
        default:
            return "NWS";
    }
}

void WeatherController::setUseAggregation(bool use) {
    if (m_useAggregation != use) {
        m_useAggregation = use;
        if (use) {
            m_serviceProvider = Aggregated;
            // Set aggregator to weighted average when aggregation is enabled
            m_aggregator->setStrategy(WeatherAggregator::WeightedAverage);
            emit serviceProviderChanged();
        }
        emit useAggregationChanged();
    }
}

void WeatherController::setServiceProvider(int provider) {
    ServiceProvider newProvider = static_cast<ServiceProvider>(provider);
    if (newProvider != m_serviceProvider) {
        if (m_aggregator) {
            m_aggregator->cancelSpatioTemporalRequests();
        }
        m_serviceProvider = newProvider;
        emit serviceProviderChanged();
        qInfo() << "Service provider changed to:" << serviceProvider();
    }
}

void WeatherController::fetchForecast(double latitude, double longitude) {
    if (!isValidCoordinate(latitude, longitude)) {
        qWarning() << "Rejected invalid coordinates" << latitude << longitude;
        setErrorMessage("Invalid GPS coordinates");
        return;
    }
    
    qInfo() << "Fetching forecast for" << latitude << longitude << "using" << serviceProvider();
    
    m_lastLat = latitude;
    m_lastLon = longitude;
    
    setLoading(true);
    setErrorMessage("");
    
    // Record request for performance monitoring
    QString requestId = QString("forecast_%1_%2_%3")
        .arg(latitude, 0, 'f', 4)
        .arg(longitude, 0, 'f', 4)
        .arg(QDateTime::currentMSecsSinceEpoch());
    m_performanceMonitor->recordForecastRequest(requestId);
    
    // Check cache first (cache key includes service provider)
    QString cacheKey = generateCacheKey(latitude, longitude);
    QList<WeatherData*> cachedData = loadFromCache(cacheKey);
    
    if (!cachedData.isEmpty()) {
        qDebug() << "Using cached forecast data";
        m_performanceMonitor->recordForecastResponse(requestId, 0); // Cache hit is instant
        onForecastReady(cachedData);
        return;
    }
    
    // Cache miss - fetch from selected service
    QElapsedTimer timer;
    timer.start();
    
    switch (m_serviceProvider) {
        case NWS:
            qDebug() << "Cache miss - fetching from NWS API";
            m_nwsService->fetchForecast(latitude, longitude);
            break;
        case PirateWeather:
            if (m_pirateService->isAvailable()) {
                m_pirateService->cancelActiveRequests();
                qDebug() << "Cache miss - fetching from Pirate Weather API";
                m_pirateService->fetchForecast(latitude, longitude);
            } else {
                setErrorMessage("Pirate Weather API key not available");
                setLoading(false);
            }
            break;
        case Weatherbit:
            if (m_weatherbitService->isAvailable()) {
                qDebug() << "Cache miss - fetching from Weatherbit API";
                m_weatherbitService->fetchForecast(latitude, longitude);
            } else {
                setErrorMessage("Weatherbit API key not available");
                setLoading(false);
            }
            break;
        case Aggregated:
            if (m_useAggregation) {
                qDebug() << "Cache miss - fetching from aggregated services";
                m_aggregator->fetchForecast(latitude, longitude);
            } else {
                // Fallback to NWS if aggregation disabled
                m_nwsService->fetchForecast(latitude, longitude);
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
    bool callerOwnsData = true;
    if (!shouldProcessServiceResponse(sender(), callerOwnsData)) {
        qDebug() << "Ignoring forecast from inactive source";
        if (callerOwnsData) {
            qDeleteAll(data);
        }
        return;
    }

    qInfo() << "Received" << data.size() << "forecast periods";
    
    if (data.isEmpty()) {
        setErrorMessage("No forecast data available");
        setLoading(false);
        return;
    }
    
    // Record response time for performance monitoring
    QString requestId = QString("forecast_%1_%2")
        .arg(m_lastLat, 0, 'f', 4)
        .arg(m_lastLon, 0, 'f', 4);
    // Note: Actual response time would be tracked per request
    m_performanceMonitor->recordForecastResponse(requestId, 1000); // Placeholder
    
    // Record service up
    m_performanceMonitor->recordServiceUp(serviceProvider());
    
    // Store forecasts to historical storage
    if (m_historicalManager) {
        QString source = serviceProvider().toLower();
        if (m_useAggregation && m_serviceProvider == Aggregated) {
            source = "merged";
        }
        m_historicalManager->storeForecasts(m_lastLat, m_lastLon, data, source);
    }
    
    // Set parent for all WeatherData objects to the model so it owns them
    for (WeatherData* item : data) {
        if (item) {
            item->setParent(m_forecastModel);
        }
    }
    
    // Clear current before clearing model to avoid dangling pointer
    if (m_current && m_current->parent() == this) {
        m_current->deleteLater();
    }
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

void WeatherController::onAggregatorForecastReady(QList<WeatherData*> data) {
    // Store individual source forecasts before merging
    // Note: Individual source data is stored in onServiceForecastReady before merging
    
    // Same handling as regular forecast ready (stores merged forecasts)
    onForecastReady(data);
}

void WeatherController::onAggregatorError(QString error) {
    qWarning() << "Aggregator error:" << error << "- Falling back to NWS";
    m_performanceMonitor->recordServiceDown("Aggregated");

    // Fallback to NWS only if we were using aggregation
    if (m_useAggregation) {
        setUseAggregation(false);
        setServiceProvider(NWS);
        
        // Retry with NWS
        if (m_lastLat != 0.0 || m_lastLon != 0.0) {
            fetchForecast(m_lastLat, m_lastLon);
        }
    } else {
        // Already on NWS or other provider, just report error
        onServiceError(error);
    }
}

void WeatherController::onCurrentReady(WeatherData* data) {
    bool callerOwns = true;
    if (!shouldProcessServiceResponse(sender(), callerOwns)) {
        qDebug() << "Ignoring current weather from inactive source";
        if (callerOwns) {
            delete data;
        }
        return;
    }
    if (!data) {
        return;
    }
    
    // Set parent to controller since this is standalone current weather
    data->setParent(this);
    
    if (m_current && m_current->parent() == this) {
        m_current->deleteLater();
    }
    m_current = data;
    emit currentChanged();
}

void WeatherController::onServiceError(QString error) {
    bool callerOwns = true;
    if (!shouldProcessServiceResponse(sender(), callerOwns)) {
        qDebug() << "Ignoring service error from inactive source:" << error;
        return;
    }
    qWarning() << "Service error:" << error;
    setErrorMessage(error);
    setLoading(false);
    
    // Record service down for performance monitoring
    m_performanceMonitor->recordServiceDown(serviceProvider());
}

void WeatherController::fetchNowcast(double latitude, double longitude) {
    if (!isValidCoordinate(latitude, longitude)) {
        qWarning() << "Rejected invalid coordinates for nowcast" << latitude << longitude;
        setErrorMessage("Invalid GPS coordinates");
        return;
    }
    
    qInfo() << "Fetching nowcast for" << latitude << longitude;
    
    // First get current weather
    if (!m_current) {
        // Fetch current weather first
        if (m_serviceProvider == Aggregated && m_useAggregation) {
            m_aggregator->fetchForecast(latitude, longitude);
        } else if (m_serviceProvider == PirateWeather && m_pirateService->isAvailable()) {
            m_pirateService->fetchCurrent(latitude, longitude);
        } else if (m_serviceProvider == Weatherbit && m_weatherbitService->isAvailable()) {
            m_weatherbitService->fetchCurrent(latitude, longitude);
        } else {
            m_nwsService->fetchCurrent(latitude, longitude);
        }
        // Note: In a real implementation, we'd wait for current weather
        // For now, use existing current if available
    }
    
    if (!m_current) {
        setErrorMessage("No current weather data available for nowcast");
        return;
    }
    
    // Generate nowcast using current weather
    QList<WeatherData*> historicalData; // Would be populated from recent forecasts
    QList<WeatherData*> nowcast = m_nowcastEngine->generateNowcast(
        latitude, longitude, m_current, historicalData);
    
    if (!nowcast.isEmpty()) {
        // Record precipitation predictions for performance monitoring
        for (WeatherData* data : nowcast) {
            if (data->precipProbability() > 0.3 || data->precipIntensity() > 0.1) {
                QString location = QString("%1,%2").arg(latitude).arg(longitude);
                m_performanceMonitor->recordPrecipitationPrediction(
                    location, data->timestamp(), data->precipIntensity());
            }
        }
        
        emit nowcastReady(nowcast);
    } else {
        setErrorMessage("Failed to generate nowcast");
    }
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
    if (m_useAggregation && m_serviceProvider == Aggregated) {
        service = "aggregated";
    }
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

bool WeatherController::isValidCoordinate(double latitude, double longitude) const {
    if (!std::isfinite(latitude) || !std::isfinite(longitude)) {
        return false;
    }
    const bool latValid = latitude >= -90.0 && latitude <= 90.0;
    const bool lonValid = longitude >= -180.0 && longitude <= 180.0;
    return latValid && lonValid;
}

bool WeatherController::shouldProcessServiceResponse(QObject* source, bool& callerOwnsData) const {
    callerOwnsData = true;

    if (m_aggregator && m_aggregator->isSpatioTemporalActive()) {
        if (source != m_aggregator) {
            callerOwnsData = false;
            return false;
        }
    }

    if (source == m_aggregator) {
        if (m_useAggregation && m_serviceProvider == Aggregated) {
            return true;
        }
        callerOwnsData = true;
        return false;
    }
    
    if (source == m_nwsService) {
        return m_serviceProvider == NWS;
    }

    if (source == m_pirateService) {
        return m_serviceProvider == PirateWeather;
    }

    if (source == m_weatherbitService) {
        return m_serviceProvider == Weatherbit;
    }

    return false;
}

void WeatherController::saveLocation(const QString& name, double latitude, double longitude) {
    DatabaseManager* dbManager = DatabaseManager::instance();
    int locationId = -1;
    if (dbManager->saveLocation(name, latitude, longitude, locationId)) {
        qInfo() << "Location saved:" << name << "at" << latitude << longitude;
    } else {
        setErrorMessage("Failed to save location");
    }
}

QVariantList WeatherController::getSavedLocations() {
    DatabaseManager* dbManager = DatabaseManager::instance();
    QList<QVariantMap> locations;
    QVariantList result;
    
    if (dbManager->getLocations(locations)) {
        for (const QVariantMap& loc : locations) {
            result.append(loc);
        }
    }
    
    return result;
}

void WeatherController::deleteLocation(int locationId) {
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (dbManager->deleteLocation(locationId)) {
        qInfo() << "Location deleted:" << locationId;
    } else {
        setErrorMessage("Failed to delete location");
    }
}

void WeatherController::loadLocation(int locationId) {
    QVariantList locations = getSavedLocations();
    for (const QVariant& locVar : locations) {
        QVariantMap loc = locVar.toMap();
        if (loc["id"].toInt() == locationId) {
            double lat = loc["latitude"].toDouble();
            double lon = loc["longitude"].toDouble();
            fetchForecast(lat, lon);
            return;
        }
    }
}

void WeatherController::setPirateWeatherApiKey(const QString& apiKey) {
    if (m_pirateService) {
        m_pirateService->setApiKey(apiKey);
        qInfo() << "Pirate Weather API key updated";
        
        // Enable aggregation if we now have API keys
        if (m_pirateService->hasApiKey() || m_weatherbitService->hasApiKey()) {
            setUseAggregation(true);
        }
    }
}

void WeatherController::setWeatherbitApiKey(const QString& apiKey) {
    if (m_weatherbitService) {
        m_weatherbitService->setApiKey(apiKey);
        qInfo() << "Weatherbit API key updated";
        
        // Enable aggregation if we now have API keys
        if (m_pirateService->hasApiKey() || m_weatherbitService->hasApiKey()) {
            setUseAggregation(true);
        }
    }
}

