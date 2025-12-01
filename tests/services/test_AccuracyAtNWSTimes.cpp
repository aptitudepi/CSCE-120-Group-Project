#include <gtest/gtest.h>
#include "services/NWSService.h"
#include "services/PirateWeatherService.h"
#include "models/WeatherData.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <cmath>

class AccuracyAtNWSTimesTest : public ::testing::Test {
protected:
    void SetUp() override {
        nwsService = new NWSService();
        pirateService = new PirateWeatherService();
        
        // Check if API key is set
        if (qEnvironmentVariable("PIRATE_WEATHER_API_KEY").isEmpty()) {
            GTEST_SKIP() << "PIRATE_WEATHER_API_KEY not set. Skipping accuracy tests.";
        }
    }
    
    void TearDown() override {
        delete nwsService;
        delete pirateService;
    }
    
    // Helper to find NWS forecast period closest to target time (6am or 6pm)
    WeatherData* findClosestToTime(const QList<WeatherData*>& data, const QDateTime& targetTime) {
        if (data.isEmpty()) return nullptr;
        
        WeatherData* closest = nullptr;
        qint64 minDiff = LLONG_MAX;
        
        for (WeatherData* item : data) {
            qint64 diff = qAbs(item->timestamp().secsTo(targetTime));
            if (diff < minDiff) {
                minDiff = diff;
                closest = item;
            }
        }
        
        return closest;
    }
    
    // Helper to get next 6am or 6pm
    QDateTime getNextNwsUpdateTime() {
        QDateTime now = QDateTime::currentDateTime();
        QTime targetTime6am(6, 0, 0);
        QTime targetTime6pm(18, 0, 0);
        
        QDateTime next6am(now.date(), targetTime6am);
        QDateTime next6pm(now.date(), targetTime6pm);
        
        // If current time is past 6am today, next 6am is tomorrow
        if (now.time() > targetTime6am) {
            next6am = next6am.addDays(1);
        }
        
        // If current time is past 6pm today, next 6pm is tomorrow
        if (now.time() > targetTime6pm) {
            next6pm = next6pm.addDays(1);
        }
        
        // Return whichever is closer
        if (qAbs(now.secsTo(next6am)) < qAbs(now.secsTo(next6pm))) {
            return next6am;
        } else {
            return next6pm;
        }
    }
    
    NWSService* nwsService;
    PirateWeatherService* pirateService;
};

