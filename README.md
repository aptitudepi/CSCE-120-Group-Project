# Hyperlocal Weather Forecasting System - C++/Qt Implementation

A comprehensive microservices-based weather forecasting system implemented entirely in **C++/Qt** that provides hyperlocal predictions with 1km resolution using machine learning and multiple data sources.

## ✅ Project Status: Ready to Build

**All source files are now complete and organized!**

- ✅ 7 backend microservices (with separated .h, .cpp, and main.cpp files)
- ✅ Qt/QML frontend application  
- ✅ CMake build configuration
- ✅ Build and deployment scripts
- ✅ Configuration files

**To build this project, you need Qt6 installed.** See [SETUP.md](SETUP.md) for detailed instructions.

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

## 🚀 Quick Start

### Prerequisites

Install Qt6 development packages:

**Ubuntu/Debian:**
```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-httpserver-dev qt6-positioning-dev qt6-location-dev cmake build-essential
```

**macOS:**
```bash
brew install qt@6 cmake
```

**Windows:**
- Download and install Qt6 from https://www.qt.io/download
- Download and install CMake from https://cmake.org/download/

### Build & Run

```bash
# Clone the repository
git clone https://github.com/aptitudepi/CSCE-120-Group-Project.git
cd CSCE-120-Group-Project

# Make scripts executable
chmod +x *.sh

# Build all services and frontend
./build.sh

# Start infrastructure (PostgreSQL, Redis)
docker-compose -f docker-compose-qt.yml up -d

# Deploy all microservices
./deploy-qt.sh

# Run the frontend application
cd build/frontend && ./HyperlocalWeatherApp

# Test the API
./test-api-qt.sh
```

## 📚 Documentation

- **[SETUP.md](SETUP.md)** - Detailed setup and installation guide
- **[README.md](README.md)** - This file (project overview)
- **settings.conf** - Configuration file with API keys and service URLs

## 🔑 Default Credentials (Testing Only)

- Username: `demo`
- Password: `demo123`

## 📁 Project Structure

```
.
├── *Service.h                # Service header files (7 services)
├── *Service.cpp             # Service implementations (7 services)  
├── *Service_main.cpp        # Service entry points (7 services)
├── main.cpp                 # Frontend application entry point
├── weatherclient.h/.cpp     # Frontend weather API client
├── main.qml                 # Frontend QML interface
├── CMakeLists_*.txt         # CMake configuration files
├── settings.conf            # Application configuration
├── build.sh                 # Build automation script
├── deploy-qt.sh             # Service deployment script
├── test-api-qt.sh           # API testing script
├── SETUP.md                 # Detailed setup guide
└── build/                   # Build output directory (created by build.sh)
```

## 🐛 Troubleshooting

**Qt6 not found?**
- Make sure Qt6 is installed and in your PATH
- Try: `export PATH="/usr/lib/qt6/bin:$PATH"` (Linux) or `export PATH="/usr/local/opt/qt@6/bin:$PATH"` (macOS)

**Build fails?**
- Ensure all Qt6 components are installed (see SETUP.md)
- Try a clean build: `rm -rf build && ./build.sh`

**Services won't start?**
- Check if ports 8000-8006 are already in use
- Verify Docker containers are running: `docker-compose -f docker-compose-qt.yml ps`

See [SETUP.md](SETUP.md) for more troubleshooting tips.

## 📄 License

See LICENSE file for details.

## 👥 Contributors

CSCE 120 - Texas A&M University Group Project

---

**Note**: This is a student project demonstrating a complete microservices architecture using C++/Qt6.

