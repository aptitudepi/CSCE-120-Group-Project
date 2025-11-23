#include <gtest/gtest.h>
#include "services/PirateWeatherService.h"
#include "models/WeatherData.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

class PirateWeatherServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = new PirateWeatherService();
        service->setApiKey("test_key");
    }
    
    void TearDown() override {
        delete service;
    }
    
    PirateWeatherService* service;
    
    void testParseForecastResponse(const QByteArray& data, double lat, double lon, bool hasMinuteReceivers = false) {
        service->parseForecastResponse(data, lat, lon, hasMinuteReceivers);
    }
};

TEST_F(PirateWeatherServiceTest, Initialization) {
    EXPECT_EQ(service->serviceName(), "PirateWeather");
    EXPECT_TRUE(service->isAvailable());
}

TEST_F(PirateWeatherServiceTest, ParseValidResponse) {
    // Construct a sample valid response
    QJsonObject current;
    current["time"] = 1620000000;
    current["temperature"] = 75.0;
    current["summary"] = "Clear";
    
    QJsonObject hourlyItem;
    hourlyItem["time"] = 1620000000;
    hourlyItem["temperature"] = 75.0;
    
    QJsonArray hourlyData;
    hourlyData.append(hourlyItem);
    
    QJsonObject hourly;
    hourly["data"] = hourlyData;
    
    QJsonObject root;
    root["currently"] = current;
    root["hourly"] = hourly;
    root["latitude"] = 30.0;
    root["longitude"] = -90.0;
    
    QJsonDocument doc(root);
    QByteArray json = doc.toJson();
    
    QSignalSpy spy(service, &WeatherService::forecastReady);
    QSignalSpy spyCurrent(service, &WeatherService::currentReady);
    
    testParseForecastResponse(json, 30.0, -90.0);
    
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spyCurrent.count(), 1);
    
    // Verify data
    QList<QVariant> arguments = spy.takeFirst();
    QList<WeatherData*> list = arguments.at(0).value<QList<WeatherData*>>();
    EXPECT_FALSE(list.isEmpty());
    EXPECT_EQ(list.first()->temperature(), 75.0);
    
    // Cleanup
    qDeleteAll(list);
}

TEST_F(PirateWeatherServiceTest, ParseEmptyResponse) {
    QByteArray json = "{}";
    QSignalSpy spy(service, &WeatherService::forecastReady);
    
    testParseForecastResponse(json, 30.0, -90.0);
    
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(PirateWeatherServiceTest, ParseMalformedJson) {
    QByteArray json = "{invalid";
    QSignalSpy spy(service, &WeatherService::error);
    
    testParseForecastResponse(json, 30.0, -90.0);
    
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(PirateWeatherServiceTest, ParseMissingFields) {
    // Response with missing standard fields
    QJsonObject root;
    QJsonObject current; // Empty current
    root["currently"] = current;
    
    QJsonDocument doc(root);
    QByteArray json = doc.toJson();
    
    QSignalSpy spyCurrent(service, &WeatherService::currentReady);
    
    testParseForecastResponse(json, 30.0, -90.0);
    
    // Should still emit current ready but with default values
    EXPECT_EQ(spyCurrent.count(), 1);
    
    QList<QVariant> arguments = spyCurrent.takeFirst();
    WeatherData* data = arguments.at(0).value<WeatherData*>();
    EXPECT_NE(data, nullptr);
    EXPECT_EQ(data->temperature(), 0.0); // Default
}
