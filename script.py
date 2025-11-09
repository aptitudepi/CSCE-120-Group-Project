
# Let me create a comprehensive structure for the Qt6 C++ desktop application
# Based on the requirements from the PDFs

project_structure = """
HyperlocalWeather/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── docs/
│   ├── SETUP.md
│   ├── BUILD.md
│   └── ARCHITECTURE.md
├── src/
│   ├── main.cpp
│   ├── models/
│   │   ├── WeatherData.h
│   │   ├── WeatherData.cpp
│   │   ├── ForecastModel.h
│   │   ├── ForecastModel.cpp
│   │   ├── AlertModel.h
│   │   └── AlertModel.cpp
│   ├── services/
│   │   ├── WeatherService.h
│   │   ├── WeatherService.cpp
│   │   ├── NWSService.h
│   │   ├── NWSService.cpp
│   │   ├── PirateWeatherService.h
│   │   ├── PirateWeatherService.cpp
│   │   ├── CacheManager.h
│   │   └── CacheManager.cpp
│   ├── controllers/
│   │   ├── WeatherController.h
│   │   ├── WeatherController.cpp
│   │   ├── AlertController.h
│   │   └── AlertController.cpp
│   ├── database/
│   │   ├── DatabaseManager.h
│   │   └── DatabaseManager.cpp
│   ├── nowcast/
│   │   ├── NowcastEngine.h
│   │   └── NowcastEngine.cpp
│   └── qml/
│       ├── main.qml
│       ├── components/
│       │   ├── ForecastCard.qml
│       │   ├── AlertItem.qml
│       │   └── LocationInput.qml
│       └── pages/
│           ├── MainPage.qml
│           └── SettingsPage.qml
├── tests/
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── models/
│   │   └── test_WeatherData.cpp
│   ├── services/
│   │   ├── test_WeatherService.cpp
│   │   ├── test_NWSService.cpp
│   │   └── test_CacheManager.cpp
│   └── integration/
│       └── test_EndToEnd.cpp
└── resources/
    └── app.qrc
"""

print("Project Structure:")
print(project_structure)

# Create key requirements summary
requirements = {
    "Frontend": "Qt6/QML with location input, map tile selection, forecast cards, alert settings",
    "Backend": "C++ services for API communication, parsing, caching, rate limiting",
    "Data Sources": "NWS API, Pirate Weather API (aggregator)",
    "Nowcasting": "Radar-interpolation based precipitation nowcast (0-90 min)",
    "Storage": "In-memory LRU cache + SQLite for user data",
    "Alerts": "Geofenced rules with threshold-based notifications",
    "Architecture": "MVC pattern separating QML/Qt/Database layers",
    "Testing": "Unit tests (80% coverage), Integration tests, Performance tests",
    "Success Metrics": {
        "Latency": "< 10 seconds for forecast delivery",
        "Accuracy": "> 75% precipitation hit rate",
        "Uptime": "> 95% service availability",
        "Alert Lead Time": "> 5 minutes median"
    }
}

print("\n\nKey Requirements:")
for key, value in requirements.items():
    print(f"- {key}: {value}")
