#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QVariant>

/**
 * @brief LRU cache manager with TTL support
 * 
 * Thread-safe in-memory cache implementation using Least Recently Used (LRU)
 * eviction policy and Time-To-Live (TTL) expiration.
 */
class CacheManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief Cache entry structure
     */
    struct CacheEntry {
        QVariant data;
        QDateTime expiresAt;
        QDateTime lastAccessed;
        
        CacheEntry() = default;
        CacheEntry(const QVariant& d, const QDateTime& exp)
            : data(d), expiresAt(exp), lastAccessed(QDateTime::currentDateTime()) {}
        
        bool isExpired() const {
            return QDateTime::currentDateTime() > expiresAt;
        }
    };
    
    /**
     * @brief Constructor
     * @param maxSize Maximum number of entries in cache
     * @param parent Parent QObject
     */
    explicit CacheManager(int maxSize = 100, QObject *parent = nullptr);
    ~CacheManager() override;
    
    /**
     * @brief Get value from cache
     * @param key Cache key
     * @return Cached value or invalid QVariant if not found/expired
     */
    QVariant get(const QString& key);
    
    /**
     * @brief Store value in cache
     * @param key Cache key
     * @param value Value to cache
     * @param ttlSeconds Time to live in seconds (default: 3600)
     */
    void put(const QString& key, const QVariant& value, int ttlSeconds = 3600);
    
    /**
     * @brief Remove entry from cache
     * @param key Cache key
     */
    void remove(const QString& key);
    
    /**
     * @brief Check if key exists in cache and is not expired
     * @param key Cache key
     * @return true if key exists and is valid
     */
    bool contains(const QString& key);
    
    /**
     * @brief Clear all cache entries
     */
    void clear();
    
    /**
     * @brief Get cache statistics
     */
    int size() const;
    int maxSize() const;
    
    /**
     * @brief Clean up expired entries
     */
    void cleanupExpired();
    
    /**
     * @brief Generate cache key for location and type
     */
    static QString generateKey(const QString& prefix, double lat, double lon, const QString& suffix = "");
    
private:
    void evictLRU();
    void updateAccessTime(const QString& key);
    
    QMap<QString, CacheEntry> m_cache;
    QList<QString> m_accessOrder;  // LRU tracking
    mutable QMutex m_mutex;
    int m_maxSize;
};

#endif // CACHEMANAGER_H

