# Hyperlocal Weather Forecasting System - Setup Guide

## Project Overview

A comprehensive microservices-based weather forecasting system implemented in **C++/Qt6** that provides hyperlocal predictions with 1km resolution using machine learning and multiple data sources.

## 📁 Project Structure

```
CSCE-120-Group-Project/
├── services/               # Backend microservices (7 services)
│   ├── ApiGatewayService.cpp/.h/_main.cpp
│   ├── WeatherDataService.cpp/.h/_main.cpp
│   ├── MLService.cpp/.h/_main.cpp
│   ├── DataProcessingService.cpp/.h/_main.cpp
│   ├── LocationService.cpp/.h/_main.cpp
│   ├── AlertService.cpp/.h/_main.cpp
│   └── DatabaseService.cpp/.h/_main.cpp
├── frontend/              # Qt/QML desktop application
│   ├── main.cpp
│   ├── main.qml
│   ├── weatherclient.cpp/.h
├── build/                 # Build artifacts (created by build.sh)
├── CMakeLists_*.txt       # CMake configuration files
├── settings.conf          # Application configuration
├── build.sh              # Build automation script
├── deploy-qt.sh          # Deployment script
├── test-api-qt.sh        # API testing script
└── README.md             # Project documentation

```

## 🔧 Prerequisites

### Required Software

1. **Qt6** (version 6.2 or higher)
   - Ubuntu/Debian:
     ```bash
     sudo apt update
     sudo apt install qt6-base-dev qt6-declarative-dev qt6-httpserver-dev
     sudo apt install qt6-positioning-dev qt6-location-dev
     ```
   - macOS:
     ```bash
     brew install qt@6
     ```
   - Windows:
     - Download from https://www.qt.io/download
     - Install Qt Creator and Qt 6.x

2. **CMake** (version 3.21 or higher)
   - Ubuntu/Debian:
     ```bash
     sudo apt install cmake
     ```
   - macOS:
     ```bash
     brew install cmake
     ```
   - Windows:
     - Download from https://cmake.org/download/

3. **C++ Compiler** (C++17 support required)
   - Ubuntu/Debian: `sudo apt install build-essential`
   - macOS: Xcode Command Line Tools
   - Windows: Visual Studio 2019 or later

4. **Docker & Docker Compose** (for infrastructure services)
   - PostgreSQL database
   - Redis cache
   
   Install from: https://docs.docker.com/get-docker/

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/aptitudepi/CSCE-120-Group-Project.git
cd CSCE-120-Group-Project
```

### 2. Make Scripts Executable

```bash
chmod +x *.sh
```

### 3. Build the Project

#### Option A: Using the Build Script (Recommended)

```bash
./build.sh
```

This script will:
- Check for Qt6 and CMake installation
- Create the build directory structure
- Copy all source files to appropriate locations
- Run CMake configuration
- Compile all services and the frontend application

#### Option B: Standard CMake Build

For IDE integration or standard CMake workflows:

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Executables will be in the build directory
ls -la *Service HyperlocalWeatherApp
```

### 4. Start Infrastructure Services

```bash
docker-compose -f docker-compose-qt.yml up -d
```

This starts:
- PostgreSQL database on port 5432
- Redis cache on port 6379

### 5. Deploy Microservices

```bash
./deploy-qt.sh
```

This script starts all backend services:
- API Gateway (port 8000)
- Weather Data Service (port 8001)
- ML Service (port 8002)
- Location Service (port 8003)
- Alert Service (port 8004)
- Data Processing Service (port 8005)
- Database Service (port 8006)

### 6. Run the Frontend Application

```bash
cd build/frontend
./HyperlocalWeatherApp
```

### 7. Test the API (Optional)

```bash
./test-api-qt.sh
```

## 🏗️ Architecture

### Microservices

1. **API Gateway Service** (Port 8000)
   - Authentication (JWT)
   - Request routing
   - CORS handling
   - Rate limiting

2. **Weather Data Service** (Port 8001)
   - Multi-source data collection
   - Pirate Weather API integration
   - NWS integration
   - Open-Meteo integration

3. **ML Service** (Port 8002)
   - LSTM-based prediction engine
   - Hyperlocal forecasting (1km resolution)
   - Model training and inference

4. **Data Processing Service** (Port 8005)
   - Multi-source data fusion
   - Data validation and cleaning
   - Quality assurance

5. **Location Service** (Port 8003)
   - Geocoding
   - Reverse geocoding
   - Location management

6. **Alert Service** (Port 8004)
   - Weather alerts
   - Notifications
   - Alert severity levels

