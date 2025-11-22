#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariantMap>
#include <QList>

/**
 * @brief SQLite database manager for persistent storage
 * 
 * Manages all database operations including locations, alerts,
 * preferences, and cache storage.
 */
class DatabaseManager : public QObject
{
    Q_OBJECT
    
public:
    static DatabaseManager* instance();
    
    /**
     * @brief Initialize database connection and create tables
     */
    bool initialize();
    
    /**
     * @brief Check if database is initialized
     */
    bool isInitialized() const { return m_initialized; }
    
    // Location management
    bool saveLocation(const QString& name, double latitude, double longitude, int& locationId);
    bool getLocations(QList<QVariantMap>& locations);
    bool deleteLocation(int locationId);
    
    // Alert management
    bool saveAlert(int locationId, double latitude, double longitude,
                   const QString& alertType, double threshold, int& alertId);
    bool getAlerts(QList<QVariantMap>& alerts);
    bool deleteAlert(int alertId);
    bool updateAlertEnabled(int alertId, bool enabled);
    bool updateAlertLastTriggered(int alertId, const QDateTime& triggered);
    
    // Preferences
    bool setPreference(const QString& key, const QString& value);
    QString getPreference(const QString& key, const QString& defaultValue = QString());
    
    // Cache (persistent cache across sessions)
    bool saveCacheEntry(const QString& key, const QString& data, const QDateTime& expiresAt);
    QString getCacheEntry(const QString& key);
    bool deleteCacheEntry(const QString& key);
    void cleanupExpiredCache();
    
    // Get database connection for use by other managers
    QSqlDatabase database() const { return m_database; }
    
private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    bool createTables();
    QString getDatabasePath() const;
    
    static DatabaseManager* s_instance;
    QSqlDatabase m_database;
    bool m_initialized;
};

#endif // DATABASEMANAGER_H

