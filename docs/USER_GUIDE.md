# User Guide: Hyperlocal Weather Application

## Introduction

The Hyperlocal Weather Application provides accurate, location-specific weather forecasts by combining data from multiple weather services. This guide explains how to use the application effectively.

## Getting Started

### Launching the Application

1. **Build the application** (see [SETUP.md](SETUP.md) for installation instructions)
2. **Run the executable:**
   ```bash
   ./build/bin/HyperlocalWeather
   ```
3. The main window will open showing the weather interface

### Main Interface Overview

The application window has several key sections:

```
┌─────────────────────────────────────────────────────────┐
│  Hyperlocal Weather        [Service Selector] [Refresh] │
├─────────────────────────────────────────────────────────┤
│  Service: Aggregated                                     │
├─────────────────────────────────────────────────────────┤
│  Location Input:                                         │
│  [Enter coordinates] [Use GPS] [Saved Locations]        │
├─────────────────────────────────────────────────────────┤
│  Current Weather                                         │
│  Temperature: 75°F                                       │
│  Condition: Partly Cloudy                               │
│  Humidity: 60% | Wind: 10 mph | Precip: 30%            │
├─────────────────────────────────────────────────────────┤
│  Forecast List:                                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Today 3:00 PM   75°F    Partly Cloudy             │  │
│  │ Precip: 40%    Wind: 12 mph                       │  │
│  └───────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Today 6:00 PM   72°F    Light Rain                │  │
│  │ Precip: 80%    Wind: 15 mph                       │  │
│  └───────────────────────────────────────────────────┘  │
│  ...                                                     │
└─────────────────────────────────────────────────────────┘
```

## Using Location Input

### Method 1: Manual Coordinates

1. Click the **Location Input** area
2. Enter latitude and longitude (decimal degrees)
   - Example: `30.2672, -97.7431` for Austin, TX
3. Press Enter or click "Search"
4. The forecast will load automatically

### Method 2: GPS Location

1. Click **"Use GPS"** button
2. Allow location permissions if prompted
3. The application will automatically fetch your current location
4. Forecast loads automatically

### Method 3: Saved Locations

1. Click **"Saved Locations"** dropdown
2. Select a previously saved location
3. Forecast loads automatically

**Saving a Location:**
- After entering coordinates, the location is automatically saved
- You can manage saved locations in Settings

## Selecting Weather Services

### Service Options

The dropdown menu in the top-right offers three options:

1. **NWS (National Weather Service)**
   - Free, government weather service
   - Reliable official forecasts
   - Best for US locations

2. **Pirate Weather**
   - Commercial weather API (requires API key)
   - Minute-by-minute precipitation forecasts
   - Enhanced nowcasting capabilities

3. **Aggregated (Recommended)**
   - **Weighted Average** of multiple sources
   - Combines NWS and Pirate Weather data
   - More accurate through consensus
   - Automatically applies moving average smoothing
   - **Best option for most users**

### Changing Services

1. Click the **Service Selector** dropdown
2. Choose your preferred service
3. The forecast will automatically refresh with the new service

## Understanding Forecasts

### Current Weather Display

The large card shows:
- **Temperature:** Current temperature in °F
- **Condition:** Weather description (e.g., "Partly Cloudy")
- **Details:** Humidity, Wind Speed, Precipitation Probability, Pressure

### Forecast List

Each forecast card shows:
- **Time:** When the forecast applies
- **Temperature:** Expected temperature
- **Condition:** Weather description
- **Precipitation Probability:** Chance of rain (0-100%)
- **Precipitation Intensity:** How heavy if it rains (mm/hour)
- **Wind Speed:** Expected wind speed in mph
- **Wind Direction:** Wind direction (N, NE, E, SE, S, SW, W, NW)

### Reading Precipitation Forecasts

- **Precipitation Probability:** Percentage chance of precipitation
  - 0-30%: Unlikely
  - 31-60%: Possible
  - 61-80%: Likely
  - 81-100%: Very likely

- **Precipitation Intensity:** How heavy the precipitation will be
  - < 0.1 mm/h: Very light (drizzle)
  - 0.1-0.5 mm/h: Light
  - 0.5-2.0 mm/h: Moderate
  - > 2.0 mm/h: Heavy

### Start/Stop Times

For aggregated forecasts, the system predicts:
- **Precipitation Start Time:** When precipitation will begin
- **Precipitation End Time:** When precipitation will end
- These are calculated using the nowcasting engine

## Advanced Features

### Aggregated Forecasts (Weighted Average)

When using "Aggregated" service:

1. **Multi-Source Combination:** Forecasts from NWS and Pirate Weather are combined
2. **Weighted Averaging:** Each source is weighted by:
   - Reliability (success rate)
   - Data recency (newer data preferred)
   - Response speed (faster sources preferred)
3. **Moving Average Smoothing:** Forecasts are smoothed over time to reduce noise
4. **Result:** More accurate, reliable forecasts

### Historical Data

The application stores historical forecasts automatically:
- Used to improve moving average calculations
- Enables better predictions over time
- Stored for 7 days (configurable in settings)
- No user action required

### Caching

Forecasts are cached automatically:
- **Instant Access:** Cached forecasts load immediately
- **Offline Capability:** Previously viewed forecasts available offline
- **Auto-Refresh:** Click "Refresh" to get latest forecasts
- **Cache Duration:** 1 hour per location

## Settings

Click the **"Settings"** button to access:

### Aggregation Settings
- Enable/disable weighted averaging
- Configure moving average window size
- Set moving average type (Simple/Exponential)

### Data Retention
- Set how many days of historical data to keep
- Default: 7 days

### API Keys
- Configure Pirate Weather API key (optional)
- Set in Settings or environment variable: `PIRATE_WEATHER_API_KEY`

## Troubleshooting

### No Forecast Data

1. **Check Internet Connection:** Application requires internet for API calls
2. **Verify Location:** Ensure coordinates are valid
3. **Try Different Service:** Switch from Aggregated to NWS (always works)
4. **Check Error Messages:** Click error dialog for details

### Slow Loading

1. **Use Cached Data:** Previously viewed locations load instantly
2. **Check Network Speed:** API calls take 3-5 seconds typically
3. **Try Simpler Service:** NWS may be faster than Aggregated
4. **Refresh:** Click Refresh button if data seems stale

### Incorrect Forecasts

1. **Use Aggregated:** Multiple sources are more accurate
2. **Check Location:** Verify coordinates are correct
3. **Refresh:** Get latest forecasts
4. **Compare Services:** Try different service to compare

### Application Crashes

1. **Check Logs:** See console output for error messages
2. **Database Issues:** Delete `~/.local/share/HyperlocalWeather/hyperlocal_weather.db` and restart
3. **Report Bug:** Include error message and steps to reproduce

## Tips for Best Results

1. **Use Aggregated Service:** Most accurate forecasts through multi-source consensus
2. **Save Favorite Locations:** Quick access to frequently checked locations
3. **Check Refresh:** Click Refresh for latest forecasts before important decisions
4. **Understand Probabilities:** Precipitation probability doesn't guarantee rain/snow
5. **Monitor Trends:** Historical data helps predict patterns

## Keyboard Shortcuts

- **Ctrl+R:** Refresh forecast
- **Ctrl+Q:** Quit application
- **Esc:** Close error dialogs

## Getting Help

- **Documentation:** See [README.md](../README.md) for technical details
- **Architecture:** See [docs/ARCHITECTURE.md](ARCHITECTURE.md) for system design
- **Issues:** Report bugs via GitHub Issues (if using GitHub)

---

**Enjoy accurate hyperlocal weather forecasts!**

