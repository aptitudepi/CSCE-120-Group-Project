// ApiGatewayService.cpp
#include "ApiGatewayService.h"
#include <QCoreApplication>
#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonArray>
#include <QThread>

ApiGatewayService::ApiGatewayService(QObject *parent)
    : QObject(parent)
    , m_server(new QHttpServer(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_settings(new QSettings(this))
{
    // Load configuration
    m_weatherServiceUrl = m_settings->value("services/weather_data", "http://localhost:8001").toString();
    m_locationServiceUrl = m_settings->value("services/location", "http://localhost:8003").toString();
    m_alertServiceUrl = m_settings->value("services/alert", "http://localhost:8004").toString();
    m_mlServiceUrl = m_settings->value("services/ml", "http://localhost:8002").toString();
    m_dataProcessingUrl = m_settings->value("services/data_processing", "http://localhost:8005").toString();
    m_databaseServiceUrl = m_settings->value("services/database", "http://localhost:8006").toString();

    m_jwtSecret = m_settings->value("jwt/secret", "default-secret-key").toString();

    qDebug() << "API Gateway Service initialized";
}

bool ApiGatewayService::start(int port)
{
    // Root endpoint
    m_server->route("/", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["message"] = "Hyperlocal Weather API Gateway";
        response["status"] = "online";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        return createJsonResponse(response);
    });

    // Authentication endpoint
    m_server->route("/auth/token", QHttpServerRequest::Method::Post, 
                   [this](const QHttpServerRequest &request) {
        return handleAuthentication(request);
    });

    // Weather endpoints with authentication
    m_server->route("/weather/current/<arg>/<arg>", QHttpServerRequest::Method::Get,
                   [this](double lat, double lon, const QHttpServerRequest &request) {
        QString token = extractTokenFromRequest(request);
        if (!validateJwtToken(token)) {
            return createErrorResponse("Authentication required", 401);
        }

        // Proxy to weather service
        QUrl url(m_weatherServiceUrl + QString("/current/%1/%2").arg(lat).arg(lon));
        QNetworkRequest netRequest(url);

        QNetworkReply *reply = m_networkManager->get(netRequest);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            reply->deleteLater();
            return createJsonResponse(doc.object());
        } else {
            reply->deleteLater();
            return createErrorResponse("Weather service unavailable", 503);
        }
    });

    m_server->route("/weather/forecast/<arg>/<arg>", QHttpServerRequest::Method::Get,
                   [this](double lat, double lon, const QHttpServerRequest &request) {
        QString token = extractTokenFromRequest(request);
        if (!validateJwtToken(token)) {
            return createErrorResponse("Authentication required", 401);
        }

        // Get weather data
        QUrl weatherUrl(m_weatherServiceUrl + QString("/forecast/%1/%2").arg(lat).arg(lon));
        QNetworkRequest weatherRequest(weatherUrl);

        QNetworkReply *weatherReply = m_networkManager->get(weatherRequest);
        QEventLoop loop;
        connect(weatherReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (weatherReply->error() != QNetworkReply::NoError) {
            weatherReply->deleteLater();
            return createErrorResponse("Weather service unavailable", 503);
        }

        QJsonDocument weatherDoc = QJsonDocument::fromJson(weatherReply->readAll());
        weatherReply->deleteLater();

        // Process through ML service
        QJsonObject mlRequest;
        mlRequest["weather_data"] = weatherDoc.object();
        mlRequest["latitude"] = lat;
        mlRequest["longitude"] = lon;

        QUrl mlUrl(m_mlServiceUrl + "/predict");
        QNetworkRequest mlNetRequest(mlUrl);
        mlNetRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *mlReply = m_networkManager->post(mlNetRequest, QJsonDocument(mlRequest).toJson());
        connect(mlReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (mlReply->error() == QNetworkReply::NoError) {
            QJsonDocument mlDoc = QJsonDocument::fromJson(mlReply->readAll());
            mlReply->deleteLater();
            return createJsonResponse(mlDoc.object());
        } else {
            mlReply->deleteLater();
            // Return raw weather data if ML service fails
            return createJsonResponse(weatherDoc.object());
        }
    });

    // Location endpoints
    m_server->route("/location/geocode/<arg>", QHttpServerRequest::Method::Get,
                   [this](const QString &address, const QHttpServerRequest &request) {
        QString token = extractTokenFromRequest(request);
        if (!validateJwtToken(token)) {
            return createErrorResponse("Authentication required", 401);
        }

        QUrl url(m_locationServiceUrl + QString("/geocode/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(address))));
        QNetworkRequest netRequest(url);

        QNetworkReply *reply = m_networkManager->get(netRequest);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            reply->deleteLater();
            return createJsonResponse(doc.object());
        } else {
            reply->deleteLater();
            return createErrorResponse("Location service unavailable", 503);
        }
    });

    // Alert subscription endpoint
    m_server->route("/alerts/subscribe", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &request) {
        QString token = extractTokenFromRequest(request);
        if (!validateJwtToken(token)) {
            return createErrorResponse("Authentication required", 401);
        }

        QUrl url(m_alertServiceUrl + "/subscribe");
        QNetworkRequest netRequest(url);
        netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->post(netRequest, request.body());
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            reply->deleteLater();
            return createJsonResponse(doc.object());
        } else {
            reply->deleteLater();
            return createErrorResponse("Alert service unavailable", 503);
        }
    });

    // Health check
    m_server->route("/health", QHttpServerRequest::Method::Get, [this]() {
        QJsonObject response;
        response["status"] = "healthy";
        response["service"] = "api_gateway";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        return createJsonResponse(response);
    });

    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "API Gateway Service started on port" << port;
        return true;
    } else {
        qDebug() << "Failed to start API Gateway Service on port" << port;
        return false;
    }
}

