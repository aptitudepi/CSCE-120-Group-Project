# Architecture Comparison: Current State vs. Future State with Moving Average Backend

## Current State Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           USER INTERFACE (QML)                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │  MainPage    │  │ SettingsPage │  │ LocationInput│  │ ForecastCard │   │
│  └──────┬───────┘  └──────────────┘  └──────────────┘  └──────────────┘   │
└─────────┼──────────────────────────────────────────────────────────────────┘
          │
          │ Signals/Properties
          ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CONTROLLER LAYER (C++)                              │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      WeatherController                               │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ • m_nwsService          → NWSService                        │   │   │
│  │  │ • m_pirateService       → PirateWeatherService              │   │   │
│  │  │ • m_aggregator          → WeatherAggregator                 │   │   │
│  │  │ • m_cache               → CacheManager (LRU)                │   │   │
│  │  │ • m_nowcastEngine       → NowcastEngine                     │   │   │
│  │  │ • m_performanceMonitor  → PerformanceMonitor                │   │   │
│  │  │                                                                 │   │   │
│  │  │ Strategy: Can use single source OR aggregator                │   │   │
│  │  │ Default: PrimaryOnly or Fallback (not WeightedAverage)       │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      AlertController                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               │ Fetches forecasts
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SERVICE LAYER (C++)                                 │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    WeatherAggregator                                 │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ Strategies:                                                  │   │   │
│  │  │  • PrimaryOnly    ✓ Works                                    │   │   │
│  │  │  • Fallback       ✓ Works                                    │   │   │
│  │  │  • WeightedAverage ✗ STUB - returns first item only         │   │   │
│  │  │  • BestAvailable   ✗ STUB - tries all but doesn't merge    │   │   │
│  │  │                                                               │   │   │
│  │  │ mergeForecasts():                                            │   │   │
│  │  │   ┌─────────────────────────────────────────────────────┐   │   │   │
│  │  │   │ return forecasts.first(); // STUB!                  │   │   │   │
│  │  │   └─────────────────────────────────────────────────────┘   │   │   │
│  │  │                                                               │   │   │
│  │  │ mergeCurrentWeather():                                       │   │   │
│  │  │   ┌─────────────────────────────────────────────────────┐   │   │   │
│  │  │   │ return currentData.first(); // STUB!                │   │   │   │
│  │  │   └─────────────────────────────────────────────────────┘   │   │   │
│  │  │                                                               │   │   │
│  │  │ calculateConfidence():                                       │   │   │
│  │  │   • Based on success/failure rate                           │   │   │
│  │  │   • Simple calculation                                      │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                               │         │                                   │
│                               │         │                                   │
│           ┌───────────────────┘         └───────────────────┐             │
│           ▼                                                 ▼             │
│  ┌──────────────────┐                          ┌──────────────────┐      │
│  │   NWSService     │                          │ PirateWeather    │      │
│  │                  │                          │    Service       │      │
│  │ • Points API     │                          │                  │      │
│  │ • Gridpoint API  │                          │ • Forecast API   │      │
│  │ • Forecast API   │                          │ • Minutely data  │      │
│  │ • Hourly API     │                          │ • Hourly data    │      │
│  │ • Alerts API     │                          │                  │      │
│  │                  │                          │                  │      │
│  │ Returns:         │                          │ Returns:         │      │
│  │ QList<WeatherData*>│                        │ QList<WeatherData*>│     │
│  └────────┬─────────┘                          └────────┬─────────┘      │
│           │                                            │                  │
│           └──────────────┬─────────────────────────────┘                  │
│                          │                                                 │
│                          ▼                                                 │
│              ┌─────────────────────────────┐                              │
│              │     WeatherData Model       │                              │
│              │  (Single weather snapshot)  │                              │
│              └─────────────────────────────┘                              │
└──────────────────────────────┬─────────────────────────────────────────────┘
                               │
                               │ Stores
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         STORAGE LAYER                                        │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    CacheManager (In-Memory LRU)                      │   │
│  │  • Key: "forecast_{service}_{lat}_{lon}"                            │   │
│  │  • Value: QList<WeatherData*> as JSON                                │   │
│  │  • TTL: 3600 seconds (1 hour)                                        │   │
│  │  • Max Size: 50-100 entries                                          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │              DatabaseManager (SQLite)                                │   │
│  │  Tables:                                                             │   │
│  │  • locations       - User saved locations                            │   │
│  │  • alerts          - Alert configurations                            │   │
│  │  • user_preferences - App settings                                   │   │
│  │                                                                       │   │
│  │  ❌ NO historical weather data storage                               │   │
│  │  ❌ NO time-series data                                              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                         NOWCASTING ENGINE                                    │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    NowcastEngine                                     │   │
│  │  • generateNowcast() - Simple persistence (Phase 1 MVP)             │   │
│  │  • predictPrecipitationStart() - Based on nowcast data              │   │
│  │  • predictPrecipitationEnd() - Based on nowcast data                │   │
│  │                                                                       │   │
│  │  ❌ NO temporal smoothing                                            │   │
│  │  ❌ NO moving average                                                │   │
│  │  ❌ Uses only current data, no historical blending                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      CURRENT DATA FLOW                                       │
│                                                                              │
│  User Request → WeatherController                                           │
│                    │                                                         │
│                    ├─→ Single Source (NWS OR Pirate)                        │
│                    │   │                                                     │
│                    │   └─→ Returns: Single QList<WeatherData*>              │
│                    │                                                         │
│                    └─→ Aggregator (if enabled)                              │
│                        │                                                     │
│                        ├─→ Strategy: PrimaryOnly → Returns first            │
│                        ├─→ Strategy: Fallback → Returns first working       │
│                        ├─→ Strategy: WeightedAverage → ✗ STUB              │
│                        │                          Returns first only        │
│                        └─→ Strategy: BestAvailable → ✗ STUB                │
│                                               Returns first only            │
│                                                                              │
│  Result:                                                                    │
│  • Only ONE source's data is used                                           │
│  • No data fusion/combining                                                 │
│  • No temporal smoothing                                                    │
│  • No historical data retention                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Future State Architecture (After Implementation)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           USER INTERFACE (QML)                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │  MainPage    │  │ SettingsPage │  │ LocationInput│  │ ForecastCard │   │
│  │              │  │ +Moving Avg  │  │              │  │              │   │
│  │              │  │  Config      │  │              │  │              │   │
│  └──────┬───────┘  └──────────────┘  └──────────────┘  └──────────────┘   │
└─────────┼──────────────────────────────────────────────────────────────────┘
          │
          │ Signals/Properties
          ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CONTROLLER LAYER (C++)                              │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      WeatherController                               │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ • m_nwsService          → NWSService                        │   │   │
