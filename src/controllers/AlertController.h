#ifndef ALERTCONTROLLER_H
#define ALERTCONTROLLER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include "models/AlertModel.h"
#include "services/WeatherService.h"
#include "database/DatabaseManager.h"

/**
 * @brief Controller for managing geofenced weather alerts
 * 
 * Monitors weather conditions and triggers alerts when thresholds are met.
 */
class AlertController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<AlertModel*> alerts READ alerts NOTIFY alertsChanged)
    Q_PROPERTY(bool monitoring READ monitoring NOTIFY monitoringChanged)
    
public:
    explicit AlertController(QObject *parent = nullptr);
    ~AlertController() override;
    
    QList<AlertModel*> alerts() const { return m_alerts; }
    bool monitoring() const { return m_monitoring; }
    
    Q_INVOKABLE void addAlert(double latitude, double longitude, 
                              const QString& alertType, double threshold);
    Q_INVOKABLE void removeAlert(int alertId);
    Q_INVOKABLE void toggleAlert(int alertId, bool enabled);
    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();
    Q_INVOKABLE void checkAlerts();
    
signals:
    void alertsChanged();
    void monitoringChanged();
    void alertTriggered(AlertModel* alert, const QString& message);
    
private slots:
    void onForecastReady(QList<WeatherData*> data);
    void onCheckTimer();
    
private:
    void loadAlertsFromDatabase();
    void checkAlertConditions(AlertModel* alert);
    void checkAlertConditions(AlertModel* alert, WeatherData* currentWeather);
    bool evaluateThreshold(const QString& type, double value, double threshold);
    
    QList<AlertModel*> m_alerts;
    DatabaseManager* m_dbManager;
    WeatherService* m_weatherService;
    QTimer* m_checkTimer;
    bool m_monitoring;
    QMap<int, QDateTime> m_lastTriggered; // Alert ID -> Last triggered time
};

#endif // ALERTCONTROLLER_H

