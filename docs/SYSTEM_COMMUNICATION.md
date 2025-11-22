# System Communication Architecture

## Overview

This document explains how individual components of the Hyperlocal Weather application communicate with each other, including data flow, signals/slots, and architectural patterns.

## Architecture Pattern: MVC (Model-View-Controller)

The application follows the Model-View-Controller pattern with clear separation of concerns:

```
┌─────────────┐
│   View      │  QML UI Layer
│  (QML)      │
└──────┬──────┘
       │ Property Bindings & Signals
       ▼
┌─────────────┐
│ Controller  │  C++ Business Logic
│   (C++)     │
└──────┬──────┘
       │ Direct Calls & Signals
       ▼
┌─────────────┐
│   Model     │  Data Storage & Services
│  (C++)      │
└─────────────┘
```

## Component Communication Flow

### 1. User Interface → Controller

**Communication Method:** QML Property Bindings, Q_INVOKABLE Methods, Signals

**Example: Location Input**
```qml
// LocationInput.qml
onLocationSelected: function(lat, lon) {
    weatherController.fetchForecast(lat, lon)  // Q_INVOKABLE method
}
```

**Key Connections:**
- **Location Input** → `WeatherController::fetchForecast(lat, lon)`
- **Service Selector** → `WeatherController::setServiceProvider(index)`
- **Refresh Button** → `WeatherController::refreshForecast()`
- **Settings Button** → Navigate to SettingsPage

**Data Flow:**
- QML emits signals when user interacts
- C++ controller receives via Q_INVOKABLE methods
- Controller updates internal state

### 2. Controller → Services

**Communication Method:** Direct Object Calls, Qt Signals/Slots

**Example: WeatherController Fetching Forecast**
```cpp
// WeatherController.cpp
void WeatherController::fetchForecast(double lat, double lon) {
    if (m_serviceProvider == Aggregated) {
        // Use aggregator
        m_aggregator->fetchForecast(lat, lon);
    } else {
        // Use single service
        m_nwsService->fetchForecast(lat, lon);
    }
}
```

**Service Hierarchy:**
```
WeatherController
    ├── WeatherAggregator (coordination layer)
    │   ├── NWSService (data source)
    │   └── PirateWeatherService (data source)
    ├── HistoricalDataManager (storage)
    ├── MovingAverageFilter (processing)
    ├── CacheManager (caching)
    └── PerformanceMonitor (metrics)
```

**Communication Patterns:**

1. **Direct Calls:** Controller calls service methods directly
   ```cpp
   m_aggregator->fetchForecast(latitude, longitude);
   ```

2. **Signal/Slot Connections:** Services notify controller when data is ready
   ```cpp
   connect(m_aggregator, &WeatherAggregator::forecastReady,
           this, &WeatherController::onAggregatorForecastReady);
   ```

### 3. Services → External APIs

**Communication Method:** HTTP Requests via QNetworkAccessManager

**Example: NWSService Fetching Data**
```cpp
// NWSService.cpp
void NWSService::fetchForecast(double latitude, double longitude) {
    // Build API URL
    QString url = buildForecastUrl(latitude, longitude);
    
    // Create HTTP request
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    
    // Send request
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Connect response handler
    connect(reply, &QNetworkReply::finished, 
            this, &NWSService::onForecastReplyFinished);
}
```

**Network Flow:**
1. Service builds API URL with parameters
2. QNetworkAccessManager sends HTTP GET request
3. API responds with JSON data
4. Service parses JSON into WeatherData objects
5. Service emits `forecastReady` signal with data

### 4. Aggregator → Multiple Services

**Communication Method:** Qt Signals/Slots, Asynchronous Collection

**Example: Weighted Average Strategy**
```cpp
// WeatherAggregator.cpp
void WeatherAggregator::fetchForecast(double lat, double lon) {
    // Fetch from ALL available services
    for (WeatherService* service : availableServices) {
        service->fetchForecast(lat, lon);  // Parallel requests
    }
    
    // Wait for all responses...
}

void WeatherAggregator::onServiceForecastReady(QList<WeatherData*> data) {
    // Collect forecast from this service
    m_pendingForecasts[cacheKey].append({data, service, responseTime});
    
    // Check if all services responded
    if (allServicesResponded()) {
        // Merge forecasts using weighted average
        QList<WeatherData*> merged = mergeForecasts(m_pendingForecasts);
        
        // Apply moving average smoothing
        if (m_movingAverageEnabled) {
            merged = m_movingAverageFilter->smoothForecast(merged);
        }
        
        // Emit merged result
        emit forecastReady(merged);
    }
}
```

