#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>

/**
 * @brief Data model representing weather information for a specific location and time
 * 
 * This class stores all weather parameters including temperature, precipitation,
 * wind, and other meteorological data. It follows Qt's object model for easy
 * integration with QML.
 */
class WeatherData : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged)
    Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(double feelsLike READ feelsLike WRITE setFeelsLike NOTIFY feelsLikeChanged)
    Q_PROPERTY(int humidity READ humidity WRITE setHumidity NOTIFY humidityChanged)
    Q_PROPERTY(double pressure READ pressure WRITE setPressure NOTIFY pressureChanged)
    Q_PROPERTY(double windSpeed READ windSpeed WRITE setWindSpeed NOTIFY windSpeedChanged)
    Q_PROPERTY(int windDirection READ windDirection WRITE setWindDirection NOTIFY windDirectionChanged)
    Q_PROPERTY(double precipProbability READ precipProbability WRITE setPrecipProbability NOTIFY precipProbabilityChanged)
    Q_PROPERTY(double precipIntensity READ precipIntensity WRITE setPrecipIntensity NOTIFY precipIntensityChanged)
    Q_PROPERTY(int cloudCover READ cloudCover WRITE setCloudCover NOTIFY cloudCoverChanged)
    Q_PROPERTY(int visibility READ visibility WRITE setVisibility NOTIFY visibilityChanged)
    Q_PROPERTY(int uvIndex READ uvIndex WRITE setUvIndex NOTIFY uvIndexChanged)
    Q_PROPERTY(QString weatherCondition READ weatherCondition WRITE setWeatherCondition NOTIFY weatherConditionChanged)
    Q_PROPERTY(QString weatherDescription READ weatherDescription WRITE setWeatherDescription NOTIFY weatherDescriptionChanged)
    
public:
    explicit WeatherData(QObject *parent = nullptr);
    ~WeatherData() override;
    
    // Getters
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    QDateTime timestamp() const { return m_timestamp; }
    double temperature() const { return m_temperature; }
    double feelsLike() const { return m_feelsLike; }
    int humidity() const { return m_humidity; }
    double pressure() const { return m_pressure; }
    double windSpeed() const { return m_windSpeed; }
    int windDirection() const { return m_windDirection; }
    double precipProbability() const { return m_precipProbability; }
    double precipIntensity() const { return m_precipIntensity; }
    int cloudCover() const { return m_cloudCover; }
    int visibility() const { return m_visibility; }
    int uvIndex() const { return m_uvIndex; }
    QString weatherCondition() const { return m_weatherCondition; }
    QString weatherDescription() const { return m_weatherDescription; }
    
    // Setters
    void setLatitude(double lat);
    void setLongitude(double lon);
    void setTimestamp(const QDateTime& dt);
    void setTemperature(double temp);
    void setFeelsLike(double feels);
    void setHumidity(int humid);
    void setPressure(double press);
    void setWindSpeed(double speed);
    void setWindDirection(int direction);
    void setPrecipProbability(double prob);
    void setPrecipIntensity(double intensity);
    void setCloudCover(int cover);
    void setVisibility(int vis);
    void setUvIndex(int uv);
    void setWeatherCondition(const QString& condition);
    void setWeatherDescription(const QString& description);
    
    // Serialization
    QJsonObject toJson() const;
    static WeatherData* fromJson(const QJsonObject& json, QObject* parent = nullptr);
    
signals:
    void latitudeChanged();
    void longitudeChanged();
    void timestampChanged();
    void temperatureChanged();
    void feelsLikeChanged();
    void humidityChanged();
    void pressureChanged();
    void windSpeedChanged();
    void windDirectionChanged();
    void precipProbabilityChanged();
    void precipIntensityChanged();
    void cloudCoverChanged();
    void visibilityChanged();
    void uvIndexChanged();
    void weatherConditionChanged();
    void weatherDescriptionChanged();
    
private:
    double m_latitude;
    double m_longitude;
    QDateTime m_timestamp;
    double m_temperature;
    double m_feelsLike;
    int m_humidity;
    double m_pressure;
    double m_windSpeed;
    int m_windDirection;
    double m_precipProbability;
    double m_precipIntensity;
    int m_cloudCover;
    int m_visibility;
    int m_uvIndex;
    QString m_weatherCondition;
    QString m_weatherDescription;
};

#endif // WEATHERDATA_H

