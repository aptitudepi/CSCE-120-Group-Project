#include "weatherclient.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

WeatherClient::WeatherClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_apiBaseUrl("http://localhost:8000")
    , m_temperature("--°F")
    , m_humidity("--%")
    , m_precipitation("-- in")
    , m_confidence(0.0)
    , m_isLoading(false)
    , m_currentLatitude(30.6280)  // Default: College Station, TX
    , m_currentLongitude(-96.3344)
{
    qDebug() << "WeatherClient initialized";
    
    // Auto-authenticate with demo credentials
    authenticate("demo", "demo123");
}

WeatherClient::~WeatherClient()
{
}

void WeatherClient::setTemperature(const QString &temp)
{
    if (m_temperature != temp) {
        m_temperature = temp;
        emit temperatureChanged();
    }
}

void WeatherClient::setHumidity(const QString &hum)
{
    if (m_humidity != hum) {
        m_humidity = hum;
        emit humidityChanged();
    }
}

void WeatherClient::setPrecipitation(const QString &precip)
{
    if (m_precipitation != precip) {
        m_precipitation = precip;
        emit precipitationChanged();
    }
}

void WeatherClient::setConfidence(double conf)
{
    if (qAbs(m_confidence - conf) > 0.001) {
        m_confidence = conf;
        emit confidenceChanged();
    }
}

void WeatherClient::setIsLoading(bool loading)
{
    if (m_isLoading != loading) {
        m_isLoading = loading;
        emit isLoadingChanged();
    }
}

void WeatherClient::authenticate(const QString &username, const QString &password)
{
    qDebug() << "Authenticating as" << username;
    
    QJsonObject authData;
    authData["username"] = username;
    authData["password"] = password;
    
    QJsonDocument doc(authData);
    QByteArray data = doc.toJson();
    
    QNetworkRequest request(QUrl(m_apiBaseUrl + "/auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &WeatherClient::handleAuthReply);
}

void WeatherClient::handleAuthReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        
        if (obj.contains("access_token")) {
            m_authToken = obj["access_token"].toString();
            qDebug() << "Authentication successful";
            emit authenticationSuccess(m_authToken);
            
            // Automatically fetch initial weather data
            updateLocationData(m_currentLatitude, m_currentLongitude);
        } else {
            qDebug() << "Authentication failed: No token in response";
            emit authenticationFailed("No token in response");
        }
    } else {
        qDebug() << "Authentication error:" << reply->errorString();
        emit authenticationFailed(reply->errorString());
    }
    
    reply->deleteLater();
}

void WeatherClient::updateLocationData(double latitude, double longitude)
{
    if (m_authToken.isEmpty()) {
        qDebug() << "Cannot update location: Not authenticated";
        return;
    }
    
    m_currentLatitude = latitude;
    m_currentLongitude = longitude;
    
    qDebug() << "Updating location:" << latitude << "," << longitude;
    setIsLoading(true);
    
    QString url = QString("%1/api/weather/current?latitude=%2&longitude=%3")
                     .arg(m_apiBaseUrl)
                     .arg(latitude)
                     .arg(longitude);
    
    QNetworkRequest request(QUrl(url));
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &WeatherClient::handleWeatherReply);
}

void WeatherClient::handleWeatherReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    setIsLoading(false);
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        
        // Extract weather data
        if (obj.contains("temperature")) {
            double temp = obj["temperature"].toDouble();
            setTemperature(QString::number(temp, 'f', 1) + "°F");
        }
        
        if (obj.contains("humidity")) {
            double hum = obj["humidity"].toDouble();
            setHumidity(QString::number(hum, 'f', 0) + "%");
        }
        
        if (obj.contains("precipitation")) {
            double precip = obj["precipitation"].toDouble();
            setPrecipitation(QString::number(precip, 'f', 2) + " in");
        }
        
        if (obj.contains("confidence")) {
            double conf = obj["confidence"].toDouble();
            setConfidence(conf);
        }
        
        qDebug() << "Weather data updated successfully";
    } else {
        qDebug() << "Weather request error:" << reply->errorString();
        emit errorOccurred(reply->errorString());
        
        // Set default values on error
        setTemperature("72.0°F");
        setHumidity("65%");
        setPrecipitation("0.00 in");
        setConfidence(0.75);
    }
    
    reply->deleteLater();
}

void WeatherClient::generateHyperlocalForecast(double latitude, double longitude)
{
    if (m_authToken.isEmpty()) {
        qDebug() << "Cannot generate forecast: Not authenticated";
        return;
    }
    
    m_currentLatitude = latitude;
    m_currentLongitude = longitude;
    
    qDebug() << "Generating hyperlocal forecast for:" << latitude << "," << longitude;
    setIsLoading(true);
    
    QJsonObject requestData;
    requestData["latitude"] = latitude;
    requestData["longitude"] = longitude;
    requestData["resolution_km"] = 1;
    requestData["hours_ahead"] = 24;
    
    QJsonDocument doc(requestData);
    QByteArray data = doc.toJson();
    
    QNetworkRequest request(QUrl(m_apiBaseUrl + "/api/forecast/hyperlocal"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    
    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &WeatherClient::handleForecastReply);
}

void WeatherClient::handleForecastReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    setIsLoading(false);
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        
        qDebug() << "Forecast generated successfully";
        emit forecastReady(obj);
        
        // Check for alerts
        if (obj.contains("alerts") && obj["alerts"].isArray()) {
            QJsonArray alerts = obj["alerts"].toArray();
            if (!alerts.isEmpty()) {
                QJsonObject firstAlert = alerts[0].toObject();
                QString alertMessage = firstAlert["message"].toString();
                emit alertTriggered(alertMessage);
            }
        }
        
        // Update current conditions from forecast
        if (obj.contains("current")) {
            QJsonObject current = obj["current"].toObject();
            
            if (current.contains("temperature")) {
                double temp = current["temperature"].toDouble();
                setTemperature(QString::number(temp, 'f', 1) + "°F");
            }
            
            if (current.contains("humidity")) {
                double hum = current["humidity"].toDouble();
                setHumidity(QString::number(hum, 'f', 0) + "%");
            }
            
            if (current.contains("precipitation_probability")) {
                double precip = current["precipitation_probability"].toDouble();
                setPrecipitation(QString::number(precip, 'f', 0) + "%");
            }
        }
        
        if (obj.contains("confidence")) {
            double conf = obj["confidence"].toDouble();
            setConfidence(conf);
        }
    } else {
        qDebug() << "Forecast request error:" << reply->errorString();
        emit errorOccurred(reply->errorString());
    }
    
    reply->deleteLater();
}

void WeatherClient::makeAuthenticatedRequest(const QString &endpoint, const QJsonObject &data)
{
    if (m_authToken.isEmpty()) {
        qDebug() << "Cannot make request: Not authenticated";
        return;
    }
    
    QNetworkRequest request(QUrl(m_apiBaseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    
    if (data.isEmpty()) {
        m_networkManager->get(request);
    } else {
        QJsonDocument doc(data);
        QByteArray requestData = doc.toJson();
        m_networkManager->post(request, requestData);
    }
}
