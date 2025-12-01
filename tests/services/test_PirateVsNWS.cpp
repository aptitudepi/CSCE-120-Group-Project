#include <gtest/gtest.h>
#include "services/NWSService.h"
#include "services/PirateWeatherService.h"
#include "models/WeatherData.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <QDebug>

class PirateVsNWSTest : public ::testing::Test {
protected:
    void SetUp() override {
        nwsService = new NWSService();
        pirateService = new PirateWeatherService();
        
        // Check if API key is set
        if (qEnvironmentVariable("PIRATE_WEATHER_API_KEY").isEmpty()) {
            GTEST_SKIP() << "PIRATE_WEATHER_API_KEY not set. Skipping comparison tests.";
        }
    }
    
    void TearDown() override {
        delete nwsService;
        delete pirateService;
    }
    
    NWSService* nwsService;
    PirateWeatherService* pirateService;
};

// Integration test - requires network and API keys
TEST_F(PirateVsNWSTest, CompareForecasts) {
    // College Station, TX
    double lat = 30.6280;
    double lon = -96.3344;
    
    if (!pirateService->hasApiKey()) {
        QString key = qEnvironmentVariable("PIRATE_WEATHER_API_KEY");
        if (!key.isEmpty()) {
            pirateService->setApiKey(key);
        } else {
            GTEST_SKIP() << "Pirate Weather API key missing";
        }
    }
    
    // Fetch NWS
    QEventLoop nwsLoop;
    QList<WeatherData*> nwsData;
    bool nwsReceived = false;
    
    QObject::connect(nwsService, &NWSService::forecastReady, [&](QList<WeatherData*> data) {
        nwsData = data;
        nwsReceived = true;
        nwsLoop.quit();
    });
    
    QObject::connect(nwsService, &NWSService::error, [&](QString error) {
        qWarning() << "NWS service failed: " << error;
        nwsLoop.quit();
    });
    
    nwsService->fetchForecast(lat, lon);
    QTimer::singleShot(15000, &nwsLoop, &QEventLoop::quit); // 15 second timeout
    nwsLoop.exec();
    
    if (!nwsReceived) {
        GTEST_SKIP() << "NWS did not respond (network issue or timeout)";
    }
    ASSERT_FALSE(nwsData.isEmpty()) << "NWS returned no data";
    
    // Fetch Pirate Weather
    QEventLoop pirateLoop;
    QList<WeatherData*> pirateData;
    bool pirateReceived = false;
    
    QObject::connect(pirateService, &PirateWeatherService::forecastReady, [&](QList<WeatherData*> data) {
        pirateData = data;
        pirateReceived = true;
        pirateLoop.quit();
    });
    
    QObject::connect(pirateService, &PirateWeatherService::error, [&](QString error) {
        FAIL() << "Pirate Weather service failed: " << error.toStdString();
        pirateLoop.quit();
    });
    
    pirateService->fetchForecast(lat, lon);
    QTimer::singleShot(15000, &pirateLoop, &QEventLoop::quit);
    pirateLoop.exec();
    
    ASSERT_TRUE(pirateReceived) << "Pirate Weather did not respond";
    ASSERT_FALSE(pirateData.isEmpty()) << "Pirate Weather returned no data";
    
    // Compare first period
    WeatherData* nwsFirst = nwsData.first();
    WeatherData* pirateFirst = pirateData.first();
    
    double tempDiff = qAbs(nwsFirst->temperature() - pirateFirst->temperature());
    
    qInfo() << "\n=========================================";
    qInfo() << "ACCURACY REPORT (NWS vs Pirate Weather)";
    qInfo() << "Location: College Station, TX";
    qInfo() << "Time:" << nwsFirst->timestamp().toString();
    qInfo() << "NWS Temp:" << nwsFirst->temperature() << "F";
    qInfo() << "Pirate Temp:" << pirateFirst->temperature() << "F";
    qInfo() << "Difference:" << tempDiff << "F";
    qInfo() << "=========================================\n";
    
    // Warn if difference is large
    if (tempDiff > 10.0) {
        qWarning() << "Temperature difference > 10 degrees:" << tempDiff;
    }
    
    // Clean up
    qDeleteAll(nwsData);
    qDeleteAll(pirateData);
}