// Test accuracy at NWS update times (6am/6pm)
TEST_F(AccuracyAtNWSTimesTest, CompareAtNextNwsUpdateTime) {
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
    
    // Get next NWS update time (6am or 6pm)
    QDateTime targetTime = getNextNwsUpdateTime();
    
    qInfo() << "\n=========================================";
    qInfo() << "ACCURACY TEST AT NWS UPDATE TIME";
    qInfo() << "Target NWS update time:" << targetTime.toString();
    qInfo() << "Location: College Station, TX";
    qInfo() << "=========================================\n";
    
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
    QTimer::singleShot(20000, &nwsLoop, &QEventLoop::quit); // 20 second timeout
    nwsLoop.exec();
    
    if (!nwsReceived || nwsData.isEmpty()) {
        GTEST_SKIP() << "NWS did not respond or returned no data";
    }
    
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
    QTimer::singleShot(20000, &pirateLoop, &QEventLoop::quit);
    pirateLoop.exec();
    
    ASSERT_TRUE(pirateReceived) << "Pirate Weather did not respond";
    ASSERT_FALSE(pirateData.isEmpty()) << "Pirate Weather returned no data";
    
    // Find forecasts closest to target time
    WeatherData* nwsMatch = findClosestToTime(nwsData, targetTime);
    WeatherData* pirateMatch = findClosestToTime(pirateData, targetTime);
    
    ASSERT_NE(nwsMatch, nullptr) << "No NWS forecast found near target time";
    ASSERT_NE(pirateMatch, nullptr) << "No Pirate Weather forecast found near target time";
    
    // Calculate differences
    double tempDiff = qAbs(nwsMatch->temperature() - pirateMatch->temperature());
    double timeDiff = qAbs(nwsMatch->timestamp().secsTo(targetTime));
    double timeDiffPirate = qAbs(pirateMatch->timestamp().secsTo(targetTime));
    
    // Calculate percentage difference relative to NWS temperature
    double nwsTemp = nwsMatch->temperature();
    double percentDiff = (nwsTemp != 0.0) ? (tempDiff / qAbs(nwsTemp)) * 100.0 : 0.0;
    const double THRESHOLD_PERCENT = 30.0; // 30% threshold
    
    // Accuracy report
    qInfo() << "\n=========================================";
    qInfo() << "ACCURACY REPORT AT NWS UPDATE TIME";
    qInfo() << "Target time:" << targetTime.toString();
    qInfo() << "NWS forecast time:" << nwsMatch->timestamp().toString() 
            << "(diff: " << (timeDiff / 3600.0) << " hours)";
    qInfo() << "Pirate forecast time:" << pirateMatch->timestamp().toString()
            << "(diff: " << (timeDiffPirate / 3600.0) << " hours)";
    qInfo() << "NWS Temperature:" << nwsTemp << "F";
    qInfo() << "Pirate Temperature:" << pirateMatch->temperature() << "F";
    qInfo() << "Temperature Difference:" << tempDiff << "F";
    qInfo() << "Percentage Difference:" << QString::number(percentDiff, 'f', 2) << "%";
    qInfo() << "Threshold:" << THRESHOLD_PERCENT << "%";
    qInfo() << "=========================================\n";
    
    // Calculate accuracy metrics
    double tempAccuracy = (tempDiff < 5.0) ? 100.0 - (tempDiff / 5.0 * 100.0) : 0.0;
    tempAccuracy = qBound(0.0, tempAccuracy, 100.0);
    
    qInfo() << "Temperature Accuracy:" << QString::number(tempAccuracy, 'f', 2) << "%";
    qInfo() << "(Accuracy calculated as 100% if diff < 5°F, decreasing linearly)";
    
    // Test passes if temperature difference is within 30% threshold
    EXPECT_LE(percentDiff, THRESHOLD_PERCENT) 
        << "Temperature difference exceeds 30% threshold. "
        << "Difference: " << percentDiff << "%, NWS: " << nwsTemp << "F, "
        << "Pirate: " << pirateMatch->temperature() << "F, "
        << "Absolute diff: " << tempDiff << "F";
    
    // Clean up
    qDeleteAll(nwsData);
    qDeleteAll(pirateData);
}

