# Hyperlocal Weather Forecasting System - C++/Qt Implementation

A comprehensive microservices-based weather forecasting system implemented entirely in **C++/Qt** that provides hyperlocal predictions with 1km resolution using machine learning and multiple data sources.

## 🏗️ Architecture Overview

The system consists of **8 microservices** all implemented in **C++/Qt**:

### Core Services
1. **API Gateway Service** (Port 8000) - Authentication, routing, and CORS handling
2. **Weather Data Service** (Port 8001) - Multi-source data collection (Pirate Weather, NWS, Open-Meteo)  
3. **ML Service** (Port 8002) - LSTM-based hyperlocal prediction engine
4. **Data Processing Service** (Port 8005) - Multi-source data fusion and validation
5. **Location Service** (Port 8003) - Geocoding and location management
6. **Alert Service** (Port 8004) - Weather alerts and notifications
7. **Database Service** (Port 8006) - Data persistence and caching
8. **Qt Frontend Application** - Cross-platform desktop/mobile client

### Technology Stack
- **Backend**: C++17 with Qt6 (QHttpServer, QNetworkAccessManager, QSql)
- **Frontend**: Qt6/QML with declarative UI
- **Database**: PostgreSQL with SQLite fallback
- **Cache**: Redis (simulated in demo)
- **Containerization**: Docker with multi-stage builds
- **Build System**: CMake with Qt6 integration

## 🚀 Quick Deploy Commands

```bash
# Complete deployment in 3 steps:
git clone <repo> && cd hyperlocal-weather-qt
chmod +x *.sh && ./build.sh
./deploy-qt.sh && ./test-api-qt.sh

# Access the application:
# Frontend: cd build/frontend && ./HyperlocalWeatherApp  
# API: http://localhost:8000 (demo/demo123)
# Health: http://localhost:8000/health
```

For complete documentation, see the full README-QT.md file.
