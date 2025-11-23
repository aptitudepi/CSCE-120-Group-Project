#ifndef WEATHERBITSERVICE_H
#define WEATHERBITSERVICE_H

#include "services/WeatherService.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QJsonObject>
#include <QtConcurrent>

/**
 * @brief Fetches forecast and current weather data from the Weatherbit API.
 *
 * Weatherbit provides high resolution hourly forecasts that complement the
 * existing NWS and Pirate Weather sources. This service focuses on the
 * 48-hour hourly forecast endpoint and the realtime current endpoint.
 */
class WeatherbitService : public WeatherService
{
    Q_OBJECT

public:
    explicit WeatherbitService(QObject* parent = nullptr);
    ~WeatherbitService() override;

    void fetchForecast(double latitude, double longitude) override;
    void fetchCurrent(double latitude, double longitude) override;
    QString serviceName() const override { return "Weatherbit"; }

    void setApiKey(const QString& apiKey);
    bool hasApiKey() const { return !m_apiKey.isEmpty(); }
    bool isAvailable() const override { return hasApiKey(); }

    void cancelActiveRequests();

private slots:
    void onReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    enum ReplyType {
        ForecastReply = 0,
        CurrentReply
    };

    QNetworkRequest buildRequest(const QUrl& url) const;
    QUrl buildForecastUrl(double latitude, double longitude) const;
    QUrl buildCurrentUrl(double latitude, double longitude) const;
    void registerReply(QNetworkReply* reply, ReplyType type, double latitude, double longitude);
    void unregisterReply(QNetworkReply* reply);
    QList<WeatherData*> parseForecastPayload(const QByteArray& payload,
                                             double latitude,
                                             double longitude);
    WeatherData* parseCurrentPayload(const QByteArray& payload,
                                     double latitude,
                                     double longitude);
    WeatherData* parseDataPoint(const QJsonObject& obj,
                                double latitude,
                                double longitude) const;

    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;
    QSet<QNetworkReply*> m_activeReplies;

    static const QString API_BASE_URL;
};

#endif // WEATHERBITSERVICE_H


