#include "nowcast/SpatioTemporalEngine.h"
#include <QSet>
#include <QMap>
#include <QDebug>

SpatioTemporalEngine::SpatioTemporalEngine(QObject *parent)
    : QObject(parent)
    , m_spatialInterpolator(new SpatialInterpolator())
    , m_temporalInterpolator(new TemporalInterpolator(this))
{
    m_apiWeights.weights["NWS"] = 0.5;
    m_apiWeights.weights["PirateWeather"] = 0.5;
}

SpatioTemporalEngine::~SpatioTemporalEngine()
{
    delete m_spatialInterpolator;
}

QList<QPointF> SpatioTemporalEngine::generateGrid(double centerLat, double centerLon)
{
    QList<QPointF> gridPoints;
    gridPoints.append(QPointF(centerLat, centerLon)); // Center

    const double latOffset = kmToLatDegrees(m_gridConfig.offsetDistanceKm);
    const double lonOffset = kmToLonDegrees(m_gridConfig.offsetDistanceKm, centerLat);

    // Cardinal points
    gridPoints.append(QPointF(centerLat + latOffset, centerLon)); // North
    gridPoints.append(QPointF(centerLat - latOffset, centerLon)); // South
    gridPoints.append(QPointF(centerLat, centerLon + lonOffset)); // East
    gridPoints.append(QPointF(centerLat, centerLon - lonOffset)); // West

    // “Vertical” offsets interpreted as diagonal perturbations to capture up/down variability
    gridPoints.append(QPointF(centerLat + latOffset, centerLon + lonOffset)); // Up
    gridPoints.append(QPointF(centerLat - latOffset, centerLon - lonOffset)); // Down

    emit gridGenerated(gridPoints);
    return gridPoints;
}

WeatherData* SpatioTemporalEngine::applySpatialSmoothing(const QList<WeatherData*>& gridForecasts,
                                                         double centerLat, double centerLon)
{
    if (gridForecasts.isEmpty()) {
        return nullptr;
    }

    QList<GridPoint> tempPoints;
    QList<GridPoint> precipPoints;
    QList<GridPoint> windPoints;
    QList<GridPoint> humidityPoints;

    for (const WeatherData* sample : gridForecasts) {
        if (!sample) {
            continue;
        }
        tempPoints.append(GridPoint(sample->latitude(), sample->longitude(), sample->temperature(), true));
        precipPoints.append(GridPoint(sample->latitude(), sample->longitude(), sample->precipIntensity(), true));
        windPoints.append(GridPoint(sample->latitude(), sample->longitude(), sample->windSpeed(), true));
        humidityPoints.append(GridPoint(sample->latitude(), sample->longitude(), sample->humidity(), true));
    }

    tempPoints = m_spatialInterpolator->handleMissingPoints(tempPoints, m_spatialConfig.missingPointsThreshold);
    precipPoints = m_spatialInterpolator->handleMissingPoints(precipPoints, m_spatialConfig.missingPointsThreshold);
    windPoints = m_spatialInterpolator->handleMissingPoints(windPoints, m_spatialConfig.missingPointsThreshold);
    humidityPoints = m_spatialInterpolator->handleMissingPoints(humidityPoints, m_spatialConfig.missingPointsThreshold);

    if (tempPoints.isEmpty()) {
        return nullptr;
    }

    WeatherData* output = new WeatherData();
    output->setLatitude(centerLat);
    output->setLongitude(centerLon);
    output->setTimestamp(gridForecasts.first()->timestamp());

    output->setTemperature(m_spatialInterpolator->interpolateWeighted(
        centerLat, centerLon, tempPoints, m_spatialConfig.strategy, m_spatialConfig.idwPower));

    output->setPrecipIntensity(qMax(0.0, m_spatialInterpolator->interpolateWeighted(
        centerLat, centerLon, precipPoints, m_spatialConfig.strategy, m_spatialConfig.idwPower)));

    output->setWindSpeed(m_spatialInterpolator->interpolateWeighted(
        centerLat, centerLon, windPoints, m_spatialConfig.strategy, m_spatialConfig.idwPower));

    output->setHumidity(static_cast<int>(qRound(m_spatialInterpolator->interpolateWeighted(
        centerLat, centerLon, humidityPoints, m_spatialConfig.strategy, m_spatialConfig.idwPower))));

    const WeatherData* reference = gridForecasts.first();
    output->setFeelsLike(reference->feelsLike());
    output->setPressure(reference->pressure());
    output->setPrecipProbability(reference->precipProbability());
    output->setWindDirection(reference->windDirection());
    output->setCloudCover(reference->cloudCover());
    output->setVisibility(reference->visibility());
    output->setUvIndex(reference->uvIndex());
    output->setWeatherCondition(reference->weatherCondition());
    output->setWeatherDescription(reference->weatherDescription());

    emit spatialSmoothingComplete();
    return output;
}