7. **Database Service** (Port 8006)
   - Data persistence
   - Caching layer
   - Query optimization

### Frontend (Qt/QML)

- Cross-platform desktop application
- Real-time weather display
- Interactive map with hyperlocal visualization
- GPS integration
- Alert notifications

## ⚙️ Configuration

Edit `settings.conf` to customize:

```ini
[database]
postgresql_url=postgresql://postgres:password@localhost:5432/weather_db
redis_url=redis://localhost:6379

[api_keys]
pirate_weather=your_pirate_weather_api_key_here

[jwt]
secret=your-super-secret-jwt-key-change-this-in-production

[services]
weather_data=http://localhost:8001
ml=http://localhost:8002
location=http://localhost:8003
alert=http://localhost:8004
data_processing=http://localhost:8005
database=http://localhost:8006

[app]
name=Hyperlocal Weather Forecasting System
version=1.0.0
debug=true
```

## 🧪 Testing

### Manual API Testing

```bash
# Login to get auth token
curl -X POST http://localhost:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"demo","password":"demo123"}'

# Get current weather (replace TOKEN with actual token)
curl -X GET "http://localhost:8000/api/weather/current?latitude=30.6280&longitude=-96.3344" \
  -H "Authorization: Bearer TOKEN"

# Generate hyperlocal forecast
curl -X POST http://localhost:8000/api/forecast/hyperlocal \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer TOKEN" \
  -d '{"latitude":30.6280,"longitude":-96.3344,"resolution_km":1,"hours_ahead":24}'
```

### Automated Testing

Run the test script:
```bash
./test-api-qt.sh
```

## 📦 Build Artifacts

After building, you'll find:

```
build/
├── services/
│   ├── api-gateway/ApiGatewayService
│   ├── weather-data/WeatherDataService
│   ├── ml-service/MLService
│   ├── data-processing/DataProcessingService
│   ├── location-service/LocationService
│   ├── alert-service/AlertService
│   └── database-service/DatabaseService
└── frontend/HyperlocalWeatherApp
```

## 🐛 Troubleshooting

### Qt6 Not Found

If the build script reports Qt6 is not found:
- Verify Qt6 is installed: `qmake6 --version` or `qmake --version`
- Add Qt6 to PATH:
  ```bash
  export PATH="/usr/lib/qt6/bin:$PATH"  # Linux
  export PATH="/usr/local/opt/qt@6/bin:$PATH"  # macOS
  ```

### CMake Configuration Fails

If CMake can't find Qt6:
```bash
export Qt6_DIR="/path/to/qt6/lib/cmake/Qt6"
```

### Build Errors

1. Ensure all dependencies are installed
2. Try a clean build:
   ```bash
   rm -rf build
   ./build.sh
   ```

### Services Won't Start

1. Check if ports are already in use:
   ```bash
   netstat -tuln | grep -E '8000|8001|8002|8003|8004|8005|8006'
   ```
2. Verify infrastructure services are running:
   ```bash
   docker-compose -f docker-compose-qt.yml ps
   ```

### Frontend Won't Run

1. Check for missing QML plugins:
   ```bash
   export QML_IMPORT_PATH="/path/to/qt6/qml"
   ```
2. Run with debug output:
   ```bash
   QT_LOGGING_RULES="*=true" ./build/frontend/HyperlocalWeatherApp
   ```

## 📚 Technology Stack

- **Backend**: C++17 with Qt6
  - QHttpServer - HTTP server framework
  - QNetworkAccessManager - HTTP client
  - QSql - Database connectivity
  - QSettings - Configuration management
  
- **Frontend**: Qt6/QML
  - QtQuick - UI framework
  - QtLocation - Maps and geocoding
  - QtPositioning - GPS integration
  
- **Database**: PostgreSQL with SQLite fallback
- **Cache**: Redis
- **Build**: CMake 3.21+
- **Containerization**: Docker & Docker Compose

## 🔒 Security Notes

- Change default JWT secret in production
- Use HTTPS for production deployments
- Store API keys securely (use environment variables)
- Implement proper authentication in production

## 👥 Default Credentials

For testing purposes:
- Username: `demo`
- Password: `demo123`

**Change these in production!**

## 📄 License

See LICENSE file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📞 Support

For issues and questions:
- GitHub Issues: https://github.com/aptitudepi/CSCE-120-Group-Project/issues
- Course: CSCE 120 - Texas A&M University

---

**Note**: This is a student project for CSCE 120 at Texas A&M University.
