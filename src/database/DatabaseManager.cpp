#include "database/DatabaseManager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QSqlError>
#include <QDateTime>
#include <QCoreApplication>

DatabaseManager* DatabaseManager::s_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

DatabaseManager::~DatabaseManager() {
    if (m_database.isOpen()) {
        m_database.close();
    }
}

DatabaseManager* DatabaseManager::instance() {
    if (!s_instance) {
        s_instance = new DatabaseManager();
    }
    return s_instance;
}

bool DatabaseManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Get database path
    QString dbPath = getDatabasePath();
    QDir dbDir = QFileInfo(dbPath).dir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            qCritical() << "Failed to create database directory:" << dbDir.path();
            return false;
        }
    }
    
    // Open database
    m_database = QSqlDatabase::addDatabase("QSQLITE", "HyperlocalWeather");
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        qCritical() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    // Create tables
    if (!createTables()) {
        qCritical() << "Failed to create database tables";
        m_database.close();
        return false;
    }
    
    // Cleanup expired cache entries
    cleanupExpiredCache();
    
    m_initialized = true;
    qInfo() << "Database initialized successfully:" << dbPath;
    return true;
}

QString DatabaseManager::getDatabasePath() const {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dataDir(dataPath);
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    return dataDir.filePath("hyperlocal_weather.db");
}

bool DatabaseManager::createTables() {
    QSqlQuery query(m_database);
    
    // Locations table
    QString createLocations = R"(
        CREATE TABLE IF NOT EXISTS locations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createLocations)) {
        qCritical() << "Failed to create locations table:" << query.lastError().text();
        return false;
    }
    
    // Alerts table
    QString createAlerts = R"(
        CREATE TABLE IF NOT EXISTS alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            location_id INTEGER,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            alert_type TEXT NOT NULL,
            threshold REAL NOT NULL,
            enabled INTEGER DEFAULT 1,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_triggered DATETIME,
            FOREIGN KEY (location_id) REFERENCES locations(id)
        )
    )";
    
    if (!query.exec(createAlerts)) {
        qCritical() << "Failed to create alerts table:" << query.lastError().text();
        return false;
    }
    
    // Preferences table
    QString createPreferences = R"(
        CREATE TABLE IF NOT EXISTS user_preferences (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        )
    )";
    
    if (!query.exec(createPreferences)) {
        qCritical() << "Failed to create preferences table:" << query.lastError().text();
        return false;
    }
    
    // Cache table
    QString createCache = R"(
        CREATE TABLE IF NOT EXISTS forecast_cache (
            cache_key TEXT PRIMARY KEY,
            data TEXT NOT NULL,
            expires_at DATETIME NOT NULL
        )
    )";
    
    if (!query.exec(createCache)) {
        qCritical() << "Failed to create cache table:" << query.lastError().text();
        return false;
    }
    
    // Historical weather table for time-series data
    QString createHistoricalWeather = R"(
        CREATE TABLE IF NOT EXISTS historical_weather (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            timestamp DATETIME NOT NULL,
            source TEXT NOT NULL,
            temperature REAL,
            precip_probability REAL,
            precip_intensity REAL,
            wind_speed REAL,
            wind_direction INTEGER,
            humidity INTEGER,
            pressure REAL,
            cloud_cover INTEGER,
            visibility INTEGER,
            uv_index INTEGER,
            weather_condition TEXT,
            weather_description TEXT,
            data_json TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(latitude, longitude, timestamp, source)
        )
    )";
    
    if (!query.exec(createHistoricalWeather)) {
        qCritical() << "Failed to create historical_weather table:" << query.lastError().text();
        return false;
    }
    
    // Create indexes
    query.exec("CREATE INDEX IF NOT EXISTS idx_alerts_location ON alerts(location_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_alerts_enabled ON alerts(enabled)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_cache_expires ON forecast_cache(expires_at)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_historical_location_time ON historical_weather(latitude, longitude, timestamp)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_historical_source_time ON historical_weather(source, timestamp)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_historical_timestamp ON historical_weather(timestamp)");
    
    return true;
}