void ApiGatewayService::stop()
{
    m_server->disconnect();
    qDebug() << "API Gateway Service stopped";
}

QHttpServerResponse ApiGatewayService::handleAuthentication(const QHttpServerRequest &request)
{
    QJsonDocument doc = QJsonDocument::fromJson(request.body());
    QJsonObject obj = doc.object();

    QString username = obj["username"].toString();
    QString password = obj["password"].toString();

    // Simple authentication - in production use proper user authentication
    if (username == "demo" && password == "demo123") {
        QString token = generateJwtToken(username);

        QJsonObject response;
        response["access_token"] = token;
        response["token_type"] = "bearer";
        response["expires_in"] = 86400; // 24 hours

        return createJsonResponse(response);
    }

    return createErrorResponse("Invalid credentials", 401);
}

QString ApiGatewayService::generateJwtToken(const QString &username)
{
    // Simple JWT-like token generation (use proper JWT library in production)
    QDateTime expiry = QDateTime::currentDateTime().addDays(1);
    QString payload = QString("%1:%2").arg(username, QString::number(expiry.toSecsSinceEpoch()));

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData((payload + m_jwtSecret).toUtf8());

    return QString("%1.%2").arg(payload.toUtf8().toBase64(), hash.result().toBase64());
}

bool ApiGatewayService::validateJwtToken(const QString &token)
{
    if (token.isEmpty()) return false;

    QStringList parts = token.split('.');
    if (parts.size() != 2) return false;

    QString payload = QString::fromUtf8(QByteArray::fromBase64(parts[0].toUtf8()));
    QString signature = parts[1];

    // Verify signature
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData((payload + m_jwtSecret).toUtf8());
    QString expectedSignature = hash.result().toBase64();

    if (signature != expectedSignature) return false;

    // Check expiry
    QStringList payloadParts = payload.split(':');
    if (payloadParts.size() != 2) return false;

    qint64 expiry = payloadParts[1].toLongLong();
    return QDateTime::currentSecsSinceEpoch() < expiry;
}

QString ApiGatewayService::extractTokenFromRequest(const QHttpServerRequest &request)
{
    QString authorization = request.value("authorization");
    if (authorization.startsWith("Bearer ")) {
        return authorization.mid(7); // Remove "Bearer " prefix
    }
    return QString();
}

QHttpServerResponse ApiGatewayService::createJsonResponse(const QJsonObject &data, int statusCode)
{
    QHttpServerResponse response(QJsonDocument(data).toJson(), QHttpServerResponse::StatusCode(statusCode));
    response.setHeader("Content-Type", "application/json");
    setupCorsHeaders(response);
    return response;
}

QHttpServerResponse ApiGatewayService::createErrorResponse(const QString &error, int statusCode)
{
    QJsonObject errorObj;
    errorObj["error"] = error;
    errorObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return createJsonResponse(errorObj, statusCode);
}

void ApiGatewayService::setupCorsHeaders(QHttpServerResponse &response)
{
    response.setHeader("Access-Control-Allow-Origin", "*");
    response.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

