
// LocationService.h
#ifndef LOCATIONSERVICE_H
#define LOCATIONSERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QDateTime>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

class LocationManager : public QObject
{
    Q_OBJECT

public:
    explicit LocationManager(QObject *parent = nullptr);
    ~LocationManager();

    bool initializeDatabase();
    QJsonObject geocodeAddress(const QString &address);
    QJsonObject reverseGeocode(double lat, double lon);
    int saveUserLocation(const QString &userId, const QString &name, double lat, double lon, bool isDefault = false);
    QJsonArray getUserLocations(const QString &userId);

private:
    QNetworkAccessManager *m_networkManager;
    QSqlDatabase m_database;
    QString m_databasePath;

    bool setupTables();
};

class LocationService : public QObject
{
    Q_OBJECT

public:
    explicit LocationService(QObject *parent = nullptr);
    bool start(int port = 8003);
    void stop();

private:
    QHttpServer *m_server;
    LocationManager *m_locationManager;
    QSettings *m_settings;
};

#endif // LOCATIONSERVICE_H
