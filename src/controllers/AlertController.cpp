#include "controllers/AlertController.h"
#include "models/AlertModel.h"
#include "models/WeatherData.h"
#include "services/NWSService.h"
#include "database/DatabaseManager.h"
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QVariantMap>
#include <QList>

AlertController::AlertController(QObject *parent)
    : QObject(parent)
    , m_dbManager(DatabaseManager::instance())
    , m_weatherService(new NWSService(this))
    , m_checkTimer(new QTimer(this))
    , m_monitoring(false)
{
    connect(m_weatherService, &WeatherService::forecastReady,
            this, &AlertController::onForecastReady);
    connect(m_checkTimer, &QTimer::timeout,
            this, &AlertController::onCheckTimer);
    
    // Check alerts every 5 minutes
    m_checkTimer->setInterval(5 * 60 * 1000);
    
    // Load alerts from database
    loadAlertsFromDatabase();
}

AlertController::~AlertController() {
    qDeleteAll(m_alerts);
    m_alerts.clear();
}

void AlertController::addAlert(double latitude, double longitude, 
                               const QString& alertType, double threshold) {
    int alertId = -1;
    if (!m_dbManager->saveAlert(-1, latitude, longitude, alertType, threshold, alertId)) {
        qWarning() << "Failed to save alert to database";
        return;
    }
    
    AlertModel* alert = new AlertModel(this);
    alert->setId(alertId);
    alert->setLatitude(latitude);
    alert->setLongitude(longitude);
    alert->setAlertType(alertType);
    alert->setThreshold(threshold);
    alert->setEnabled(true);
    alert->setCreatedAt(QDateTime::currentDateTime());
    
    m_alerts.append(alert);
    emit alertsChanged();
    
    // Immediately check this alert
    checkAlertConditions(alert);
}

void AlertController::removeAlert(int alertId) {
    if (!m_dbManager->deleteAlert(alertId)) {
        qWarning() << "Failed to delete alert from database";
        return;
    }
    
    for (int i = 0; i < m_alerts.size(); ++i) {
        if (m_alerts[i]->id() == alertId) {
            AlertModel* alert = m_alerts.takeAt(i);
            alert->deleteLater();
            m_lastTriggered.remove(alertId);
            emit alertsChanged();
            return;
        }
    }
}

void AlertController::toggleAlert(int alertId, bool enabled) {
    if (!m_dbManager->updateAlertEnabled(alertId, enabled)) {
        qWarning() << "Failed to update alert in database";
        return;
    }
    
    for (AlertModel* alert : m_alerts) {
        if (alert->id() == alertId) {
            alert->setEnabled(enabled);
            emit alertsChanged();
            return;
        }
    }
}

void AlertController::startMonitoring() {
    if (m_monitoring) {
        return;
    }
    
    m_monitoring = true;
    m_checkTimer->start();
    emit monitoringChanged();
    
    // Do an initial check
    checkAlerts();
}

void AlertController::stopMonitoring() {
    if (!m_monitoring) {
        return;
    }
    
    m_monitoring = false;
    m_checkTimer->stop();
    emit monitoringChanged();
}

void AlertController::checkAlerts() {
    if (m_alerts.isEmpty()) {
        return;
    }
    
    // Fetch current weather for each alert location
    for (AlertModel* alert : m_alerts) {
        if (!alert->enabled()) {
            continue;
        }
        
        m_weatherService->fetchCurrent(alert->latitude(), alert->longitude());
        // Store alert for callback
        // Note: This is simplified - in a real implementation, we'd track which alert
        // each request belongs to
    }
}

void AlertController::onForecastReady(QList<WeatherData*> data) {
    if (data.isEmpty()) {
        return;
    }
    
    // Use first item as current weather
    WeatherData* current = data.first();
    
    // Check all alerts against this weather data
    for (AlertModel* alert : m_alerts) {
        if (!alert->enabled()) {
            continue;
        }
        
        // Check if location matches (simplified - should check distance)
        double latDiff = qAbs(alert->latitude() - current->latitude());
        double lonDiff = qAbs(alert->longitude() - current->longitude());
        
        if (latDiff < 0.01 && lonDiff < 0.01) {
            checkAlertConditions(alert);
        }
    }
    
    // Clean up
    qDeleteAll(data);
}

void AlertController::onCheckTimer() {
    checkAlerts();
}

void AlertController::loadAlertsFromDatabase() {
    QList<QVariantMap> alertData;
    if (!m_dbManager->getAlerts(alertData)) {
        qWarning() << "Failed to load alerts from database";
        return;
    }
    
    for (const QVariantMap& data : alertData) {
        AlertModel* alert = new AlertModel(this);
        alert->setId(data["id"].toInt());
        alert->setLatitude(data["latitude"].toDouble());
        alert->setLongitude(data["longitude"].toDouble());
        alert->setAlertType(data["alert_type"].toString());
        alert->setThreshold(data["threshold"].toDouble());
        alert->setEnabled(data["enabled"].toBool());
        alert->setCreatedAt(QDateTime::fromString(data["created_at"].toString(), Qt::ISODate));
        if (data.contains("last_triggered")) {
            alert->setLastTriggered(QDateTime::fromString(data["last_triggered"].toString(), Qt::ISODate));
        }
        
        m_alerts.append(alert);
    }
    
    emit alertsChanged();
}

void AlertController::checkAlertConditions(AlertModel* alert) {
    // This is a placeholder - in real implementation, we'd have current weather data
    // For now, we'll trigger based on forecast data received
    
    // Prevent duplicate triggers within 1 hour
    if (m_lastTriggered.contains(alert->id())) {
        QDateTime lastTriggered = m_lastTriggered[alert->id()];
        if (lastTriggered.secsTo(QDateTime::currentDateTime()) < 3600) {
            return;
        }
    }
    
    // In a real implementation, we'd evaluate the threshold here
    // For now, this is a placeholder
    m_lastTriggered[alert->id()] = QDateTime::currentDateTime();
    alert->setLastTriggered(QDateTime::currentDateTime());
    
    QString message = QString("Alert triggered: %1 threshold exceeded")
        .arg(alert->alertType());
    emit alertTriggered(alert, message);
}

bool AlertController::evaluateThreshold(const QString& type, double value, double threshold) {
    if (type == "precipitation" || type == "temperature" || type == "windSpeed") {
        return value >= threshold;
    }
    return false;
}

