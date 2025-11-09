#include <gtest/gtest.h>
#include "models/WeatherData.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

class WeatherDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        data = new WeatherData();
    }
    
    void TearDown() override {
        delete data;
    }
    
    WeatherData* data;
};

TEST_F(WeatherDataTest, Initialization) {
    EXPECT_EQ(data->latitude(), 0.0);
    EXPECT_EQ(data->longitude(), 0.0);
    EXPECT_EQ(data->temperature(), 0.0);
    EXPECT_EQ(data->humidity(), 0);
}

TEST_F(WeatherDataTest, SettersAndGetters) {
    data->setLatitude(30.6272);
    data->setLongitude(-96.3344);
    data->setTemperature(75.5);
    data->setHumidity(60);
    
    EXPECT_DOUBLE_EQ(data->latitude(), 30.6272);
    EXPECT_DOUBLE_EQ(data->longitude(), -96.3344);
    EXPECT_DOUBLE_EQ(data->temperature(), 75.5);
    EXPECT_EQ(data->humidity(), 60);
}

TEST_F(WeatherDataTest, JsonSerialization) {
    data->setLatitude(30.6272);
    data->setLongitude(-96.3344);
    data->setTemperature(75.5);
    data->setHumidity(60);
    data->setTimestamp(QDateTime::currentDateTime());
    data->setWeatherCondition("Sunny");
    
    QJsonObject json = data->toJson();
    
    EXPECT_TRUE(json.contains("latitude"));
    EXPECT_TRUE(json.contains("longitude"));
    EXPECT_TRUE(json.contains("temperature"));
    EXPECT_TRUE(json.contains("humidity"));
    EXPECT_TRUE(json.contains("weatherCondition"));
    
    EXPECT_DOUBLE_EQ(json["latitude"].toDouble(), 30.6272);
    EXPECT_DOUBLE_EQ(json["longitude"].toDouble(), -96.3344);
    EXPECT_DOUBLE_EQ(json["temperature"].toDouble(), 75.5);
    EXPECT_EQ(json["humidity"].toInt(), 60);
}

TEST_F(WeatherDataTest, JsonDeserialization) {
    QJsonObject json;
    json["latitude"] = 30.6272;
    json["longitude"] = -96.3344;
    json["temperature"] = 75.5;
    json["humidity"] = 60;
    json["weatherCondition"] = "Sunny";
    json["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    WeatherData* deserialized = WeatherData::fromJson(json);
    
    EXPECT_NE(deserialized, nullptr);
    EXPECT_DOUBLE_EQ(deserialized->latitude(), 30.6272);
    EXPECT_DOUBLE_EQ(deserialized->longitude(), -96.3344);
    EXPECT_DOUBLE_EQ(deserialized->temperature(), 75.5);
    EXPECT_EQ(deserialized->humidity(), 60);
    
    delete deserialized;
}

TEST_F(WeatherDataTest, RoundTripSerialization) {
    data->setLatitude(30.6272);
    data->setLongitude(-96.3344);
    data->setTemperature(75.5);
    data->setHumidity(60);
    data->setPrecipProbability(0.3);
    data->setWindSpeed(10.5);
    
    QJsonObject json = data->toJson();
    WeatherData* deserialized = WeatherData::fromJson(json);
    
    EXPECT_NE(deserialized, nullptr);
    EXPECT_DOUBLE_EQ(deserialized->latitude(), data->latitude());
    EXPECT_DOUBLE_EQ(deserialized->longitude(), data->longitude());
    EXPECT_DOUBLE_EQ(deserialized->temperature(), data->temperature());
    EXPECT_EQ(deserialized->humidity(), data->humidity());
    EXPECT_DOUBLE_EQ(deserialized->precipProbability(), data->precipProbability());
    EXPECT_DOUBLE_EQ(deserialized->windSpeed(), data->windSpeed());
    
    delete deserialized;
}