│  │  │ • m_pirateService       → PirateWeatherService              │   │   │
│  │  │ • m_aggregator          → WeatherAggregator (ENHANCED)      │   │   │
│  │  │ • m_cache               → CacheManager (LRU)                │   │   │
│  │  │ • m_nowcastEngine       → NowcastEngine                     │   │   │
│  │  │ • m_performanceMonitor  → PerformanceMonitor                │   │   │
│  │  │ • m_historicalManager   → HistoricalDataManager (NEW!)      │   │   │
│  │  │                                                                 │   │   │
│  │  │ Default Strategy: WeightedAverage                             │   │   │
│  │  │                                                                 │   │   │
│  │  │ onForecastReady():                                            │   │   │
│  │  │   1. Store forecasts to historical storage                    │   │   │
│  │  │   2. Store merged forecasts                                   │   │   │
│  │  │   3. Update model                                             │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      AlertController                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               │ Fetches forecasts
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SERVICE LAYER (C++)                                 │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    WeatherAggregator (ENHANCED)                     │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ Strategies:                                                  │   │   │
│  │  │  • PrimaryOnly    ✓ Works                                    │   │   │
│  │  │  • Fallback       ✓ Works                                    │   │   │
│  │  │  • WeightedAverage ✓ FULLY IMPLEMENTED                       │   │   │
│  │  │  • BestAvailable   ✓ FULLY IMPLEMENTED                       │   │   │
│  │  │                                                               │   │   │
│  │  │ mergeForecasts():                                            │   │   │
│  │  │   ┌─────────────────────────────────────────────────────┐   │   │   │
│  │  │   │ 1. Group forecasts by timestamp (bin to 15-30 min)  │   │   │   │
│  │  │   │ 2. Calculate weights per source:                     │   │   │   │
│  │  │   │    • Confidence (success rate)                       │   │   │   │
│  │  │   │    • Priority (service priority)                     │   │   │   │
│  │  │   │    • Recency (data freshness)                        │   │   │   │
│  │  │   │    • Response time (faster = higher weight)          │   │   │   │
│  │  │   │ 3. For each time bin:                                │   │   │   │
│  │  │   │    • Weighted average: temperature, precip, etc.     │   │   │   │
│  │  │   │    • Vector average: wind direction                  │   │   │   │
│  │  │   │    • Handle missing data gracefully                  │   │   │   │
│  │  │   │ 4. Create merged WeatherData objects                 │   │   │   │
│  │  │   │ 5. Apply MovingAverageFilter if enabled              │   │   │   │
│  │  │   └─────────────────────────────────────────────────────┘   │   │   │
│  │  │                                                               │   │   │
│  │  │ mergeCurrentWeather():                                       │   │   │
│  │  │   ┌─────────────────────────────────────────────────────┐   │   │   │
│  │  │   │ 1. Calculate weights for each source                 │   │   │   │
│  │  │   │ 2. Weighted average all numeric parameters           │   │   │   │
│  │  │   │ 3. Vector average wind direction                     │   │   │   │
│  │  │   │ 4. Return merged WeatherData                         │   │   │   │
│  │  │   └─────────────────────────────────────────────────────┘   │   │   │
│  │  │                                                               │   │   │
│  │  │ NEW: m_movingAverageFilter → MovingAverageFilter            │   │   │
│  │  │   • Window size: configurable (5, 10, 15, 30 readings)      │   │   │
│  │  │   • Type: Simple or Exponential                             │   │   │
│  │  │   • Applied after merging forecasts                         │   │   │
│  │  │                                                               │   │   │
│  │  │ Enhanced calculateConfidence():                             │   │   │
│  │  │   • Success/failure rate                                    │   │   │
│  │  │   • Response time history                                   │   │   │
│  │  │   • Data quality metrics                                    │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                               │         │                                   │
│                               │         │                                   │
│           ┌───────────────────┘         └───────────────────┐             │
│           ▼                                                 ▼             │
│  ┌──────────────────┐                          ┌──────────────────┐      │
│  │   NWSService     │                          │ PirateWeather    │      │
│  │                  │                          │    Service       │      │
│  │ (unchanged)      │                          │  (unchanged)     │      │
│  └────────┬─────────┘                          └────────┬─────────┘      │
│           │                                            │                  │
│           └──────────────┬─────────────────────────────┘                  │
│                          │                                                 │
│                          ▼                                                 │
│              ┌─────────────────────────────┐                              │
│              │     WeatherData Model       │                              │
│              │  (Single weather snapshot)  │                              │
│              └─────────────────────────────┘                              │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │           MovingAverageFilter (NEW COMPONENT)                       │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ Purpose: Temporal smoothing of weather data                  │   │   │
│  │  │                                                               │   │   │
│  │  │ Methods:                                                     │   │   │
│  │  │  • addDataPoint(WeatherData*) - Add to time series           │   │   │
│  │  │  • getMovingAverage(int window) - Last N readings            │   │   │
│  │  │  • getExponentialMovingAverage(double alpha) - EMA           │   │   │
│  │  │  • smoothForecast(QList<WeatherData*>) - Apply to list       │   │   │
│  │  │                                                               │   │   │
│  │  │ Config:                                                      │   │   │
│  │  │  • Window size per parameter type                            │   │   │
│  │  │  • Temperature: 10 readings                                  │   │   │
│  │  │  • Precipitation: 5 readings (more reactive)                 │   │   │
│  │  │  • Wind: 15 readings                                         │   │   │
│  │  │  • Alpha for EMA: 0.1-0.3 (configurable)                     │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               │ Stores & Retrieves Historical
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         STORAGE LAYER (ENHANCED)                            │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    CacheManager (In-Memory LRU)                      │   │
│  │  (unchanged - still used for fast access)                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │         HistoricalDataManager (NEW COMPONENT)                       │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ Purpose: Time-series weather data storage                    │   │   │
│  │  │                                                               │   │   │
│  │  │ Methods:                                                     │   │   │
│  │  │  • storeForecast(lat, lon, timestamp, source, WeatherData*) │   │   │
│  │  │  • getHistoricalData(lat, lon, startTime, endTime, source)  │   │   │
│  │  │  • getRecentData(lat, lon, hours, source)                    │   │   │
│  │  │  • cleanupOldData(daysToKeep)                                │   │   │
│  │  │                                                               │   │   │
│  │  │ Used by:                                                     │   │   │
│  │  │  • WeatherController - store forecasts after fetch           │   │   │
│  │  │  • MovingAverageFilter - retrieve historical for smoothing   │   │   │
│  │  │  • NowcastEngine - use historical for better predictions     │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │              DatabaseManager (SQLite - ENHANCED)                    │   │
│  │  Existing Tables:                                                   │   │
│  │  • locations                                                        │   │
│  │  • alerts                                                           │   │
│  │  • user_preferences                                                 │   │
│  │                                                                      │   │
│  │  NEW Table:                                                         │   │
│  │  • historical_weather                                               │   │
│  │    ┌──────────────────────────────────────────────────────────┐    │   │
│  │    │ id              INTEGER PRIMARY KEY                       │    │   │
│  │    │ latitude        REAL                                     │    │   │
│  │    │ longitude       REAL                                     │    │   │
│  │    │ timestamp       DATETIME                                 │    │   │
│  │    │ source          TEXT ('nws', 'pirate', 'merged')         │    │   │
│  │    │ temperature     REAL                                     │    │   │
│  │    │ precip_probability REAL                                  │    │   │
│  │    │ precip_intensity REAL                                    │    │   │
│  │    │ wind_speed      REAL                                     │    │   │
│  │    │ wind_direction  INTEGER                                  │    │   │
│  │    │ humidity        INTEGER                                  │    │   │
│  │    │ pressure        REAL                                     │    │   │
│  │    │ data_json       TEXT (full WeatherData JSON)             │    │   │
│  │    │ created_at      DATETIME                                 │    │   │
│  │    └──────────────────────────────────────────────────────────┘    │   │
│  │                                                                      │   │
│  │  Indexes:                                                           │   │
│  │  • idx_historical_location_time (latitude, longitude, timestamp)    │   │
│  │  • idx_historical_source_time (source, timestamp)                   │   │
│  │                                                                      │   │
│  │  Retention:                                                         │   │
│  │  • Default: 7 days                                                  │   │
│  │  • Configurable: via user_preferences                              │   │
│  │  • Auto-cleanup: daily background task                             │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                         NOWCASTING ENGINE (ENHANCED)                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    NowcastEngine                                     │   │
│  │  Existing:                                                           │   │
│  │  • generateNowcast() - Simple persistence                           │   │
│  │                                                                      │   │
│  │  Enhanced:                                                           │   │
│  │  • Can use HistoricalDataManager for historical blending            │   │
│  │  • Can use MovingAverageFilter for smoother predictions             │   │
│  │  • Better motion estimation using historical data                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      FUTURE DATA FLOW                                        │
│                                                                              │
│  User Request → WeatherController                                           │
│                    │                                                         │
│                    ├─→ Single Source (NWS OR Pirate)                        │
│                    │   │                                                     │
│                    │   └─→ Returns: Single QList<WeatherData*>              │
│                    │                                                         │
│                    └─→ Aggregator (ENABLED by default)                      │
│                        │                                                     │
│                        ├─→ Strategy: WeightedAverage ✓                      │
│                        │   │                                                  │
│                        │   ├─→ Fetches from NWS + Pirate Weather            │
│                        │   │   │                                              │
│                        │   │   ├─→ NWS: QList<WeatherData*> (14 periods)    │
│                        │   │   └─→ Pirate: QList<WeatherData*> (24 hours)   │
│                        │   │                                                  │
│                        │   ├─→ mergeForecasts():                            │
│                        │   │   • Align timestamps (bin to 15-min intervals) │
│                        │   │   • Calculate weights per source                │
│                        │   │   • Weighted average all parameters             │
│                        │   │   • Create merged WeatherData* list             │
│                        │   │                                                  │
│                        │   ├─→ MovingAverageFilter.smoothForecast():        │
│                        │   │   • Retrieve historical data from DB            │
│                        │   │   • Apply temporal smoothing                    │
│                        │   │   • Window: 10 readings (configurable)          │
│                        │   │   • Type: Exponential (alpha=0.2)               │
│                        │   │                                                  │
│                        │   └─→ Returns: Smoothed Merged QList<WeatherData*> │
│                        │                                                      │
│                        ├─→ WeatherController stores to HistoricalDataManager│
│                        │   • Store NWS forecasts                            │
│                        │   • Store Pirate Weather forecasts                 │
│                        │   • Store merged forecasts                         │
│                        │   • Store to SQLite historical_weather table       │
│                        │                                                      │
│                        └─→ Update UI with smoothed, merged forecasts        │
│                                                                              │
│  Result:                                                                    │
│  • MULTIPLE sources' data is combined with weights                         │
│  • Data fusion using confidence-based weighting                            │
│  • Temporal smoothing applied via moving average                           │
│  • Historical data retained for future use                                 │
│  • Better accuracy through multi-source consensus                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Key Differences Summary

