cmake_minimum_required(VERSION 3.21)
project(HyperlocalWeatherServices VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 with all required components
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Network 
    Sql 
    HttpServer
    Quick 
    Positioning 
    Location
)

# Enable Qt6 features
qt_standard_project_setup()

# API Gateway Service
qt_add_executable(ApiGatewayService
    ApiGatewayService.cpp
    ApiGatewayService.h
    ApiGatewayService_main.cpp
)
target_link_libraries(ApiGatewayService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)
qt_finalize_executable(ApiGatewayService)

# Weather Data Service
qt_add_executable(WeatherDataService
    WeatherDataService.cpp
    WeatherDataService.h
    WeatherDataService_main.cpp
)
target_link_libraries(WeatherDataService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)
qt_finalize_executable(WeatherDataService)

# ML Service
qt_add_executable(MLService
    MLService.cpp
    MLService.h
    MLService_main.cpp
)
target_link_libraries(MLService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)
qt_finalize_executable(MLService)

# Data Processing Service
qt_add_executable(DataProcessingService
    DataProcessingService.cpp
    DataProcessingService.h
    DataProcessingService_main.cpp
)
target_link_libraries(DataProcessingService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)
qt_finalize_executable(DataProcessingService)

# Location Service
qt_add_executable(LocationService
    LocationService.cpp
    LocationService.h
    LocationService_main.cpp
)
target_link_libraries(LocationService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)
qt_finalize_executable(LocationService)

# Alert Service
qt_add_executable(AlertService
    AlertService.cpp
    AlertService.h
    AlertService_main.cpp
)
target_link_libraries(AlertService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)
qt_finalize_executable(AlertService)

# Database Service
qt_add_executable(DatabaseService
    DatabaseService.cpp
    DatabaseService.h
    DatabaseService_main.cpp
)
target_link_libraries(DatabaseService
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
    Qt6::Sql
)
qt_finalize_executable(DatabaseService)

# Frontend Application
qt_add_executable(HyperlocalWeatherApp
    main.cpp
    weatherclient.cpp
    weatherclient.h
)

qt_add_qml_module(HyperlocalWeatherApp
    URI HyperlocalWeatherApp
    VERSION 1.0
    QML_FILES 
        main.qml
    SOURCES
        weatherclient.h
        weatherclient.cpp
)

qt_add_resources(HyperlocalWeatherApp "resources"
    PREFIX "/"
    FILES
        main.qml
)

target_link_libraries(HyperlocalWeatherApp 
    Qt6::Core 
    Qt6::Quick 
    Qt6::Network 
    Qt6::Positioning 
    Qt6::Location
)

qt_finalize_executable(HyperlocalWeatherApp)

# Installation targets
install(TARGETS 
    ApiGatewayService
    WeatherDataService
    MLService
    DataProcessingService
    LocationService
    AlertService
    DatabaseService
    HyperlocalWeatherApp
    DESTINATION bin
)

# Copy configuration files
install(FILES 
    settings.conf
    DESTINATION etc/hyperlocal-weather
)
