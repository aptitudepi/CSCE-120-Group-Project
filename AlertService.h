
// AlertService.h
#ifndef ALERTSERVICE_H
#define ALERTSERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSmtp>
#include <QtMath>

class AlertManager : public QObject
{
    Q_OBJECT

public:
    explicit AlertManager(QObject *parent = nullptr);
    ~AlertManager();

    bool initializeDatabase();
    int createSubscription(const QJsonObject &subscriptionData);
    QJsonArray getUserSubscriptions(const QString &userId);
    QJsonArray checkAlertConditions(const QJsonObject &weatherPrediction, const QJsonObject &subscription);
    bool sendEmailAlert(const QString &email, const QJsonObject &alert);
    void saveAlertHistory(int subscriptionId, const QJsonObject &alert, const QJsonObject &weatherData, const QString &status = "sent");

private:
    QSqlDatabase m_database;
    QString m_databasePath;
    QSettings *m_emailSettings;

    bool setupTables();
};

class AlertService : public QObject
{
    Q_OBJECT

public:
    explicit AlertService(QObject *parent = nullptr);
    bool start(int port = 8004);
    void stop();

private:
    QHttpServer *m_server;
    AlertManager *m_alertManager;
    QSettings *m_settings;
};

#endif // ALERTSERVICE_H
