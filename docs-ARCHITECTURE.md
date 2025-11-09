# Architecture Documentation

## System Overview

The Hyperlocal Weather application follows a **Model-View-Controller (MVC)** architecture with clear separation between the presentation layer (QML), business logic (C++ Controllers), and data access (Services/Models).

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Presentation Layer                     │
│                      (QML/Qt Quick)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  MainPage    │  │ ForecastCard │  │  AlertItem   │  │
│  │              │  │              │  │              │  │
│  └──────┬───────┘  └──────────────┘  └──────────────┘  │
└─────────┼───────────────────────────────────────────────┘
          │ Property Bindings & Signal/Slot
┌─────────▼───────────────────────────────────────────────┐
│                   Controller Layer                        │
│                      (C++ QObjects)                       │
│  ┌──────────────────┐       ┌───────────────────┐       │
│  │WeatherController │       │ AlertController   │       │
│  │  - fetchForecast │       │  - checkAlerts    │       │
│  │  - getForecast   │       │  - addAlert       │       │
│  └────────┬─────────┘       └─────────┬─────────┘       │
└───────────┼───────────────────────────┼─────────────────┘
            │                           │
┌───────────▼───────────────────────────▼─────────────────┐
│                   Service Layer                           │
│                 (Business Logic & APIs)                   │
│  ┌────────────┐  ┌────────────┐  ┌────────────────────┐ │
│  │ NWSService │  │PirateWeathr│  │  CacheManager      │ │
│  │            │  │  Service   │  │  - LRU Cache       │ │
│  └─────┬──────┘  └─────┬──────┘  │  - TTL Management  │ │
└────────┼───────────────┼─────────┴────────┬─────────────┘
         │               │                   │
         │  HTTP APIs    │                   │
┌────────▼───────────────▼─────────┐  ┌──────▼────────────┐
│   External Weather APIs           │  │    Data Layer     │
│  - NWS API (api.weather.gov)      │  │  ┌──────────────┐│
│  - Pirate Weather API             │  │  │DatabaseMgr   ││
│  - Other sources                  │  │  │  SQLite      ││
└───────────────────────────────────┘  │  └──────────────┘│
                                       │  ┌──────────────┐│
                                       │  │ Models       ││
                                       │  │ WeatherData  ││
                                       │  │ ForecastModel││
                                       │  └──────────────┘│
                                       └───────────────────┘
```

## Component Details

### 1. Presentation Layer (QML)

**Technology**: Qt Quick/QML 6.x

**Components**:
- `main.qml`: Application window and navigation
- `MainPage.qml`: Primary user interface with location input and forecast display
- `SettingsPage.qml`: Configuration and preferences
- `ForecastCard.qml`: Individual forecast display component
- `AlertItem.qml`: Weather alert notification component
- `LocationInput.qml`: Location selection (GPS/map/text)

**Responsibilities**:
- Render UI components
- Handle user interactions
- Display data from controllers via property bindings
- Emit signals for user actions

**Data Flow**: Read-only access to controller properties, emit signals for actions

### 2. Controller Layer (C++)

#### WeatherController

**Purpose**: Coordinates weather data fetching and presentation

**Key Methods**:
```cpp
Q_INVOKABLE void fetchForecast(double lat, double lon);
Q_INVOKABLE void refreshForecast();
Q_PROPERTY(QList<WeatherData*> forecast READ forecast NOTIFY forecastChanged)
```

**Responsibilities**:
- Receive requests from UI layer
- Coordinate between multiple weather services
- Aggregate data from different sources
- Manage cache strategy
- Emit signals when data is ready

#### AlertController

**Purpose**: Manages geofenced weather alerts

**Key Methods**:
```cpp
Q_INVOKABLE void addAlert(double lat, double lon, QString type, double threshold);
Q_INVOKABLE void checkAlerts();
Q_INVOKABLE void removeAlert(int alertId);
```

**Responsibilities**:
- Monitor weather conditions vs. alert thresholds
- Trigger notifications when conditions met
- Persist alert configurations
- Manage alert lifecycle

### 3. Service Layer

#### WeatherService (Abstract Base)

**Purpose**: Define interface for weather data providers

**Key Methods**:
```cpp
virtual void fetchForecast(double latitude, double longitude) = 0;
virtual void fetchCurrent(double latitude, double longitude) = 0;
signals:
    void forecastReady(QList<WeatherData*> data);
    void error(QString message);