bool DatabaseManager::saveLocation(const QString& name, double latitude, double longitude, int& locationId) {
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO locations (name, latitude, longitude) VALUES (?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(latitude);
    query.addBindValue(longitude);
    
    if (!query.exec()) {
        qWarning() << "Failed to save location:" << query.lastError().text();
        return false;
    }
    
    locationId = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::getLocations(QList<QVariantMap>& locations) {
    QSqlQuery query(m_database);
    query.prepare("SELECT id, name, latitude, longitude, created_at FROM locations ORDER BY created_at DESC");
    
    if (!query.exec()) {
        qWarning() << "Failed to get locations:" << query.lastError().text();
        return false;
    }
    
    locations.clear();
    while (query.next()) {
        QVariantMap location;
        location["id"] = query.value(0).toInt();
        location["name"] = query.value(1).toString();
        location["latitude"] = query.value(2).toDouble();
        location["longitude"] = query.value(3).toDouble();
        location["created_at"] = query.value(4).toString();
        locations.append(location);
    }
    
    return true;
}

bool DatabaseManager::deleteLocation(int locationId) {
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM locations WHERE id = ?");
    query.addBindValue(locationId);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete location:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::saveAlert(int locationId, double latitude, double longitude,
                                const QString& alertType, double threshold, int& alertId) {
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO alerts (location_id, latitude, longitude, alert_type, threshold) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(locationId > 0 ? locationId : QVariant());
    query.addBindValue(latitude);
    query.addBindValue(longitude);
    query.addBindValue(alertType);
    query.addBindValue(threshold);
    
    if (!query.exec()) {
        qWarning() << "Failed to save alert:" << query.lastError().text();
        return false;
    }
    
    alertId = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::getAlerts(QList<QVariantMap>& alerts) {
    QSqlQuery query(m_database);
    query.prepare("SELECT id, location_id, latitude, longitude, alert_type, threshold, enabled, created_at, last_triggered FROM alerts ORDER BY created_at DESC");
    
    if (!query.exec()) {
        qWarning() << "Failed to get alerts:" << query.lastError().text();
        return false;
    }
    
    alerts.clear();
    while (query.next()) {
        QVariantMap alert;
        alert["id"] = query.value(0).toInt();
        alert["location_id"] = query.value(1).toInt();
        alert["latitude"] = query.value(2).toDouble();
        alert["longitude"] = query.value(3).toDouble();
        alert["alert_type"] = query.value(4).toString();
        alert["threshold"] = query.value(5).toDouble();
        alert["enabled"] = query.value(6).toInt() == 1;
        alert["created_at"] = query.value(7).toString();
        if (!query.value(8).isNull()) {
            alert["last_triggered"] = query.value(8).toString();
        }
        alerts.append(alert);
    }
    
    return true;
}

bool DatabaseManager::deleteAlert(int alertId) {
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM alerts WHERE id = ?");
    query.addBindValue(alertId);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete alert:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::updateAlertEnabled(int alertId, bool enabled) {
    QSqlQuery query(m_database);
    query.prepare("UPDATE alerts SET enabled = ? WHERE id = ?");
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(alertId);
    
    if (!query.exec()) {
        qWarning() << "Failed to update alert:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::updateAlertLastTriggered(int alertId, const QDateTime& triggered) {
    QSqlQuery query(m_database);
    query.prepare("UPDATE alerts SET last_triggered = ? WHERE id = ?");
    query.addBindValue(triggered.toString(Qt::ISODate));
    query.addBindValue(alertId);
    
    if (!query.exec()) {
        qWarning() << "Failed to update alert last triggered:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::setPreference(const QString& key, const QString& value) {
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO user_preferences (key, value) VALUES (?, ?)");
    query.addBindValue(key);
    query.addBindValue(value);
    
    if (!query.exec()) {
        qWarning() << "Failed to set preference:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QString DatabaseManager::getPreference(const QString& key, const QString& defaultValue) {
    QSqlQuery query(m_database);
    query.prepare("SELECT value FROM user_preferences WHERE key = ?");
    query.addBindValue(key);
    
    if (!query.exec()) {
        qWarning() << "Failed to get preference:" << query.lastError().text();
        return defaultValue;
    }
    
    if (query.next()) {
        return query.value(0).toString();
    }
    
    return defaultValue;
}

bool DatabaseManager::saveCacheEntry(const QString& key, const QString& data, const QDateTime& expiresAt) {
    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO forecast_cache (cache_key, data, expires_at) VALUES (?, ?, ?)");
    query.addBindValue(key);
    query.addBindValue(data);
    query.addBindValue(expiresAt.toString(Qt::ISODate));
    
    if (!query.exec()) {
        qWarning() << "Failed to save cache entry:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QString DatabaseManager::getCacheEntry(const QString& key) {
    QSqlQuery query(m_database);
    query.prepare("SELECT data FROM forecast_cache WHERE cache_key = ? AND expires_at > datetime('now')");
    query.addBindValue(key);
    
    if (!query.exec()) {
        qWarning() << "Failed to get cache entry:" << query.lastError().text();
        return QString();
    }
    
    if (query.next()) {
        return query.value(0).toString();
    }
    
    return QString();
}

bool DatabaseManager::deleteCacheEntry(const QString& key) {
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM forecast_cache WHERE cache_key = ?");
    query.addBindValue(key);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete cache entry:" << query.lastError().text();
        return false;
    }
    
    return true;
}

void DatabaseManager::cleanupExpiredCache() {
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM forecast_cache WHERE expires_at < datetime('now')");
    
    if (!query.exec()) {
        qWarning() << "Failed to cleanup expired cache:" << query.lastError().text();
    }
}

