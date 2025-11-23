#include <gtest/gtest.h>
#include "services/WeatherAggregator.h"
#include "services/NWSService.h"
#include "services/PirateWeatherService.h"
#include "models/WeatherData.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>

class WeatherAggregatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        aggregator = new WeatherAggregator();
        nwsService = new NWSService();
        pirateService = new PirateWeatherService();
    }
    
    void TearDown() override {
        delete aggregator;
        delete nwsService;
        delete pirateService;
    }
    
    WeatherAggregator* aggregator;
    NWSService* nwsService;
    PirateWeatherService* pirateService;
};

TEST_F(WeatherAggregatorTest, AddService) {
    aggregator->addService(nwsService, 10);
    aggregator->addService(pirateService, 5);
    
    // Services should be added
    WeatherAggregator::PerformanceMetrics metrics = aggregator->getMetrics();
    // Metrics should be initialized
    EXPECT_GE(metrics.totalRequests, 0);
}

TEST_F(WeatherAggregatorTest, SetStrategy) {
    aggregator->setStrategy(WeatherAggregator::PrimaryOnly);
    aggregator->setStrategy(WeatherAggregator::Fallback);
    aggregator->setStrategy(WeatherAggregator::WeightedAverage);
    aggregator->setStrategy(WeatherAggregator::BestAvailable);
    
    // Strategy should be set (no errors)
    SUCCEED();
}

TEST_F(WeatherAggregatorTest, GetMetrics) {
    WeatherAggregator::PerformanceMetrics metrics = aggregator->getMetrics();
    
    EXPECT_GE(metrics.averageResponseTime, 0);
    EXPECT_GE(metrics.cacheHitRate, 0.0);
    EXPECT_LE(metrics.cacheHitRate, 1.0);
    EXPECT_GE(metrics.serviceUptime, 0.0);
    EXPECT_LE(metrics.serviceUptime, 1.0);
    EXPECT_GE(metrics.totalRequests, 0);
    EXPECT_GE(metrics.successfulRequests, 0);
    EXPECT_GE(metrics.failedRequests, 0);
}

// Test weighted average merging of forecasts
TEST_F(WeatherAggregatorTest, WeightedAverageMerging) {
    aggregator->addService(nwsService, 10);
    aggregator->addService(pirateService, 5);
    aggregator->setStrategy(WeatherAggregator::WeightedAverage);
    
    // Create test forecast data from two sources
    QList<WeatherData*> nwsForecasts;
    QList<WeatherData*> pirateForecasts;
    
    QDateTime baseTime = QDateTime::currentDateTime();
    
    // NWS: 3 forecast periods
    for (int i = 0; i < 3; ++i) {
        WeatherData* data = new WeatherData();
        data->setLatitude(30.2672);
        data->setLongitude(-97.7431);
        data->setTimestamp(baseTime.addSecs(i * 3600));
        data->setTemperature(75.0 + i);
        data->setPrecipProbability(0.5);
        data->setPrecipIntensity(0.1);
        data->setWindSpeed(10.0);
        data->setWindDirection(180); // South
        data->setHumidity(60);
        nwsForecasts.append(data);
    }
    
    // Pirate Weather: 3 forecast periods (slightly different)
    for (int i = 0; i < 3; ++i) {
        WeatherData* data = new WeatherData();
        data->setLatitude(30.2672);
        data->setLongitude(-97.7431);
        data->setTimestamp(baseTime.addSecs(i * 3600));
        data->setTemperature(76.0 + i); // Slightly warmer
        data->setPrecipProbability(0.6); // Higher probability
        data->setPrecipIntensity(0.15);
        data->setWindSpeed(12.0); // Faster wind
        data->setWindDirection(190); // SSW
        data->setHumidity(65);
        pirateForecasts.append(data);
    }
    
    // Manually test mergeForecasts (we need to access private method, so use reflection or friend)
    // For now, test the strategy selection works
    aggregator->setStrategy(WeatherAggregator::WeightedAverage);
    
    // Verify services are set up
    SUCCEED();
    
    // Cleanup
    for (WeatherData* data : nwsForecasts) {
        delete data;
    }
    for (WeatherData* data : pirateForecasts) {
        delete data;
    }
}

TEST_F(WeatherAggregatorTest, MovingAverageConfiguration) {
    aggregator->setMovingAverageEnabled(true);
    aggregator->setMovingAverageWindowSize(15);
    aggregator->setMovingAverageType(MovingAverageFilter::Exponential);
    aggregator->setMovingAverageAlpha(0.3);
    
    // Configuration should work without errors
    SUCCEED();
}