```

#### NWSService

**Purpose**: National Weather Service API integration

**API Flow**:
1. Convert lat/lon to grid point: `GET /points/{lat},{lon}`
2. Extract forecast URL from response
3. Fetch forecast: `GET /gridpoints/{office}/{x},{y}/forecast`
4. Parse JSON response into WeatherData objects

**Features**:
- Conditional GET with If-Modified-Since
- Last-Modified caching
- Automatic retry on failure
- Rate limiting compliance

**NWS API Endpoints**:
- Points: `https://api.weather.gov/points/{lat},{lon}`
- Forecast: `https://api.weather.gov/gridpoints/{office}/{x},{y}/forecast`
- Hourly: `https://api.weather.gov/gridpoints/{office}/{x},{y}/forecast/hourly`
- Alerts: `https://api.weather.gov/alerts/active?point={lat},{lon}`

#### PirateWeatherService

**Purpose**: Pirate Weather API integration (Dark Sky replacement)

**API**: `GET https://api.pirateweather.net/forecast/{apikey}/{lat},{lon}`

**Features**:
- Minute-by-minute precipitation (nowcasting)
- Hourly forecasts up to 48 hours
- Daily forecasts up to 8 days
- Requires API key (user-provided)

#### CacheManager

**Purpose**: In-memory LRU cache with TTL

**Implementation**:
```cpp
class CacheManager {
    QMap<QString, CacheEntry> m_cache;
    QList<QString> m_accessOrder;  // LRU tracking
    QMap<QString, QDateTime> m_expiry;  // TTL tracking
    QMutex m_mutex;  // Thread safety
};
```

**Features**:
- Least Recently Used (LRU) eviction policy
- Time-to-live (TTL) expiration
- Thread-safe with mutex protection
- Automatic cleanup of expired entries
- Configurable max size

**Cache Keys**:
```
"nws_forecast_{lat}_{lon}_{timestamp}"
"pirate_weather_{lat}_{lon}_{timestamp}"
"nowcast_{lat}_{lon}_{timestamp}"
```

### 4. Data Layer

#### DatabaseManager

**Purpose**: SQLite database for persistent storage

**Schema**:

```sql
CREATE TABLE locations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    location_id INTEGER NOT NULL,
    alert_type TEXT NOT NULL,
    threshold REAL NOT NULL,
    enabled INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (location_id) REFERENCES locations(id)
);

CREATE TABLE user_preferences (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE forecast_cache (
    cache_key TEXT PRIMARY KEY,
    data TEXT NOT NULL,
    expires_at DATETIME NOT NULL
);
```

**Responsibilities**:
- Store user-saved locations
- Persist alert configurations
- Cache forecast data across sessions
- Store user preferences

#### Models

**WeatherData**: Single weather observation/forecast
- All weather parameters (temp, humidity, wind, precip, etc.)
- Timestamp and location
- JSON serialization/deserialization

**ForecastModel**: QAbstractListModel for forecast list
- Extends QAbstractListModel for QML ListView integration
- Provides roles for QML property access
- Auto-updates UI when data changes

**AlertModel**: QAbstractListModel for alerts
- Manages list of active alerts
- Provides CRUD operations
- Notifies UI of changes

### 5. Nowcasting Engine

**Purpose**: Short-term (0-90 minute) precipitation forecasting

**Algorithm** (MVP - Phase 1):
1. Fetch recent radar imagery (via NWS or external service)
2. Extract precipitation patterns
3. Apply optical flow or block-matching for motion tracking
4. Extrapolate forward in time
5. Generate minute-by-minute precipitation forecast

**Future Enhancements** (Phase 2-3):
- Blend radar nowcast with NWP model guidance
- Machine learning for improved accuracy
- Confidence scoring based on recent performance

## Data Flow Examples

### Forecast Retrieval Flow

```
User Action (QML) → Controller → Service → API/Cache → Parse → Model → UI Update
                                    ↓
                              Cache Manager
```

**Detailed Steps**:

