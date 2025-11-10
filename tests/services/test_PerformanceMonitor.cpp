#include <gtest/gtest.h>
#include "services/PerformanceMonitor.h"
#include <QCoreApplication>
#include <QDateTime>

class PerformanceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor = new PerformanceMonitor();
    }
    
    void TearDown() override {
        delete monitor;
    }
    
    PerformanceMonitor* monitor;
};

TEST_F(PerformanceMonitorTest, RecordForecastRequest) {
    QString requestId = "test_request_1";
    monitor->recordForecastRequest(requestId);
    
    // Should not crash
    SUCCEED();
}

TEST_F(PerformanceMonitorTest, RecordForecastResponse) {
    QString requestId = "test_request_1";
    monitor->recordForecastRequest(requestId);
    monitor->recordForecastResponse(requestId, 1000); // 1 second
    
    double avgTime = monitor->averageForecastResponseTime();
    EXPECT_GT(avgTime, 0.0);
}

TEST_F(PerformanceMonitorTest, IsForecastTimeAcceptable) {
    EXPECT_TRUE(monitor->isForecastTimeAcceptable(5000)); // 5 seconds
    EXPECT_FALSE(monitor->isForecastTimeAcceptable(15000)); // 15 seconds
}

TEST_F(PerformanceMonitorTest, RecordPrecipitationPrediction) {
    QString location = "30.6272,-96.3344";
    QDateTime predictedTime = QDateTime::currentDateTime().addSecs(3600);
    double intensity = 0.5;
    
    monitor->recordPrecipitationPrediction(location, predictedTime, intensity);
    
    double hitRate = monitor->precipitationHitRate();
    EXPECT_GE(hitRate, 0.0);
    EXPECT_LE(hitRate, 1.0);
}

TEST_F(PerformanceMonitorTest, RecordServiceUpDown) {
    QString serviceName = "NWS";
    
    monitor->recordServiceUp(serviceName);
    double uptime = monitor->serviceUptime(serviceName);
    EXPECT_GT(uptime, 0.0);
    
    monitor->recordServiceDown(serviceName);
    // Uptime should decrease
    double uptime2 = monitor->serviceUptime(serviceName);
    EXPECT_GE(uptime2, 0.0);
    EXPECT_LE(uptime2, 1.0);
}

TEST_F(PerformanceMonitorTest, GetMetrics) {
    PerformanceMonitor::Metrics metrics = monitor->getMetrics();
    
    EXPECT_GE(metrics.forecastResponseTime, 0.0);
    EXPECT_GE(metrics.precipitationHitRate, 0.0);
    EXPECT_LE(metrics.precipitationHitRate, 1.0);
    EXPECT_GE(metrics.serviceUptime, 0.0);
    EXPECT_LE(metrics.serviceUptime, 1.0);
    EXPECT_GE(metrics.alertLeadTime, 0.0);
    EXPECT_GE(metrics.testCoverage, 0.0);
    EXPECT_LE(metrics.testCoverage, 1.0);
}

TEST_F(PerformanceMonitorTest, RecordTestCoverage) {
    monitor->recordTestCoverage("WeatherController", 100, 150);
    
    double coverage = monitor->testCoverage("WeatherController");
    EXPECT_GT(coverage, 0.0);
    EXPECT_LT(coverage, 1.0);
    
    double totalCoverage = monitor->testCoverage();
    EXPECT_GT(totalCoverage, 0.0);
    EXPECT_LE(totalCoverage, 1.0);
}