// Test accuracy at both 6am and 6pm if available
TEST_F(AccuracyAtNWSTimesTest, CompareAtBoth6amAnd6pm) {
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
    
    QDateTime now = QDateTime::currentDateTime();
    QDateTime target6am(now.date(), QTime(6, 0, 0));
    QDateTime target6pm(now.date(), QTime(18, 0, 0));
    
    // Adjust if past today's times
    if (now.time() > QTime(6, 0, 0)) {
        target6am = target6am.addDays(1);
    }
    if (now.time() > QTime(18, 0, 0)) {
        target6pm = target6pm.addDays(1);
    }
    
    // Fetch both services
    QEventLoop nwsLoop;
    QList<WeatherData*> nwsData;
    bool nwsReceived = false;
    
    QObject::connect(nwsService, &NWSService::forecastReady, [&](QList<WeatherData*> data) {
        nwsData = data;
        nwsReceived = true;
        nwsLoop.quit();
    });
    
    nwsService->fetchForecast(lat, lon);
    QTimer::singleShot(20000, &nwsLoop, &QEventLoop::quit);
    nwsLoop.exec();
    
    if (!nwsReceived || nwsData.isEmpty()) {
        GTEST_SKIP() << "NWS did not respond";
    }
    
    QEventLoop pirateLoop;
    QList<WeatherData*> pirateData;
    bool pirateReceived = false;
    
    QObject::connect(pirateService, &PirateWeatherService::forecastReady, [&](QList<WeatherData*> data) {
        pirateData = data;
        pirateReceived = true;
        pirateLoop.quit();
    });
    
    pirateService->fetchForecast(lat, lon);
    QTimer::singleShot(20000, &pirateLoop, &QEventLoop::quit);
    pirateLoop.exec();
    
    if (!pirateReceived || pirateData.isEmpty()) {
        GTEST_SKIP() << "Pirate Weather did not respond";
    }
    
    // Compare at 6am
    WeatherData* nws6am = findClosestToTime(nwsData, target6am);
    WeatherData* pirate6am = findClosestToTime(pirateData, target6am);
    
    // Compare at 6pm
    WeatherData* nws6pm = findClosestToTime(nwsData, target6pm);
    WeatherData* pirate6pm = findClosestToTime(pirateData, target6pm);
    
    const double THRESHOLD_PERCENT = 30.0; // 30% threshold
    
    qInfo() << "\n=========================================";
    qInfo() << "ACCURACY REPORT: 6AM AND 6PM COMPARISON";
    qInfo() << "Threshold: " << THRESHOLD_PERCENT << "%";
    qInfo() << "=========================================\n";
    
    if (nws6am && pirate6am) {
        double diff6am = qAbs(nws6am->temperature() - pirate6am->temperature());
        double nwsTemp6am = nws6am->temperature();
        double percentDiff6am = (nwsTemp6am != 0.0) ? (diff6am / qAbs(nwsTemp6am)) * 100.0 : 0.0;
        
        qInfo() << "6AM Comparison:";
        qInfo() << "  NWS:" << nwsTemp6am << "F at" << nws6am->timestamp().toString();
        qInfo() << "  Pirate:" << pirate6am->temperature() << "F at" << pirate6am->timestamp().toString();
        qInfo() << "  Absolute Difference:" << diff6am << "F";
        qInfo() << "  Percentage Difference:" << QString::number(percentDiff6am, 'f', 2) << "%";
        
        // Assert that difference is within 30% threshold
        EXPECT_LE(percentDiff6am, THRESHOLD_PERCENT)
            << "6AM: Temperature difference exceeds 30% threshold. "
            << "Difference: " << percentDiff6am << "%, NWS: " << nwsTemp6am << "F, "
            << "Pirate: " << pirate6am->temperature() << "F";
    }
    
    if (nws6pm && pirate6pm) {
        double diff6pm = qAbs(nws6pm->temperature() - pirate6pm->temperature());
        double nwsTemp6pm = nws6pm->temperature();
        double percentDiff6pm = (nwsTemp6pm != 0.0) ? (diff6pm / qAbs(nwsTemp6pm)) * 100.0 : 0.0;
        
        qInfo() << "6PM Comparison:";
        qInfo() << "  NWS:" << nwsTemp6pm << "F at" << nws6pm->timestamp().toString();
        qInfo() << "  Pirate:" << pirate6pm->temperature() << "F at" << pirate6pm->timestamp().toString();
        qInfo() << "  Absolute Difference:" << diff6pm << "F";
        qInfo() << "  Percentage Difference:" << QString::number(percentDiff6pm, 'f', 2) << "%";
        
        // Assert that difference is within 30% threshold
        EXPECT_LE(percentDiff6pm, THRESHOLD_PERCENT)
            << "6PM: Temperature difference exceeds 30% threshold. "
            << "Difference: " << percentDiff6pm << "%, NWS: " << nwsTemp6pm << "F, "
            << "Pirate: " << pirate6pm->temperature() << "F";
    }
    
    qInfo() << "=========================================\n";
    
    // Clean up
    qDeleteAll(nwsData);
    qDeleteAll(pirateData);
}