1. User enters location in QML: `locationInput.locationSelected(lat, lon)`
2. QML calls controller: `weatherController.fetchForecast(lat, lon)`
3. Controller checks cache: `cacheManager.get("nws_forecast_{lat}_{lon}")`
4. If cache miss:
   a. Controller calls service: `nwsService.fetchForecast(lat, lon)`
   b. Service makes HTTP request to NWS API
   c. Service parses JSON response
   d. Service emits: `forecastReady(QList<WeatherData*>)`
5. Controller receives data:
   a. Stores in cache: `cacheManager.put(key, data, TTL=3600)`
   b. Updates model: `m_forecast = data`
   c. Emits: `forecastChanged()`
6. QML property binding auto-updates UI

**Timing Targets**:
- Cache hit: < 100ms
- API call: < 10s (p95)
- Total end-to-end: < 10s

### Alert Checking Flow

```
Timer → Controller → Check Conditions → Threshold Met? → Notify User
          ↓
    Fetch Current Weather
```

**Detailed Steps**:

1. Timer triggers: `alertController.checkAlerts()` (every 5 minutes)
2. Controller fetches current weather for each alert location
3. For each alert:
   a. Compare current conditions to threshold
   b. If threshold met and not recently notified:
      - Generate notification
      - Update last_notified timestamp
      - Emit signal to UI
4. QML displays notification dialog or system tray notification

## Threading Model

**Main Thread (Qt GUI Thread)**:
- QML UI rendering
- Property bindings
- Signal/slot connections

**Network Thread Pool**:
- QNetworkAccessManager uses Qt's thread pool
- HTTP requests are non-blocking
- Replies handled via signals in main thread

**Worker Threads** (future enhancement):
- Nowcasting computation
- Large data processing
- Background cache cleanup

**Thread Safety**:
- CacheManager uses QMutex for thread-safe access
- Database operations serialized through single connection
- Controllers emit signals from main thread only

## Error Handling Strategy

**Network Errors**:
```cpp
if (reply->error() != QNetworkReply::NoError) {
    qWarning() << "Network error:" << reply->errorString();
    emit error(reply->errorString());
    // Fall back to cached data
    tryLoadFromCache();
}
```

**Parse Errors**:
```cpp
QJsonDocument doc = QJsonDocument::fromJson(data);
if (doc.isNull()) {
    qCritical() << "Failed to parse JSON";
    emit error("Invalid server response");
    return;
}
```

**Database Errors**:
```cpp
if (!query.exec()) {
    qCritical() << "Database error:" << query.lastError().text();
    // Continue with in-memory only mode
    m_databaseAvailable = false;
}
```

**Graceful Degradation**:
1. Network failure → Use cached data
2. Cache miss → Show "no data" message with retry option
3. Database failure → Continue with in-memory only
4. API rate limit → Use alternative service or cached data

## Performance Optimizations

1. **Lazy Loading**: Only fetch data when needed
2. **Request Deduplication**: Don't fetch same data multiple times
3. **Conditional Requests**: Use If-Modified-Since headers
4. **Connection Pooling**: Reuse HTTP connections
5. **Async Operations**: All network I/O is asynchronous
6. **Efficient QML**: Minimize property bindings, use Loaders

## Security Considerations

1. **API Keys**: Never hardcode, use environment variables or settings
2. **HTTPS**: All API calls use TLS
3. **Input Validation**: Sanitize lat/lon inputs
4. **SQL Injection**: Use parameterized queries
5. **XSS Prevention**: Escape user input in QML

## Testing Strategy

See test suite for details:
- **Unit Tests**: Individual classes in isolation
- **Integration Tests**: Controller → Service → API flow
- **Performance Tests**: Latency and throughput benchmarks
- **UI Tests**: QML component interaction (future)

## Deployment

**Desktop Application**:
- Single executable with embedded QML resources
- SQLite database in user's app data directory
- Settings stored in OS-specific location (QSettings)

**System Requirements**:
- Linux: Ubuntu 24.04+ or equivalent
- Windows: Windows 10+ (future)
- macOS: 11+ (future)

## Future Enhancements

**Phase 2**:
- Radar-based nowcasting implementation
- Additional weather data sources
- Historical data and trends

**Phase 3**:
- Machine learning for forecast blending
- Confidence intervals and uncertainty quantification
- Mobile app version (Qt for Android/iOS)

**Phase 4**:
- Social features (share forecasts)
- Advanced visualizations (radar maps)
- Custom notification rules engine
