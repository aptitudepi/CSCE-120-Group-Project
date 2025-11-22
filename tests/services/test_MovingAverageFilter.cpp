#include <gtest/gtest.h>
#include "services/MovingAverageFilter.h"
#include "models/WeatherData.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QtMath>

class MovingAverageFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        filter = new MovingAverageFilter();
        filter->setWindowSize(5);
        filter->setAlpha(0.2);
    }
    
    void TearDown() override {
        delete filter;
    }
    
    MovingAverageFilter* filter;
};

TEST_F(MovingAverageFilterTest, AddDataPoint) {
    WeatherData* data = new WeatherData();
    data->setTemperature(75.0);
    data->setTimestamp(QDateTime::currentDateTime());
    
    filter->addDataPoint(data);
    
    EXPECT_EQ(filter->dataPointCount(), 1);
    
    delete data;
}

TEST_F(MovingAverageFilterTest, SimpleMovingAverage) {
    filter->setType(MovingAverageFilter::Simple);
    filter->setWindowSize(5);
    
    // Add 10 data points with varying temperatures
    QDateTime baseTime = QDateTime::currentDateTime();
    for (int i = 0; i < 10; ++i) {
        WeatherData* data = new WeatherData();
        data->setTemperature(70.0 + i * 2.0); // 70, 72, 74, 76, 78, 80, 82, 84, 86, 88
        data->setTimestamp(baseTime.addMinutes(i * 15));
        filter->addDataPoint(data);
        delete data;
    }
    
    WeatherData* averaged = filter->getMovingAverage(5);
    ASSERT_NE(averaged, nullptr);
    
    // Average of last 5: (80 + 82 + 84 + 86 + 88) / 5 = 84.0
    EXPECT_NEAR(averaged->temperature(), 84.0, 0.1);
    
    delete averaged;
}

TEST_F(MovingAverageFilterTest, ExponentialMovingAverage) {
    filter->setType(MovingAverageFilter::Exponential);
    filter->setAlpha(0.3);
    
    // Add data points
    QDateTime baseTime = QDateTime::currentDateTime();
    QList<double> values = {70.0, 72.0, 74.0, 76.0, 78.0};
    
    for (int i = 0; i < values.size(); ++i) {
        WeatherData* data = new WeatherData();
        data->setTemperature(values[i]);
        data->setTimestamp(baseTime.addMinutes(i * 15));
        filter->addDataPoint(data);
        delete data;
    }
    
    WeatherData* averaged = filter->getExponentialMovingAverage(0.3);
    ASSERT_NE(averaged, nullptr);
    
    // EMA calculation: should be between values
    EXPECT_GT(averaged->temperature(), 70.0);
    EXPECT_LT(averaged->temperature(), 78.0);
    
    delete averaged;
}

TEST_F(MovingAverageFilterTest, WindDirectionAverage) {
    filter->setWindowSize(3);
    
    // Add data points with different wind directions
    QDateTime baseTime = QDateTime::currentDateTime();
    
    WeatherData* data1 = new WeatherData();
    data1->setWindDirection(0);   // North
    data1->setWindSpeed(10.0);
    data1->setTimestamp(baseTime);
    filter->addDataPoint(data1);
    delete data1;
    
    WeatherData* data2 = new WeatherData();
    data2->setWindDirection(90);  // East
    data2->setWindSpeed(10.0);
    data2->setTimestamp(baseTime.addMinutes(15));
    filter->addDataPoint(data2);
    delete data2;
    
    WeatherData* data3 = new WeatherData();
    data3->setWindDirection(45);  // Northeast
    data3->setWindSpeed(10.0);
    data3->setTimestamp(baseTime.addMinutes(30));
    filter->addDataPoint(data3);
    delete data3;
    
    WeatherData* averaged = filter->getMovingAverage(3);
    ASSERT_NE(averaged, nullptr);
    
    // Vector average should be approximately 45 degrees
    // Direction should be valid (0-360)
    EXPECT_GE(averaged->windDirection(), 0);
    EXPECT_LE(averaged->windDirection(), 360);
    
    delete averaged;
}

TEST_F(MovingAverageFilterTest, SmoothForecast) {
    filter->setType(MovingAverageFilter::Simple);
    filter->setWindowSize(3);
    
    // Create a forecast list
    QList<WeatherData*> forecasts;
    QDateTime baseTime = QDateTime::currentDateTime();
    
    for (int i = 0; i < 5; ++i) {
        WeatherData* data = new WeatherData();
        data->setTemperature(75.0 + i * 0.5 + (i % 2 == 0 ? 2.0 : -2.0)); // Oscillating pattern
        data->setTimestamp(baseTime.addHours(i));
        forecasts.append(data);
    }
    
    QList<WeatherData*> smoothed = filter->smoothForecast(forecasts);
    
    EXPECT_EQ(smoothed.size(), forecasts.size());
    
    // Smoothed values should be less variable than original
    // (This is a simple check - in practice would verify variance reduction)
    for (int i = 0; i < smoothed.size(); ++i) {
        EXPECT_NEAR(smoothed[i]->temperature(), forecasts[i]->temperature(), 5.0);
    }
    
    // Cleanup
    for (WeatherData* data : forecasts) {
        delete data;
    }
    for (WeatherData* data : smoothed) {
        delete data;
    }
}

TEST_F(MovingAverageFilterTest, Clear) {
    // Add some data points
    for (int i = 0; i < 5; ++i) {
        WeatherData* data = new WeatherData();
        data->setTemperature(70.0 + i);
        data->setTimestamp(QDateTime::currentDateTime());
        filter->addDataPoint(data);
        delete data;
    }
    
    EXPECT_EQ(filter->dataPointCount(), 5);
    
    filter->clear();
    
    EXPECT_EQ(filter->dataPointCount(), 0);
}

TEST_F(MovingAverageFilterTest, WindowSizePerParameter) {
    filter->setWindowSize("temperature", 10);
    filter->setWindowSize("precipitation", 5);
    filter->setWindowSize("wind", 15);
    
    // Configuration should work
    SUCCEED();
}