QList<WeatherData*> SpatioTemporalEngine::applyTemporalInterpolation(const QList<WeatherData*>& apiForecasts)
{
    if (apiForecasts.isEmpty()) {
        return {};
    }

    QList<WeatherData*> filtered = apiForecasts;
    const QDateTime nowRounded = QDateTime::currentDateTimeUtc().toUTC();
    QList<WeatherData*> trimmed;
    for (WeatherData* data : filtered) {
        if (!data) {
            continue;
        }
        trimmed.append(data);
    }

    if (trimmed.isEmpty()) {
        return {};
    }

    QList<WeatherData*> interpolated = m_temporalInterpolator->interpolate(
        trimmed,
        qMax(1, m_temporalConfig.outputGranularityMinutes),
        m_temporalConfig.method);

    if (m_temporalConfig.smoothingWindowMinutes > 0) {
        QList<WeatherData*> smoothed = m_temporalInterpolator->smooth(
            interpolated,
            m_temporalConfig.smoothingWindowMinutes,
            TemporalInterpolator::SimpleMovingAverage);
        qDeleteAll(interpolated);
        interpolated = smoothed;
    }

    emit temporalInterpolationComplete();
    return interpolated;
}

QList<WeatherData*> SpatioTemporalEngine::combineAPIForecasts(const QMap<QString, QList<WeatherData*>>& apiForecasts)
{
    if (apiForecasts.isEmpty()) {
        return {};
    }

    QSet<QDateTime> timeline;
    for (const QList<WeatherData*>& series : apiForecasts) {
        for (WeatherData* sample : series) {
            if (sample) {
                timeline.insert(sample->timestamp());
            }
        }
    }

    QList<QDateTime> sortedTimes = timeline.values();
    std::sort(sortedTimes.begin(), sortedTimes.end());

    QList<WeatherData*> combined;
    for (const QDateTime& ts : sortedTimes) {
        double totalWeight = 0.0;
        double sumTemp = 0.0;
        double sumPrecip = 0.0;
        double sumWind = 0.0;
        double sumHumidity = 0.0;
        double sumPressure = 0.0;

        WeatherData* reference = nullptr;

        for (auto it = apiForecasts.constBegin(); it != apiForecasts.constEnd(); ++it) {
            const QString apiName = it.key();
            const QList<WeatherData*>& series = it.value();

            WeatherData* match = nullptr;
            for (WeatherData* sample : series) {
                if (sample && sample->timestamp() == ts) {
                    match = sample;
                    break;
                }
            }

            if (!match) {
                continue;
            }

            if (!reference) {
                reference = match;
            }

            const double weight = m_apiWeights.weights.value(apiName, 1.0);
            totalWeight += weight;
            sumTemp += match->temperature() * weight;
            sumPrecip += match->precipIntensity() * weight;
            sumWind += match->windSpeed() * weight;
            sumHumidity += match->humidity() * weight;
            sumPressure += match->pressure() * weight;
        }

        if (!reference || totalWeight <= 0.0) {
            continue;
        }

        const double norm = 1.0 / totalWeight;
        WeatherData* blended = new WeatherData();
        blended->setTimestamp(ts);
        blended->setLatitude(reference->latitude());
        blended->setLongitude(reference->longitude());
        blended->setTemperature(sumTemp * norm);
        blended->setPrecipIntensity(qMax(0.0, sumPrecip * norm));
        blended->setWindSpeed(sumWind * norm);
        blended->setHumidity(static_cast<int>(qRound(sumHumidity * norm)));
        blended->setPressure(sumPressure * norm);
        blended->setFeelsLike(reference->feelsLike());
        blended->setPrecipProbability(reference->precipProbability());
        blended->setWindDirection(reference->windDirection());
        blended->setCloudCover(reference->cloudCover());
        blended->setVisibility(reference->visibility());
        blended->setUvIndex(reference->uvIndex());
        blended->setWeatherCondition(reference->weatherCondition());
        blended->setWeatherDescription(reference->weatherDescription());

        combined.append(blended);
    }

    if (combined.isEmpty()) {
        emit error("Combined forecast is empty");
    } else {
        emit forecastReady(combined);
    }

    return combined;
}

double SpatioTemporalEngine::kmToLatDegrees(double distanceKm)
{
    return distanceKm / 111.0;
}

double SpatioTemporalEngine::kmToLonDegrees(double distanceKm, double latitude)
{
    const double kmPerDegree = 111.0 * qCos(qDegreesToRadians(latitude));
    if (qFuzzyIsNull(kmPerDegree)) {
        return 0.0;
    }
    return distanceKm / kmPerDegree;
}

