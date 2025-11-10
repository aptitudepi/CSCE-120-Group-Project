cmake_minimum_required(VERSION 3.21)

# Prevent CMake from finding conda/miniconda libraries
# This is critical to avoid library conflicts (conda's libstdc++ is incompatible with Qt6)
if(DEFINED ENV{CONDA_PREFIX})
    # Remove conda from CMAKE_PREFIX_PATH to prevent finding conda libraries
    list(REMOVE_ITEM CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
    list(REMOVE_ITEM CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}/lib/cmake")
    
    # Also remove from library paths
    list(REMOVE_ITEM CMAKE_LIBRARY_PATH "$ENV{CONDA_PREFIX}/lib")
    
    # Clear any cached GTest variables that might point to conda
    unset(GTest_DIR CACHE)
    unset(GTest_FOUND CACHE)
    unset(GTEST_FOUND CACHE)
    unset(GTEST_LIBRARIES CACHE)
    unset(GTEST_MAIN_LIBRARIES CACHE)
    unset(GTEST_INCLUDE_DIRS CACHE)
    
    message(STATUS "Excluded conda from CMAKE_PREFIX_PATH and cleared GTest cache to avoid library conflicts")
endif()

# Always use FetchContent for GoogleTest to avoid library conflicts with conda/miniconda
# This ensures we use a version that's compatible with system Qt6 and libstdc++
# CRITICAL: Do NOT use find_package - it will find conda's version!
include(FetchContent)

# Set FetchContent options to ensure it builds with system libraries
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    OVERRIDE_FIND_PACKAGE  # This ensures FetchContent is used even if find_package was called
)

# Make available - this will build GTest from source with system libraries
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Test sources
set(TEST_SOURCES
    test_main.cpp
    models/test_WeatherData.cpp
    services/test_CacheManager.cpp
    services/test_NWSService.cpp
    services/test_WeatherAggregator.cpp
    services/test_PerformanceMonitor.cpp
    integration/test_EndToEnd.cpp
)

# Create test executable
add_executable(HyperlocalWeatherTests ${TEST_SOURCES})

# Link test executable with application code and Google Test
target_link_libraries(HyperlocalWeatherTests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    GTest::gtest
    GTest::gtest_main
)

# Configure RPATH to avoid conda/miniconda library conflicts
# The issue: conda's libstdc++ is older and incompatible with Qt6
# Solution: Explicitly set RPATH via linker flags to system library paths only
set_target_properties(HyperlocalWeatherTests PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH FALSE
    SKIP_BUILD_RPATH TRUE  # Skip automatic RPATH, we'll set it manually
)

# Always set explicit RPATH to system libraries (helps even without conda)
# If conda is present, this ensures conda paths are not used
# Note: We must add these AFTER target_link_libraries to override any conda paths
target_link_options(HyperlocalWeatherTests PRIVATE
    -Wl,-rpath,/usr/lib/x86_64-linux-gnu
    -Wl,-rpath,/lib/x86_64-linux-gnu
)

# If conda is present, use patchelf to remove conda paths from RPATH after linking
if(DEFINED ENV{CONDA_PREFIX})
    message(STATUS "Conda detected. Will configure test executable to use system libraries only (excluding conda)")
    message(STATUS "RPATH set to: /usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu")
    
    # Try to use patchelf to fix RPATH after linking (removes conda paths)
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(PATCHELF_EXECUTABLE)
        # Remove conda paths from RPATH and set it to system libraries only
        add_custom_command(TARGET HyperlocalWeatherTests POST_BUILD
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu" $<TARGET_FILE:HyperlocalWeatherTests>
            COMMENT "Removing conda paths from RPATH using patchelf"
        )
        message(STATUS "Will use patchelf to fix RPATH after linking")
    else()
        message(WARNING "patchelf not found. Install it with: sudo apt install patchelf")
        message(WARNING "Without patchelf, you may need to run tests with: LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu")
    endif()
endif()

# Include application source directories
target_include_directories(HyperlocalWeatherTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Add application sources to test (except main.cpp)
target_sources(HyperlocalWeatherTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/models/WeatherData.cpp
    ${CMAKE_SOURCE_DIR}/src/models/ForecastModel.cpp
    ${CMAKE_SOURCE_DIR}/src/models/AlertModel.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WeatherService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/NWSService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PirateWeatherService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CacheManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WeatherAggregator.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PerformanceMonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/controllers/WeatherController.cpp
    ${CMAKE_SOURCE_DIR}/src/controllers/AlertController.cpp
    ${CMAKE_SOURCE_DIR}/src/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/nowcast/NowcastEngine.cpp
)

# Enable automoc for test sources
set_target_properties(HyperlocalWeatherTests PROPERTIES
    AUTOMOC ON
)

# Discover tests for CTest
include(GoogleTest)

# If conda is present and patchelf is not available, modify test discovery to use wrapper
if(DEFINED ENV{CONDA_PREFIX})
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(NOT PATCHELF_EXECUTABLE)
        # Set environment variables for test execution to use system libraries
        # This ensures system libstdc++ is used even if RPATH includes conda
        set_property(TARGET HyperlocalWeatherTests PROPERTY
            ENVIRONMENT_MODIFICATION
            "LD_LIBRARY_PATH=path_list_prepend:/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu"
        )
        message(STATUS "Configured test environment to prefer system libraries")
    endif()
endif()

gtest_discover_tests(HyperlocalWeatherTests)

# Add custom test target
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS HyperlocalWeatherTests
    COMMENT "Running tests..."
)

# Coverage target (optional, requires gcov/lcov)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(LCOV_PATH AND GENHTML_PATH)
        target_compile_options(HyperlocalWeatherTests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_options(HyperlocalWeatherTests PRIVATE --coverage)
        
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR} --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' --output-file coverage.info
            COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS check
            COMMENT "Generating coverage report..."
        )
    endif()
endif()

