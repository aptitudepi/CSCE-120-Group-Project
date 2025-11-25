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
    , m_countdownTimer(new QTimer(this))
    , m_monitoring(false)
    , m_secondsToNextCheck(0)
    , m_checkIntervalSeconds(5 * 60)
{
    connect(m_weatherService, &WeatherService::forecastReady,
            this, &AlertController::onForecastReady);
    connect(m_countdownTimer, &QTimer::timeout,
            this, &AlertController::onCountdownTick);
    
    // Check alerts every 5 minutes
    m_countdownTimer->setInterval(1000);
    
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
        resetCountdown();
        return;
    }
    
    m_monitoring = true;
    resetCountdown();
    m_countdownTimer->start();
    emit monitoringChanged();
    
    // Do an initial check
    checkAlerts();
}

void AlertController::stopMonitoring() {
    if (!m_monitoring) {
        return;
    }
    
    m_monitoring = false;
    m_countdownTimer->stop();
    m_secondsToNextCheck = 0;
    emit secondsToNextCheckChanged();
    emit monitoringChanged();
}

void AlertController::checkAlerts() {
    int activeAlerts = 0;
    for (AlertModel* alert : m_alerts) {
        if (alert->enabled()) {
            activeAlerts++;
        }
    }
    
    emit alertCycleStarted(activeAlerts);
    
    if (activeAlerts == 0) {
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
        
        // Check alert conditions with current weather data
        checkAlertConditions(alert, current);
    }
    
    // Clean up - note: data items may be owned by other objects
    // Only delete if we own them
    // For now, assume they're managed elsewhere
}

void AlertController::onCountdownTick() {
    if (!m_monitoring) {
        return;
    }
    
    if (m_secondsToNextCheck > 0) {
        m_secondsToNextCheck--;
        emit secondsToNextCheckChanged();
    }
    
    if (m_secondsToNextCheck <= 0) {
        checkAlerts();
        resetCountdown();
    }
}

void AlertController::resetCountdown() {
    m_secondsToNextCheck = m_checkIntervalSeconds;
    emit secondsToNextCheckChanged();
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
    if (!alert || !alert->enabled()) {
        return;
    }
    
    // Prevent duplicate triggers within 1 hour
    if (m_lastTriggered.contains(alert->id())) {
        QDateTime lastTriggered = m_lastTriggered[alert->id()];
        if (lastTriggered.secsTo(QDateTime::currentDateTime()) < 3600) {
            return;
        }
    }
    
    // This method is called when forecast data is received
    // In a real implementation, we'd have current weather data here
    // For now, we'll use the first forecast item as current weather
    // Note: This is a simplified implementation - ideally we'd fetch current weather
}

void AlertController::checkAlertConditions(AlertModel* alert, WeatherData* currentWeather) {
    if (!alert || !alert->enabled() || !currentWeather) {
        return;
    }
    
    // Prevent duplicate triggers within 1 hour
    if (m_lastTriggered.contains(alert->id())) {
        QDateTime lastTriggered = m_lastTriggered[alert->id()];
        if (lastTriggered.secsTo(QDateTime::currentDateTime()) < 3600) {
            return;
        }
    }
    
    // Check if location matches (within 10km radius - simplified)
    double latDiff = qAbs(alert->latitude() - currentWeather->latitude());
    double lonDiff = qAbs(alert->longitude() - currentWeather->longitude());
    
    // Rough distance check (1 degree ≈ 111km, so 0.1 degree ≈ 11km)
    if (latDiff > 0.1 || lonDiff > 0.1) {
        return; // Location doesn't match
    }
    
    // Evaluate threshold based on alert type
    bool thresholdMet = false;
    double currentValue = 0.0;
    QString alertType = alert->alertType().toLower();
    
    if (alertType == "precipitation" || alertType == "precip") {
        currentValue = currentWeather->precipIntensity();
        thresholdMet = evaluateThreshold("precipitation", currentValue, alert->threshold());
    } else if (alertType == "temperature" || alertType == "temp") {
        currentValue = currentWeather->temperature();
        thresholdMet = evaluateThreshold("temperature", currentValue, alert->threshold());
    } else if (alertType == "windspeed" || alertType == "wind") {
        currentValue = currentWeather->windSpeed();
        thresholdMet = evaluateThreshold("windSpeed", currentValue, alert->threshold());
    } else if (alertType == "humidity") {
        currentValue = currentWeather->humidity();
        thresholdMet = evaluateThreshold("humidity", currentValue, alert->threshold());
    } else if (alertType == "pressure") {
        currentValue = currentWeather->pressure();
        thresholdMet = evaluateThreshold("pressure", currentValue, alert->threshold());
    }
    
    if (thresholdMet) {
        m_lastTriggered[alert->id()] = QDateTime::currentDateTime();
        alert->setLastTriggered(QDateTime::currentDateTime());
        m_dbManager->updateAlertLastTriggered(alert->id(), QDateTime::currentDateTime());
        
        QString message = QString("Alert triggered: %1 is %2 (threshold: %3)")
            .arg(alert->alertType())
            .arg(currentValue, 0, 'f', 1)
            .arg(alert->threshold(), 0, 'f', 1);
        emit alertTriggered(alert, message);
    }
}

bool AlertController::evaluateThreshold(const QString& type, double value, double threshold) {
    QString alertType = type.toLower();
    
    // For most metrics, check if value exceeds threshold
    if (alertType == "precipitation" || alertType == "precip" ||
        alertType == "windspeed" || alertType == "wind" ||
        alertType == "humidity") {
        return value >= threshold;
    }
    
    // For temperature, check if it's above or below threshold
    // (could be "above 90F" or "below 32F" - simplified to above)
    if (alertType == "temperature" || alertType == "temp") {
        return value >= threshold;
    }
    
    // For pressure, check if it's above or below threshold
    if (alertType == "pressure") {
        return value >= threshold;
    }
    
    return false;
}

