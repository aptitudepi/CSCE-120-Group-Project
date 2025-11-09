#ifndef ALERTMODEL_H
#define ALERTMODEL_H

#include <QObject>
#include <QDateTime>

/**
 * @brief Data model representing a weather alert
 */
class AlertModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(QString alertType READ alertType WRITE setAlertType NOTIFY alertTypeChanged)
    Q_PROPERTY(double threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime lastTriggered READ lastTriggered WRITE setLastTriggered NOTIFY lastTriggeredChanged)
    
public:
    explicit AlertModel(QObject *parent = nullptr);
    ~AlertModel() override;
    
    int id() const { return m_id; }
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    QString alertType() const { return m_alertType; }
    double threshold() const { return m_threshold; }
    bool enabled() const { return m_enabled; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime lastTriggered() const { return m_lastTriggered; }
    
    void setId(int id);
    void setLatitude(double lat);
    void setLongitude(double lon);
    void setAlertType(const QString& type);
    void setThreshold(double threshold);
    void setEnabled(bool enabled);
    void setCreatedAt(const QDateTime& dt);
    void setLastTriggered(const QDateTime& dt);
    
signals:
    void idChanged();
    void latitudeChanged();
    void longitudeChanged();
    void alertTypeChanged();
    void thresholdChanged();
    void enabledChanged();
    void createdAtChanged();
    void lastTriggeredChanged();
    
private:
    int m_id;
    double m_latitude;
    double m_longitude;
    QString m_alertType;
    double m_threshold;
    bool m_enabled;
    QDateTime m_createdAt;
    QDateTime m_lastTriggered;
};

#endif // ALERTMODEL_H

