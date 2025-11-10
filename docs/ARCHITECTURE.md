# Architecture Documentation

## System Overview

The Hyperlocal Weather application follows a **Model-View-Controller (MVC)** architecture with clear separation between the presentation layer (QML), business logic (C++ Controllers), and data access (Services/Models).

## Six Key Modules

### 1. Qt/QML Frontend (View Layer)
**Location**: `src/qml/`
- **Purpose**: User interaction and presentation
- **Components**:
  - `main.qml` - Application entry point
  - `pages/MainPage.qml` - Main weather display
  - `components/` - Reusable UI components
- **Responsibilities**:
  - Display weather data
  - Handle user input
  - Show alerts and notifications
  - Service selection UI

### 2. C++ Backend Services (Controller Layer)
**Location**: `src/controllers/`
- **Purpose**: Business logic and coordination
- **Components**:
  - `WeatherController` - Manages weather data flow
  - `AlertController` - Manages geofenced alerts
- **Responsibilities**:
  - Coordinate between UI and services
  - Manage application state
  - Handle user actions
  - Enforce business rules

### 3. Data Source Layer (Service Layer)
**Location**: `src/services/`
- **Purpose**: Abstract weather API communication
- **Components**:
  - `WeatherService` - Abstract base class
  - `NWSService` - NWS API implementation
  - `PirateWeatherService` - Pirate Weather API implementation
  - `WeatherAggregator` - Multi-source data aggregation
- **Responsibilities**:
  - Abstract API differences
  - Handle network communication
  - Parse API responses
  - Provide unified data interface

### 4. Nowcasting Engine
**Location**: `src/nowcast/`
- **Purpose**: Short-term precipitation prediction (0-90 minutes)
- **Components**:
  - `NowcastEngine` - Core nowcasting logic
- **Responsibilities**:
  - Generate minute-by-minute precipitation forecasts
  - Estimate precipitation motion
  - Calculate confidence scores
  - Predict start/end times

### 5. Caching Layer
**Location**: `src/services/CacheManager.cpp`, `src/database/`
- **Purpose**: Performance optimization and offline capability
- **Components**:
  - `CacheManager` - In-memory LRU cache
  - `DatabaseManager` - SQLite persistence
- **Responsibilities**:
  - Cache API responses
  - Manage TTL (Time To Live)
  - Persist data for offline access
  - Optimize response times

### 6. Geofenced Alerts Engine
**Location**: `src/controllers/AlertController.cpp`
- **Purpose**: Location-based weather alerts
- **Components**:
  - `AlertController` - Alert management
  - `AlertModel` - Alert data model
- **Responsibilities**:
  - Monitor weather conditions
  - Trigger alerts based on thresholds
  - Track alert lead times
  - Manage geofenced regions

## MVC Architecture

### Model Layer
**Location**: `src/models/`
- `WeatherData` - Weather data point
- `ForecastModel` - QML model for forecasts
- `AlertModel` - Alert data structure

### View Layer
**Location**: `src/qml/`
- QML files for UI presentation
- Property bindings for data display
- User interaction handling

### Controller Layer
**Location**: `src/controllers/`
- Mediates between View and Model
- Handles business logic
- Manages service coordination

## Communication Channels

### View ↔ Controller
- **QML Property Bindings**: Controllers expose Q_PROPERTY for QML access
- **Q_INVOKABLE Methods**: QML can call controller methods
- **Signals**: Controllers emit signals that QML can connect to

### Controller ↔ Service
- **Qt Signals/Slots**: Asynchronous communication
- **Direct Method Calls**: Synchronous operations
- **Service Interface**: Abstract base class ensures consistency

### Service ↔ Data Source
- **HTTP Requests**: QNetworkAccessManager for API calls
- **JSON Parsing**: QJsonDocument for response parsing
- **Error Handling**: Standardized error signals

## Aggregator Pattern

The `WeatherAggregator` service implements the aggregator pattern to:
- Combine data from multiple sources (NWS, Pirate Weather)
- Implement fallback strategies
- Weighted averaging of forecasts
- Best-available source selection
- Performance tracking per source

## Success Criteria Tracking

### Performance Metrics
- **Forecast Delivery**: < 10 seconds (tracked by PerformanceMonitor)
- **Precipitation Hit Rate**: > 75% (tracked via prediction/observation records)
- **Service Uptime**: 95% (tracked per service)
- **Alert Lead Time**: ≥ 5 minutes (tracked per alert)
- **Test Coverage**: 75% on critical modules (tracked per module)

### Monitoring
The `PerformanceMonitor` class tracks all KPIs and emits warnings when thresholds are not met.

## Testing Strategy

### Unit Tests
- Data parsing (JSON, API responses)
- Caching logic (LRU, TTL)
- Nowcasting algorithms
- Alert threshold evaluation
- Aggregation logic

### Integration Tests
- End-to-end forecast flow
- Service fallback scenarios
- Network failure simulation
- Low compute resource simulation
- Cache hit/miss scenarios

### Backtesting
- Historical forecast accuracy
- Precipitation prediction validation
- Alert lead time analysis
- Service performance over time

## Future Enhancements

1. **Radar Extrapolation**: More sophisticated nowcasting using radar data
2. **Prior Validation**: Blend nowcasts with historical priors
3. **UI Testing**: Automated UI/UX testing
4. **ML Integration**: Machine learning for improved predictions