**Aggregation Flow:**
1. Aggregator sends parallel requests to all services
2. Each service responds independently (async)
3. Aggregator collects responses in `m_pendingForecasts`
4. When all responses arrive, merge using weighted average
5. Apply moving average smoothing
6. Emit single merged forecast

### 5. Controller → Storage

**Communication Method:** Direct Calls, Database Queries

**Example: Storing Historical Data**
```cpp
// WeatherController.cpp
void WeatherController::onForecastReady(QList<WeatherData*> data) {
    // Store to historical database
    if (m_historicalManager) {
        QString source = serviceProvider().toLower();
        m_historicalManager->storeForecasts(m_lastLat, m_lastLon, data, source);
    }
    
    // Update UI...
}
```

**Storage Hierarchy:**
```
WeatherController
    └── HistoricalDataManager
        └── DatabaseManager (Singleton)
            └── SQLite Database
```

**Communication Flow:**
1. Controller calls `HistoricalDataManager::storeForecasts()`
2. HistoricalDataManager gets database connection from DatabaseManager
3. Executes SQL INSERT statements
4. Data persisted to `historical_weather` table

### 6. Controller → Cache

**Communication Method:** Direct Calls, Key-Value Interface

**Example: Caching Forecasts**
```cpp
// WeatherController.cpp
void WeatherController::fetchForecast(double lat, double lon) {
    // Check cache first
    QString cacheKey = generateCacheKey(lat, lon);
    QList<WeatherData*> cached = loadFromCache(cacheKey);
    
    if (!cached.isEmpty()) {
        // Cache hit - use immediately
        onForecastReady(cached);
        return;
    }
    
    // Cache miss - fetch from service
    m_aggregator->fetchForecast(lat, lon);
}

void WeatherController::saveToCache(const QString& key, const QList<WeatherData*>& data) {
    // Serialize to JSON
    QJsonDocument doc = serializeForecasts(data);
    
    // Store in cache with 1-hour TTL
    m_cache->put(key, doc.toJson(), 3600);
}
```

**Cache Flow:**
1. Controller generates cache key from location and service
2. Checks `CacheManager::get(key)`
3. If found and not expired, use cached data
4. If not found, fetch from services
5. Store response in cache for future use

### 7. Moving Average Filter Integration

**Communication Method:** Direct Method Calls, Data Processing

**Example: Applying Moving Average**
```cpp
// WeatherAggregator.cpp
QList<WeatherData*> merged = mergeForecasts(m_pendingForecasts);

if (m_movingAverageEnabled) {
    // Get historical data for context
    QList<WeatherData*> historical = m_historicalManager->getRecentData(
        lat, lon, m_movingAverageFilter->dataPointCount());
    
    // Apply smoothing
    merged = m_movingAverageFilter->smoothForecast(merged, historical);
}
```

**Filter Flow:**
1. Aggregator receives merged forecasts
2. Retrieves historical data from HistoricalDataManager
3. Passes both to MovingAverageFilter
4. Filter calculates smoothed values
5. Returns smoothed forecasts

## Signal/Slot Connections

### Key Connections in main.cpp

```cpp
// WeatherController exposed to QML
engine.rootContext()->setContextProperty("weatherController", &weatherController);

// Service connections
connect(m_nwsService, &NWSService::forecastReady,
        &weatherController, &WeatherController::onForecastReady);

connect(m_pirateService, &PirateWeatherService::forecastReady,
        &weatherController, &WeatherController::onForecastReady);

// Aggregator connections
connect(m_aggregator, &WeatherAggregator::forecastReady,
        &weatherController, &WeatherController::onAggregatorForecastReady);

// Error handling
connect(m_nwsService, &NWSService::error,
        &weatherController, &WeatherController::onServiceError);
```

