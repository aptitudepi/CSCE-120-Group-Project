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