### Components Added (NEW)
1. **HistoricalDataManager**
   - Stores time-series weather data
   - Enables historical queries for moving averages
   - Manages data retention and cleanup

2. **MovingAverageFilter**
   - Applies temporal smoothing to forecasts
   - Configurable window sizes per parameter type
   - Supports simple and exponential moving averages

### Components Enhanced (MODIFIED)
1. **WeatherAggregator**
   - `mergeForecasts()`: Full weighted average implementation
   - `mergeCurrentWeather()`: Full weighted average implementation
   - Integration with `MovingAverageFilter`
   - Enhanced confidence calculation

2. **DatabaseManager**
   - New `historical_weather` table
   - Indexes for efficient time-series queries
   - Data retention and cleanup logic

3. **WeatherController**
   - Stores forecasts to historical storage
   - Configures aggregator with WeightedAverage strategy
   - Manages moving average filter settings

### Data Flow Changes

**Before:**
```
NWS → WeatherController → UI
OR
Pirate → WeatherController → UI
(Only one source, no merging)
```

**After:**
```
NWS ─┐
     ├─→ Aggregator (Weighted Average) → MovingAverageFilter → HistoricalStorage
Pirate ─┘                                                          ↓
                                                               SQLite DB
                                                                    ↓
                                                            WeatherController → UI
(Combined, smoothed, stored for future use)
```