### Signal Flow Diagram

```
User Action (QML)
    │
    ▼
WeatherController::fetchForecast()
    │
    ├─→ [Single Service Path]
    │   │
    │   ▼
    │   NWSService::fetchForecast()
    │   │
    │   ▼
    │   HTTP Request → API
    │   │
    │   ▼
    │   NWSService::forecastReady(WeatherData*)
    │   │
    │   ▼
    │   WeatherController::onForecastReady()
    │
    └─→ [Aggregation Path]
        │
        ▼
        WeatherAggregator::fetchForecast()
        │
        ├─→ NWSService::fetchForecast() ──┐
        └─→ PirateWeatherService::fetchForecast() ──┤
            │                                    │
            ▼                                    ▼
        (Parallel HTTP Requests)            (Parallel HTTP Requests)
            │                                    │
            ▼                                    ▼
        NWSService::forecastReady()      PirateWeather::forecastReady()
            │                                    │
            └────────────────┬───────────────────┘
                           │
                           ▼
        WeatherAggregator::onServiceForecastReady()
                           │
                           ├─→ Collect responses
                           │
                           ▼
        All responses received?
                           │
                           ▼
        WeatherAggregator::mergeForecasts()
                           │
                           ├─→ MovingAverageFilter::smoothForecast()
                           │
                           ▼
        WeatherAggregator::forecastReady(merged)
                           │
                           ▼
        WeatherController::onAggregatorForecastReady()
                           │
                           ├─→ HistoricalDataManager::storeForecasts()
                           ├─→ CacheManager::put()
                           │
                           ▼
        WeatherController::forecastUpdated() [Signal]
                           │
                           ▼
        QML updates UI automatically via property bindings
```

## Data Flow Summary

### Complete Request Flow

1. **User enters location** (QML)
   - `LocationInput` emits `locationSelected(lat, lon)`
   - Calls `weatherController.fetchForecast(lat, lon)`

2. **Controller processes request** (C++)
   - Checks cache (fast path if hit)
   - Decides: single service or aggregator?
   - Sets loading state (UI shows spinner)

3. **Aggregator coordinates** (if using aggregation)
   - Fetches from NWS + Pirate Weather (parallel)
   - Waits for both responses
   - Merges using weighted average
   - Applies moving average smoothing

4. **Data processing** (C++)
   - HistoricalDataManager stores forecasts
   - CacheManager caches result
   - ForecastModel updates

5. **UI updates** (QML)
   - Property bindings detect changes
   - Forecast list updates automatically
   - Current weather card updates
   - Loading spinner disappears

### Error Handling Flow

1. **Service error occurs** (network timeout, API error)
   - Service emits `error(QString message)` signal
   - Controller receives via `onServiceError()`
   - Sets error message property

2. **QML error display**
   - Property binding detects `errorMessage` change
   - Error dialog popup opens
   - User sees error message

3. **Fallback** (if using aggregation)
   - If one service fails, aggregator continues with other
   - Uses cached data if available
   - Degrades gracefully

## Threading Model

**All operations run on main GUI thread** (Qt's event loop):
- Network requests are asynchronous (non-blocking)
- Database operations use SQLite (thread-safe, but called from main thread)
- Moving average calculations are synchronous but fast (<100ms)

**Future Enhancement:** Move heavy computations to worker threads for better responsiveness.

## Communication Patterns Summary

| From | To | Method | Purpose |
|------|-----|--------|---------|
| QML | Controller | Q_INVOKABLE | User actions |
| Controller | Services | Direct calls | Request data |
| Services | APIs | HTTP (async) | Fetch forecasts |
| Services | Controller | Signals | Notify data ready |
| Controller | Aggregator | Direct calls | Coordinate |
| Aggregator | Services | Direct calls | Parallel fetch |
| Aggregator | Filter | Direct calls | Smooth data |
| Controller | Storage | Direct calls | Persist data |
| Controller | Cache | Direct calls | Cache data |
| Controller | QML | Property bindings | Update UI |
| Controller | QML | Signals | Notify events |

This architecture ensures loose coupling, easy testing, and clear separation of concerns while maintaining efficient communication between components.

