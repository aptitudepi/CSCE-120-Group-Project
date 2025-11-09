#include <gtest/gtest.h>
#include "services/CacheManager.h"
#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QVariant>

class CacheManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = new CacheManager(10); // Small cache for testing
    }
    
    void TearDown() override {
        delete cache;
    }
    
    CacheManager* cache;
};

TEST_F(CacheManagerTest, PutAndGet) {
    QString key = "test_key";
    QVariant value = QString("test_value");
    
    cache->put(key, value, 3600); // 1 hour TTL
    
    QVariant retrieved = cache->get(key);
    EXPECT_TRUE(retrieved.isValid());
    EXPECT_EQ(retrieved.toString(), "test_value");
}

TEST_F(CacheManagerTest, Contains) {
    QString key = "test_key";
    QVariant value = QString("test_value");
    
    EXPECT_FALSE(cache->contains(key));
    
    cache->put(key, value, 3600);
    
    EXPECT_TRUE(cache->contains(key));
}

TEST_F(CacheManagerTest, Remove) {
    QString key = "test_key";
    QVariant value = QString("test_value");
    
    cache->put(key, value, 3600);
    EXPECT_TRUE(cache->contains(key));
    
    cache->remove(key);
    EXPECT_FALSE(cache->contains(key));
}

TEST_F(CacheManagerTest, Expiration) {
    QString key = "test_key";
    QVariant value = QString("test_value");
    
    cache->put(key, value, 1); // 1 second TTL
    
    QVariant retrieved = cache->get(key);
    EXPECT_TRUE(retrieved.isValid());
    
    // Wait for expiration
    QThread::msleep(1100);
    
    cache->cleanupExpired();
    QVariant expired = cache->get(key);
    // After expiration, get should return invalid
    EXPECT_FALSE(expired.isValid());
}

TEST_F(CacheManagerTest, LRUEviction) {
    // Fill cache to capacity
    for (int i = 0; i < 10; ++i) {
        QString key = QString("key_%1").arg(i);
        cache->put(key, QVariant(i), 3600);
    }
    
    EXPECT_EQ(cache->size(), 10);
    
    // Add one more - should evict least recently used
    cache->put("key_10", QVariant(10), 3600);
    
    // Cache should still be at max size
    EXPECT_LE(cache->size(), 10);
}

TEST_F(CacheManagerTest, GenerateKey) {
    QString key = CacheManager::generateKey("forecast", 30.6272, -96.3344);
    
    EXPECT_TRUE(key.contains("forecast"));
    EXPECT_TRUE(key.contains("30.6272"));
    EXPECT_TRUE(key.contains("-96.3344"));
}

TEST_F(CacheManagerTest, Clear) {
    cache->put("key1", QVariant(1), 3600);
    cache->put("key2", QVariant(2), 3600);
    
    EXPECT_GT(cache->size(), 0);
    
    cache->clear();
    
    EXPECT_EQ(cache->size(), 0);
}

