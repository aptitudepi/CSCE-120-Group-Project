#include "models/AlertModel.h"

AlertModel::AlertModel(QObject *parent)
    : QObject(parent)
    , m_id(-1)
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_alertType("")
    , m_threshold(0.0)
    , m_enabled(true)
    , m_createdAt(QDateTime::currentDateTime())
{
}

AlertModel::~AlertModel() = default;

void AlertModel::setId(int id) {
    if (m_id != id) {
        m_id = id;
        emit idChanged();
    }
}

void AlertModel::setLatitude(double lat) {
    if (qAbs(m_latitude - lat) > 0.0001) {
        m_latitude = lat;
        emit latitudeChanged();
    }
}

void AlertModel::setLongitude(double lon) {
    if (qAbs(m_longitude - lon) > 0.0001) {
        m_longitude = lon;
        emit longitudeChanged();
    }
}

void AlertModel::setAlertType(const QString& type) {
    if (m_alertType != type) {
        m_alertType = type;
        emit alertTypeChanged();
    }
}

void AlertModel::setThreshold(double threshold) {
    if (qAbs(m_threshold - threshold) > 0.01) {
        m_threshold = threshold;
        emit thresholdChanged();
    }
}

void AlertModel::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
    }
}

void AlertModel::setCreatedAt(const QDateTime& dt) {
    if (m_createdAt != dt) {
        m_createdAt = dt;
        emit createdAtChanged();
    }
}

void AlertModel::setLastTriggered(const QDateTime& dt) {
    if (m_lastTriggered != dt) {
        m_lastTriggered = dt;
        emit lastTriggeredChanged();
    }
}

