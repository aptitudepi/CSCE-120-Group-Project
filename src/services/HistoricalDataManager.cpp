#include "services/HistoricalDataManager.h"
#include "database/DatabaseManager.h"
#include "models/WeatherData.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QDateTime>
#include <QtMath>

HistoricalDataManager::HistoricalDataManager(QObject *parent)
    : QObject(parent)
    , m_retentionDays(7)
{
}

HistoricalDataManager::~HistoricalDataManager() = default;

bool HistoricalDataManager::initialize() {
    if (!createTableIfNotExists()) {
        qCritical() << "Failed to create historical_weather table";
        return false;
    }
    
    // Load retention days from preferences
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (dbManager && dbManager->isInitialized()) {
        QString retentionStr = dbManager->getPreference("historical_retention_days", "7");
        bool ok;
        int days = retentionStr.toInt(&ok);
        if (ok && days > 0) {
            m_retentionDays = days;
        }
    }
    
    qInfo() << "HistoricalDataManager initialized with retention:" << m_retentionDays << "days";
    return true;
}

bool HistoricalDataManager::createTableIfNotExists() {
    // Table is created by DatabaseManager, just verify it exists
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager || !dbManager->isInitialized()) {
        qWarning() << "DatabaseManager not initialized";
        return false;
    }
    
    QSqlDatabase db = dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "Database not open";
        return false;
    }
    
    // Check if table exists
    QSqlQuery query(db);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='historical_weather'");
    if (!query.exec()) {
        qWarning() << "Failed to check table existence:" << query.lastError().text();
        return false;
    }
    
    return query.next(); // Table exists if query has results
}

bool HistoricalDataManager::storeForecast(double latitude, double longitude, WeatherData* data, const QString& source) {
    if (!data) {
        return false;
    }
    
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager || !dbManager->isInitialized()) {
        qWarning() << "DatabaseManager not initialized";
        return false;
    }
    
    QSqlDatabase db = dbManager->database();
    QSqlQuery query(db);
    
    // Round coordinates to reduce uniqueness issues (0.0001 degree ~ 11 meters)
    double roundedLat = qRound(latitude * 10000.0) / 10000.0;
    double roundedLon = qRound(longitude * 10000.0) / 10000.0;
    
    query.prepare(R"(
        INSERT OR REPLACE INTO historical_weather 
        (latitude, longitude, timestamp, source, temperature, precip_probability, precip_intensity,
         wind_speed, wind_direction, humidity, pressure, cloud_cover, visibility, uv_index,
         weather_condition, weather_description, data_json)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(roundedLat);
    query.addBindValue(roundedLon);
    query.addBindValue(data->timestamp().toString(Qt::ISODate));
    query.addBindValue(source);
    query.addBindValue(data->temperature());
    query.addBindValue(data->precipProbability());
    query.addBindValue(data->precipIntensity());
    query.addBindValue(data->windSpeed());
    query.addBindValue(data->windDirection());
    query.addBindValue(data->humidity());
    query.addBindValue(data->pressure());
    query.addBindValue(data->cloudCover());
    query.addBindValue(data->visibility());
    query.addBindValue(data->uvIndex());
    query.addBindValue(data->weatherCondition());
    query.addBindValue(data->weatherDescription());
    
    // Store full JSON data
    QJsonObject jsonData = data->toJson();
    QJsonDocument doc(jsonData);
    query.addBindValue(doc.toJson(QJsonDocument::Compact));
    
    if (!query.exec()) {
        qWarning() << "Failed to store historical forecast:" << query.lastError().text();
        emit error(query.lastError().text());
        return false;
    }
    
    return true;
}

bool HistoricalDataManager::storeForecasts(double latitude, double longitude, const QList<WeatherData*>& forecasts, const QString& source) {
    if (forecasts.isEmpty()) {
        return true;
    }
    
    int successCount = 0;
    for (WeatherData* data : forecasts) {
        if (storeForecast(latitude, longitude, data, source)) {
            successCount++;
        }
    }
    
    if (successCount > 0) {
        emit dataStored(successCount);
    }
    
    return successCount == forecasts.size();
}

