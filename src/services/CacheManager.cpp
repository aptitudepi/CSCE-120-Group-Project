#include "services/CacheManager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QStringList>
#include <QDateTime>

CacheManager::CacheManager(int maxSize, QObject *parent)
    : QObject(parent)
    , m_maxSize(maxSize)
{
}

CacheManager::~CacheManager() {
    clear();
}

QVariant CacheManager::get(const QString& key) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_cache.contains(key)) {
        return QVariant();
    }
    
    CacheEntry& entry = m_cache[key];
    
    // Check if expired
    if (entry.isExpired()) {
        m_cache.remove(key);
        m_accessOrder.removeAll(key);
        return QVariant();
    }
    
    // Update access time
    entry.lastAccessed = QDateTime::currentDateTime();
    updateAccessTime(key);
    
    return entry.data;
}

void CacheManager::put(const QString& key, const QVariant& value, int ttlSeconds) {
    QMutexLocker locker(&m_mutex);
    
    // Remove if already exists
    if (m_cache.contains(key)) {
        m_accessOrder.removeAll(key);
    }
    
    // Check if we need to evict
    while (m_cache.size() >= m_maxSize && !m_cache.isEmpty()) {
        evictLRU();
    }
    
    // Add new entry
    QDateTime expiresAt = QDateTime::currentDateTime().addSecs(ttlSeconds);
    m_cache[key] = CacheEntry(value, expiresAt);
    m_accessOrder.append(key);
}

void CacheManager::remove(const QString& key) {
    QMutexLocker locker(&m_mutex);
    m_cache.remove(key);
    m_accessOrder.removeAll(key);
}

bool CacheManager::contains(const QString& key) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    if (m_cache[key].isExpired()) {
        m_cache.remove(key);
        m_accessOrder.removeAll(key);
        return false;
    }
    
    return true;
}

void CacheManager::clear() {
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
    m_accessOrder.clear();
}

int CacheManager::size() const {
    QMutexLocker locker(&m_mutex);
    return m_cache.size();
}

int CacheManager::maxSize() const {
    return m_maxSize;
}

void CacheManager::cleanupExpired() {
    QMutexLocker locker(&m_mutex);
    
    QStringList keysToRemove;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().isExpired()) {
            keysToRemove.append(it.key());
        }
    }
    
    for (const QString& key : keysToRemove) {
        m_cache.remove(key);
        m_accessOrder.removeAll(key);
    }
}

QString CacheManager::generateKey(const QString& prefix, double lat, double lon, const QString& suffix) {
    QString key = QString("%1_%2_%3").arg(prefix).arg(lat, 0, 'f', 4).arg(lon, 0, 'f', 4);
    if (!suffix.isEmpty()) {
        key += "_" + suffix;
    }
    return key;
}

void CacheManager::evictLRU() {
    if (m_accessOrder.isEmpty()) {
        return;
    }
    
    QString keyToRemove = m_accessOrder.takeFirst();
    m_cache.remove(keyToRemove);
}

void CacheManager::updateAccessTime(const QString& key) {
    // Move to end of access order (most recently used)
    m_accessOrder.removeAll(key);
    m_accessOrder.append(key);
}

