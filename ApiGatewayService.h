
// ApiGatewayService.h
#ifndef APIGATEWAYSERVICE_H
#define APIGATEWAYSERVICE_H

#include <QObject>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDateTime>
#include <QCryptographicHash>
#include <QJwtToken>
#include <QSettings>

class ApiGatewayService : public QObject
{
    Q_OBJECT

public:
    explicit ApiGatewayService(QObject *parent = nullptr);
    bool start(int port = 8000);
    void stop();

private slots:
    void handleRequest(const QHttpServerRequest &request);

private:
    // Authentication methods
    QHttpServerResponse handleAuthentication(const QHttpServerRequest &request);
    QString generateJwtToken(const QString &username);
    bool validateJwtToken(const QString &token);
    QString extractTokenFromRequest(const QHttpServerRequest &request);

    // Service proxy methods
    QHttpServerResponse proxyToWeatherService(const QHttpServerRequest &request);
    QHttpServerResponse proxyToLocationService(const QHttpServerRequest &request);
    QHttpServerResponse proxyToAlertService(const QHttpServerRequest &request);

    // Helper methods
    QHttpServerResponse createJsonResponse(const QJsonObject &data, int statusCode = 200);
    QHttpServerResponse createErrorResponse(const QString &error, int statusCode = 400);
    void setupCorsHeaders(QHttpServerResponse &response);

    QHttpServer *m_server;
    QNetworkAccessManager *m_networkManager;
    QSettings *m_settings;

    // Service URLs
    QString m_weatherServiceUrl;
    QString m_locationServiceUrl;
    QString m_alertServiceUrl;
    QString m_mlServiceUrl;
    QString m_dataProcessingUrl;
    QString m_databaseServiceUrl;

    QString m_jwtSecret;
};

#endif // APIGATEWAYSERVICE_H
