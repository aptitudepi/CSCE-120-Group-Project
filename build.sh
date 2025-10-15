#!/bin/bash
# build.sh - Build script for C++/Qt Hyperlocal Weather Services

set -e

echo "🏗️  Building Hyperlocal Weather Forecasting System (C++/Qt)"
echo "============================================================"

# Check if Qt6 is installed
if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
    echo "❌ Qt6 not found. Please install Qt6 development packages:"
    echo "   Ubuntu/Debian: sudo apt install qt6-base-dev qt6-declarative-dev"
    echo "   macOS: brew install qt@6"
    echo "   Windows: Download from https://www.qt.io/download"
    exit 1
fi

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake:"
    echo "   Ubuntu/Debian: sudo apt install cmake"
    echo "   macOS: brew install cmake"
    echo "   Windows: Download from https://cmake.org/download/"
    exit 1
fi

# Create build directory
echo "📁 Creating build directory..."
mkdir -p build
cd build

# Create directory structure for services
echo "📁 Creating service directories..."
mkdir -p services/api-gateway
mkdir -p services/weather-data
mkdir -p services/ml-service
mkdir -p services/data-processing
mkdir -p services/location-service
mkdir -p services/alert-service
mkdir -p services/database-service
mkdir -p frontend
mkdir -p config

# Copy source files to appropriate directories
echo "📝 Organizing source files..."

# Copy service source files (user would need to extract from the .txt files)
echo "⚠️  Please extract the C++ source files from the generated .txt files:"
echo "   - ApiGatewayService_cpp.txt -> services/api-gateway/"
echo "   - WeatherDataService_cpp.txt -> services/weather-data/"
echo "   - MLService_cpp.txt -> services/ml-service/"
echo "   - DataProcessingService_cpp.txt -> services/data-processing/"
echo "   - LocationService_cpp.txt -> services/location-service/"
echo "   - AlertService_cpp.txt -> services/alert-service/"
echo "   - DatabaseService_cpp.txt -> services/database-service/"

# Copy CMakeLists files
echo "📝 Setting up CMake files..."
cp ../CMakeLists_master.txt ./CMakeLists.txt
cp ../CMakeLists_api_gateway.txt ./services/api-gateway/CMakeLists.txt
cp ../CMakeLists_weather_data.txt ./services/weather-data/CMakeLists.txt
cp ../CMakeLists_ml_service.txt ./services/ml-service/CMakeLists.txt
cp ../CMakeLists_data_processing.txt ./services/data-processing/CMakeLists.txt
cp ../CMakeLists_location_service.txt ./services/location-service/CMakeLists.txt
cp ../CMakeLists_alert_service.txt ./services/alert-service/CMakeLists.txt
cp ../CMakeLists_database_service.txt ./services/database-service/CMakeLists.txt
cp ../CMakeLists_frontend.txt ./frontend/CMakeLists.txt

# Copy frontend files
cp ../main.cpp ./frontend/
cp ../weatherclient.h ./frontend/
cp ../weatherclient.cpp ./frontend/
cp ../main.qml ./frontend/

# Configure with CMake
echo "⚙️  Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build all services
echo "🔨 Building all services..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "✅ Build completed successfully!"
echo ""
echo "📋 Built services:"
echo "   🔹 API Gateway Service"
echo "   🔹 Weather Data Service" 
echo "   🔹 ML Service"
echo "   🔹 Data Processing Service"
echo "   🔹 Location Service"
echo "   🔹 Alert Service"
echo "   🔹 Database Service"
echo "   🔹 Qt Frontend Application"
echo ""
echo "🚀 To run the services:"
echo "   1. Start infrastructure: docker-compose up -d postgres redis"
echo "   2. Run services manually or use: ./deploy-qt.sh"
echo "   3. Run frontend: ./frontend/HyperlocalWeatherApp"
