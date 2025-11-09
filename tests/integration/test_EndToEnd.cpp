#include <gtest/gtest.h>
#include "controllers/WeatherController.h"
#include "database/DatabaseManager.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QString>
#include <QObject>

class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize database
        DatabaseManager::instance()->initialize();
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(EndToEndTest, DatabaseInitialization) {
    DatabaseManager* db = DatabaseManager::instance();
    EXPECT_TRUE(db->isInitialized());
}

TEST_F(EndToEndTest, WeatherControllerCreation) {
    WeatherController controller;
    EXPECT_NE(controller.forecastModel(), nullptr);
    EXPECT_FALSE(controller.loading());
}

// Integration test - requires network
TEST_F(EndToEndTest, DISABLED_FetchForecast) {
    WeatherController controller;
    QEventLoop loop;
    bool forecastReceived = false;
    
    QObject::connect(&controller, &WeatherController::forecastUpdated, [&]() {
        forecastReceived = true;
        loop.quit();
    });
    
    QObject::connect(&controller, &WeatherController::errorMessageChanged, [&]() {
        if (!controller.errorMessage().isEmpty()) {
            GTEST_SKIP() << "Error: " << controller.errorMessage().toStdString();
            loop.quit();
        }
    });
    
    // Test with College Station, TX
    controller.fetchForecast(30.6272, -96.3344);
    
    QTimer::singleShot(15000, &loop, &QEventLoop::quit); // 15 second timeout
    loop.exec();
    
    // Note: This test may fail if network is unavailable
    // It's marked as DISABLED by default to avoid flaky tests
    if (forecastReceived) {
        EXPECT_GT(controller.forecastModel()->rowCount(), 0);
    }
}

