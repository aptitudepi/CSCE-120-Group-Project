cmake_minimum_required(VERSION 3.21)
project(HyperlocalWeather VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages (6.4 minimum for Ubuntu 24.04)
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Network
    Sql
    Positioning
)

# Set Qt policies to use standard QML module paths
qt_policy(SET QTP0001 NEW)

# Provide helpful error message if Qt6 not found
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please install Qt6 development packages.")
    message(STATUS "On Ubuntu 24.04, run: sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev")
endif()

# Enable testing
enable_testing()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/models/WeatherData.cpp
    src/models/ForecastModel.cpp
    src/models/AlertModel.cpp
    src/services/WeatherService.cpp
    src/services/NWSService.cpp
    src/services/PirateWeatherService.cpp
    src/services/CacheManager.cpp
    src/services/WeatherAggregator.cpp
    src/services/PerformanceMonitor.cpp
    src/controllers/WeatherController.cpp
    src/controllers/AlertController.cpp
    src/database/DatabaseManager.cpp
    src/nowcast/NowcastEngine.cpp
)

set(HEADERS
    src/models/WeatherData.h
    src/models/ForecastModel.h
    src/models/AlertModel.h
    src/services/WeatherService.h
    src/services/NWSService.h
    src/services/PirateWeatherService.h
    src/services/CacheManager.h
    src/services/WeatherAggregator.h
    src/services/PerformanceMonitor.h
    src/controllers/WeatherController.h
    src/controllers/AlertController.h
    src/database/DatabaseManager.h
    src/nowcast/NowcastEngine.h
)

# Create executable first
qt6_add_executable(HyperlocalWeather
    ${SOURCES}
    ${HEADERS}
)

# Create QML module for proper type resolution
# Note: Executable output is set to bin/ directory to avoid conflicts
qt6_add_qml_module(HyperlocalWeather
    URI HyperlocalWeather
    VERSION 1.0
    QML_FILES
        src/qml/main.qml
        src/qml/pages/MainPage.qml
        src/qml/components/ForecastCard.qml
        src/qml/components/LocationInput.qml
        src/qml/components/AlertItem.qml
)

# Link libraries
target_link_libraries(HyperlocalWeather PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Network
    Qt6::Sql
    Qt6::Positioning
)

# Set target properties
set_target_properties(HyperlocalWeather PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


# Install target
install(TARGETS HyperlocalWeather
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Add subdirectory for tests
add_subdirectory(tests)
