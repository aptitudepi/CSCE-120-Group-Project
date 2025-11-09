#include <gtest/gtest.h>
#include "services/NWSService.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QObject>

class NWSServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = new NWSService();
    }
    
    void TearDown() override {
        delete service;
    }
    
    NWSService* service;
};

TEST_F(NWSServiceTest, ServiceName) {
    EXPECT_EQ(service->serviceName(), "NWS");
}

TEST_F(NWSServiceTest, IsAvailable) {
    EXPECT_TRUE(service->isAvailable());
}

// Integration test - requires network
TEST_F(NWSServiceTest, DISABLED_FetchGridpoint) {
    QEventLoop loop;
    bool received = false;
    
    QObject::connect(service, &NWSService::gridpointReady, [&](QString office, int x, int y) {
        EXPECT_FALSE(office.isEmpty());
        EXPECT_GE(x, 0);
        EXPECT_GE(y, 0);
        received = true;
        loop.quit();
    });
    
    QObject::connect(service, &NWSService::error, [&](QString error) {
        GTEST_SKIP() << "Network error: " << error.toStdString();
        loop.quit();
    });
    
    // Test with College Station, TX
    service->fetchGridpoint(30.6272, -96.3344);
    
    QTimer::singleShot(10000, &loop, &QEventLoop::quit); // 10 second timeout
    loop.exec();
    
    EXPECT_TRUE(received);
}