QList<WeatherData*> HistoricalDataManager::getHistoricalData(double latitude, double longitude,
                                                               const QDateTime& startTime,
                                                               const QDateTime& endTime,
                                                               const QString& source) {
    QList<WeatherData*> results;
    
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager || !dbManager->isInitialized()) {
        qWarning() << "DatabaseManager not initialized";
        return results;
    }
    
    QSqlDatabase db = dbManager->database();
    QSqlQuery query(db);
    
    // Round coordinates to match stored values
    double roundedLat = qRound(latitude * 10000.0) / 10000.0;
    double roundedLon = qRound(longitude * 10000.0) / 10000.0;
    
    QString sql = R"(
        SELECT latitude, longitude, timestamp, source, temperature, precip_probability, precip_intensity,
               wind_speed, wind_direction, humidity, pressure, cloud_cover, visibility, uv_index,
               weather_condition, weather_description, data_json
        FROM historical_weather
        WHERE ABS(latitude - ?) < 0.0001
          AND ABS(longitude - ?) < 0.0001
          AND timestamp >= ?
          AND timestamp <= ?
    )";
    
    if (!source.isEmpty()) {
        sql += " AND source = ?";
    }
    
    sql += " ORDER BY timestamp ASC";
    
    query.prepare(sql);
    query.addBindValue(roundedLat);
    query.addBindValue(roundedLon);
    query.addBindValue(startTime.toString(Qt::ISODate));
    query.addBindValue(endTime.toString(Qt::ISODate));
    
    if (!source.isEmpty()) {
        query.addBindValue(source);
    }
    
    if (!query.exec()) {
        qWarning() << "Failed to retrieve historical data:" << query.lastError().text();
        emit error(query.lastError().text());
        return results;
    }
    
    while (query.next()) {
        // Try to restore from JSON first (preserves all fields)
        QString jsonStr = query.value(16).toString(); // data_json column
        if (!jsonStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                WeatherData* data = WeatherData::fromJson(doc.object(), this);
                if (data) {
                    results.append(data);
                    continue;
                }
            }
        }
        
        // Fallback: reconstruct from individual columns
        WeatherData* fallbackData = new WeatherData(this);
        fallbackData->setLatitude(query.value(0).toDouble());
        fallbackData->setLongitude(query.value(1).toDouble());
        fallbackData->setTimestamp(QDateTime::fromString(query.value(2).toString(), Qt::ISODate));
        fallbackData->setTemperature(query.value(4).toDouble());
        fallbackData->setPrecipProbability(query.value(5).toDouble());
        fallbackData->setPrecipIntensity(query.value(6).toDouble());
        fallbackData->setWindSpeed(query.value(7).toDouble());
        fallbackData->setWindDirection(query.value(8).toInt());
        fallbackData->setHumidity(query.value(9).toInt());
        fallbackData->setPressure(query.value(10).toDouble());
        fallbackData->setCloudCover(query.value(11).toInt());
        fallbackData->setVisibility(query.value(12).toInt());
        fallbackData->setUvIndex(query.value(13).toInt());
        fallbackData->setWeatherCondition(query.value(14).toString());
        fallbackData->setWeatherDescription(query.value(15).toString());
        
        results.append(fallbackData);
    }
    
    return results;
}

QList<WeatherData*> HistoricalDataManager::getRecentData(double latitude, double longitude,
                                                          int hours,
                                                          const QString& source) {
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime = endTime.addSecs(-hours * 3600);
    
    return getHistoricalData(latitude, longitude, startTime, endTime, source);
}

int HistoricalDataManager::cleanupOldData(int daysToKeep) {
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager || !dbManager->isInitialized()) {
        qWarning() << "DatabaseManager not initialized";
        return 0;
    }
    
    QSqlDatabase db = dbManager->database();
    QSqlQuery query(db);
    
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    query.prepare("DELETE FROM historical_weather WHERE timestamp < ?");
    query.addBindValue(cutoffTime.toString(Qt::ISODate));
    
    if (!query.exec()) {
        qWarning() << "Failed to cleanup old data:" << query.lastError().text();
        emit error(query.lastError().text());
        return 0;
    }
    
    int deletedCount = query.numRowsAffected();
    emit cleanupComplete(deletedCount);
    qInfo() << "Cleaned up" << deletedCount << "old historical weather records";
    
    return deletedCount;
}

QString HistoricalDataManager::generateLocationKey(double latitude, double longitude, double precision) const {
    double roundedLat = qRound(latitude / precision) * precision;
    double roundedLon = qRound(longitude / precision) * precision;
    return QString("%1_%2").arg(roundedLat, 0, 'f', 4).arg(roundedLon, 0, 'f', 4);
}

