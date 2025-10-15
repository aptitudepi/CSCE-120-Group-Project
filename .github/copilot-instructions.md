# Copilot Instructions for Hyperlocal Weather Forecasting System

## Project Overview

This is a comprehensive microservices-based hyperlocal weather forecasting system implemented in **C++/Qt6** that provides hyperlocal predictions with 1km resolution using machine learning and multiple data sources.

## Technology Stack

- **Primary Language**: C++17
- **Framework**: Qt6 (QHttpServer, QNetworkAccessManager, QSql, QML)
- **Build System**: CMake with Qt6 integration
- **Database**: PostgreSQL with SQLite fallback
- **Cache**: Redis
- **Containerization**: Docker with multi-stage builds
- **Frontend**: Qt6/QML with declarative UI

## Architecture

### Microservices (8 total)

1. **API Gateway Service** (Port 8000) - Authentication, routing, and CORS handling
2. **Weather Data Service** (Port 8001) - Multi-source data collection (Pirate Weather, NWS, Open-Meteo)
3. **ML Service** (Port 8002) - LSTM-based hyperlocal prediction engine
4. **Data Processing Service** (Port 8005) - Multi-source data fusion and validation
5. **Location Service** (Port 8003) - Geocoding and location management
6. **Alert Service** (Port 8004) - Weather alerts and notifications
7. **Database Service** (Port 8006) - Data persistence and caching
8. **Qt Frontend Application** - Cross-platform desktop/mobile client

### Service Communication

- Services communicate via HTTP REST APIs using Qt's QHttpServer and QNetworkAccessManager
- JSON is used for data exchange (QJsonDocument, QJsonObject)
- API Gateway handles authentication using JWT tokens
- All services use Qt's signal/slot mechanism for async operations

## Coding Conventions

### C++ Style

- Use modern C++17 features
- Follow Qt naming conventions:
  - Classes: PascalCase (e.g., `ApiGatewayService`)
  - Member variables: m_camelCase prefix (e.g., `m_server`, `m_networkManager`)
  - Methods: camelCase (e.g., `handleRequest`, `createJsonResponse`)
  - Signals/slots: camelCase (e.g., `handleRequest`)
- Use Qt's parent-child memory management where appropriate
- Prefer Qt containers (QList, QMap, QString) over STL for Qt integration

### File Organization

- Each microservice has its own `.cpp` source file
- CMakeLists files are named with `CMakeLists_<service>.txt` pattern
- Build scripts use bash with clear error messages
- Configuration uses `settings.conf` file

### Qt Patterns

- Inherit from QObject for signal/slot support
- Use Q_OBJECT macro for meta-object system
- Implement proper parent-child relationships for memory management
- Use QSettings for configuration management
- Use QTimer for scheduled tasks
- Use QHttpServer for RESTful service endpoints

## Build and Development

### Building the Project

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install qt6-base-dev qt6-declarative-dev cmake

# Build all services
chmod +x *.sh
./build.sh
```

### Running Services

```bash
# Start infrastructure
docker-compose up -d postgres redis

# Deploy services
./deploy-qt.sh

# Test API
./test-api-qt.sh

# Run frontend
cd build/frontend && ./HyperlocalWeatherApp
```

### Directory Structure

```
.
├── <ServiceName>.cpp          # Service implementation files
├── CMakeLists_<service>.txt   # CMake configuration per service
├── CMakeLists_master.txt      # Master CMake configuration
├── build.sh                   # Build script
├── deploy-qt.sh              # Deployment script
├── test-api-qt.sh            # API testing script
├── docker-compose-qt.yml     # Docker compose configuration
├── settings.conf             # Application settings
└── weather_app_qml_example.qml  # QML frontend example
```

## Common Patterns

### Creating a QHttpServer Endpoint

```cpp
QHttpServerResponse handleEndpoint(const QHttpServerRequest &request) {
    QJsonObject response;
    response["status"] = "success";
    response["data"] = QJsonObject();
    return createJsonResponse(response, 200);
}
```

### Making HTTP Requests Between Services

```cpp
QNetworkRequest request(QUrl("http://localhost:8001/api/endpoint"));
request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
QNetworkReply *reply = m_networkManager->get(request);
```

### JSON Handling

```cpp
QJsonObject data;
data["key"] = "value";
QJsonDocument doc(data);
QByteArray jsonData = doc.toJson();
```

### Error Handling

- Use QHttpServerResponse with appropriate HTTP status codes (200, 400, 401, 404, 500)
- Always include descriptive error messages in JSON responses
- Log errors using Qt's logging facilities (qDebug(), qWarning(), qCritical())

## Testing

- Test scripts are provided in bash (test-api-qt.sh)
- API endpoints can be tested with curl or using Qt's QNetworkAccessManager
- Authentication requires demo/demo123 credentials for testing
- Health check endpoint: http://localhost:8000/health

## Security Considerations

- JWT tokens are used for authentication (QJwtToken)
- Passwords should be hashed using QCryptographicHash
- CORS headers are configured in API Gateway
- Sensitive data is stored in settings.conf (not committed to git)

## Dependencies

### External APIs

- Pirate Weather API
- National Weather Service (NWS) API
- Open-Meteo API

### Qt Modules Required

- Qt6::Core
- Qt6::Network
- Qt6::HttpServer
- Qt6::Sql
- Qt6::Qml
- Qt6::Quick

## Key Files

- `ApiGatewayService.cpp` - Entry point for all API requests
- `WeatherDataService.cpp` - Weather data collection from multiple sources
- `MLService.cpp` - Machine learning predictions
- `DatabaseService.cpp` - Data persistence layer
- `settings.conf` - Configuration file for all services

## Best Practices

1. Always inherit from QObject for services to enable signal/slot mechanism
2. Use Q_OBJECT macro in class declarations for meta-object features
3. Implement proper error handling with descriptive JSON error responses
4. Use Qt's parent-child memory management to avoid memory leaks
5. Configure CORS headers for web frontend compatibility
6. Use QSettings for configuration management
7. Follow async patterns with QNetworkAccessManager for HTTP requests
8. Implement proper logging with qDebug/qWarning/qCritical
9. Use CMake with Qt6 integration for consistent builds
10. Test services individually before integration

## When Making Changes

- Ensure all services compile with their respective CMakeLists files
- Test service endpoints using the provided test scripts
- Verify service communication through API Gateway
- Check that JSON responses follow the established format
- Ensure Qt signal/slot connections are properly set up
- Update CMakeLists files if adding new dependencies
- Test with Docker deployment for production readiness
