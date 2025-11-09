# Hyperlocal Weather Forecasting Application

A Qt6/QML desktop application for hyperlocal weather forecasting with precipitation nowcasting, multi-source data aggregation, and geofenced alerts.

## Team: MaroonPtrs
- Viet Hoang Pham
- Devkumar Banerjee
- Asvath Madhan

## Features

- **Hyperlocal Forecasts**: Weather predictions down to 1km resolution
- **Multi-Source Data**: Integrates NWS API, Pirate Weather, and other sources
- **Precipitation Nowcasting**: 0-90 minute precipitation forecasts
- **Geofenced Alerts**: Location-based weather alerts with configurable thresholds
- **Offline Caching**: LRU cache + SQLite for fast access and offline capability
- **MVC Architecture**: Clean separation of UI, business logic, and data layers

## Requirements

### System Requirements
- Ubuntu 24.04 (or compatible Linux distribution)
- Qt 6.4 or higher (6.5+ recommended)
- CMake 3.21 or higher
- C++17 compatible compiler (GCC 11+ or Clang 14+)
- SQLite 3.x
- Internet connection for weather data APIs
- X server (VcXsrv or X410) for GUI on WSL, or use headless mode

### Dependencies
- Qt6 Core, Gui, Quick, Network, Sql, Positioning
- Google Test (for testing)

## Quick Start

**For Ubuntu 24.04 WSL:**

1. Install dependencies (see [SETUP.md](docs/SETUP.md))
2. Build: `mkdir build && cd build && cmake -G Ninja .. && ninja`
3. Run: `./HyperlocalWeather`

See detailed instructions in:
- [QUICKSTART.md](QUICKSTART.md) - Quick setup guide
- [docs/SETUP.md](docs/SETUP.md) - Environment setup and dependencies
- [docs/BUILD.md](docs/BUILD.md) - Building and running the application
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - System design and architecture

## Project Structure

```
HyperlocalWeather/
├── src/              # Source code
│   ├── models/       # Data models
│   ├── services/     # Weather API services
│   ├── controllers/  # Application controllers
│   ├── database/     # Database management
│   ├── nowcast/      # Nowcasting engine
│   └── qml/          # QML UI components
├── tests/            # Test suite
├── docs/             # Documentation
└── resources/        # Application resources
```

## Success Metrics

- **Latency**: < 10 seconds for forecast delivery
- **Accuracy**: > 75% precipitation hit rate
- **Uptime**: > 95% service availability
- **Alert Lead Time**: > 5 minutes median
- **Test Coverage**: > 80% on critical backend modules

## License

Open source project for educational purposes.

## API Keys

The application requires API keys for:
- Pirate Weather API (optional, for enhanced forecasts)
- Other weather services (optional)

Configure API keys in the application settings or environment variables.