### Storage Changes

**Before:**
- Cache: Temporary in-memory storage (1 hour TTL)
- Database: Only user locations and alerts
- **No historical weather data**

**After:**
- Cache: Temporary in-memory storage (1 hour TTL) - unchanged
- Database: User locations, alerts, **AND historical weather**
- Historical data retained for 7 days (configurable)
- Enables time-series analysis and smoothing

### Algorithm Changes

**Weighted Average Calculation:**
```cpp
// Before: (stub)
return forecasts.first();

// After: (full implementation)
for (each time bin) {
    weight_NWS = confidence_NWS * priority_NWS * recency_NWS * (1/responseTime_NWS);
    weight_Pirate = confidence_Pirate * priority_Pirate * recency_Pirate * (1/responseTime_Pirate);
    totalWeight = weight_NWS + weight_Pirate;
    
    merged_temp = (NWS_temp * weight_NWS + Pirate_temp * weight_Pirate) / totalWeight;
    merged_precip = (NWS_precip * weight_NWS + Pirate_precip * weight_Pirate) / totalWeight;
    // ... etc for all parameters
}
```

**Moving Average Application:**
```cpp
// After merging, apply temporal smoothing
for (each forecast in mergedForecasts) {
    historicalData = historicalManager->getRecentData(lat, lon, windowSize);
    smoothed_temp = movingAverage(historicalData + forecast, windowSize);
    smoothed_precip = exponentialMovingAverage(historicalData + forecast, alpha);
    // ... etc
}
```

## Benefits After Implementation

1. **Better Accuracy**: Multi-source consensus improves forecast reliability
2. **Smoother Predictions**: Temporal moving average reduces noise and spikes
3. **Historical Context**: Past data can inform current predictions
4. **Flexibility**: Configurable weights and window sizes
5. **Resilience**: If one source fails, weighted average gracefully degrades
6. **Data Retention**: Historical data enables future enhancements (e.g., accuracy tracking, pattern recognition)

## Configuration Options Added

1. **Aggregation Strategy**: PrimaryOnly, Fallback, WeightedAverage, BestAvailable
2. **Moving Average Enabled**: Boolean toggle
3. **Moving Average Window Size**: 5, 10, 15, 30 readings
4. **Moving Average Type**: Simple or Exponential
5. **EMA Alpha**: 0.1-0.3 (smoothing factor)
6. **Historical Data Retention**: 3, 7, 14, 30 days

