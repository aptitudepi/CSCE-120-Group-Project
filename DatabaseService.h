
// DatabaseService.h
#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

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
#include <QRedis>
#include <QTimer>
#include <QDebug>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initializeConnections();
    bool initializeSchema();
    int storeWeatherData(const QJsonObject &weatherData);
    QJsonArray getHistoricalWeather(double lat, double lon, int hours = 24);
    bool cacheForecast(const QString &key, const QJsonObject &forecastData, int ttl = 600);
    QJsonObject getCachedForecast(const QString &key);
    bool storeUserSession(const QString &userId, const QJsonObject &sessionData);

private:
    QSqlDatabase m_pgDatabase;
    QRedis *m_redisClient;
    QSettings *m_settings;
    QString m_databaseUrl;
    QString m_redisUrl;

    bool setupPostgreSQLTables();
    bool connectToRedis();
};

class DatabaseService : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseService(QObject *parent = nullptr);
    bool start(int port = 8006);
    void stop();

private:
    QHttpServer *m_server;
    DatabaseManager *m_dbManager;
    QSettings *m_settings;
};

#endif // DATABASESERVICE_H
