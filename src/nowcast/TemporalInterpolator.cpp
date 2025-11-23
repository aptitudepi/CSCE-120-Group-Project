#include "nowcast/TemporalInterpolator.h"
#include <QDebug>
#include <QtMath>
#include <QTime>
#include <algorithm>

// Forward declaration of helper function
static void copyWeatherData(WeatherData* dest, const WeatherData* src);
static QDateTime roundToNearestMinute(const QDateTime& dt);

TemporalInterpolator::TemporalInterpolator(QObject *parent)
    : QObject(parent)
    , m_interpolationMethod(Linear)
    , m_smoothingMethod(None)
    , m_smoothingWindowMinutes(30)
{
}

TemporalInterpolator::~TemporalInterpolator() = default;

QList<WeatherData*> TemporalInterpolator::interpolate(const QList<WeatherData*>& sourceData,
                                                        int outputGranularityMinutes,
                                                        InterpolationMethod method)
{
    if (sourceData.isEmpty()) {
        return QList<WeatherData*>();
    }
    
    if (sourceData.size() == 1) {
        // Can't interpolate with single point, return copy
        WeatherData* copy = new WeatherData();
        copy->setLatitude(sourceData.first()->latitude());
        copy->setLongitude(sourceData.first()->longitude());
        copy->setTimestamp(sourceData.first()->timestamp());
        copy->setTemperature(sourceData.first()->temperature());
        copy->setFeelsLike(sourceData.first()->feelsLike());
        copy->setHumidity(sourceData.first()->humidity());
        copy->setPressure(sourceData.first()->pressure());
        copy->setWindSpeed(sourceData.first()->windSpeed());
        copy->setWindDirection(sourceData.first()->windDirection());
        copy->setPrecipProbability(sourceData.first()->precipProbability());
        copy->setPrecipIntensity(sourceData.first()->precipIntensity());
        copy->setCloudCover(sourceData.first()->cloudCover());
        copy->setVisibility(sourceData.first()->visibility());
        copy->setUvIndex(sourceData.first()->uvIndex());
        copy->setWeatherCondition(sourceData.first()->weatherCondition());
        copy->setWeatherDescription(sourceData.first()->weatherDescription());
        return QList<WeatherData*>() << copy;
    }
    
    QList<WeatherData*> sorted;
    for (WeatherData* sample : sourceData) {
        if (sample) {
            sorted.append(sample);
        }
    }

    if (sorted.isEmpty()) {
        return QList<WeatherData*>();
    }

    std::sort(sorted.begin(), sorted.end(), [](const WeatherData* a, const WeatherData* b) {
        return a->timestamp() < b->timestamp();
    });

    const int granularityMinutes = qMax(1, outputGranularityMinutes);
    const QDateTime nowRounded = roundToNearestMinute(QDateTime::currentDateTimeUtc());

    QDateTime startTime = qMax(nowRounded, roundToNearestMinute(sorted.first()->timestamp()));
    QDateTime nextForecast;
    for (WeatherData* sample : sorted) {
        QDateTime candidate = roundToNearestMinute(sample->timestamp());
        if (candidate > startTime) {
            nextForecast = candidate;
            break;
        }
    }
    if (!nextForecast.isValid()) {
        nextForecast = roundToNearestMinute(sorted.last()->timestamp());
        if (nextForecast <= startTime) {
            nextForecast = startTime.addSecs(granularityMinutes * 60);
        }
    }

    QDateTime endTime = qMax(startTime, nextForecast);

    QList<WeatherData*> result;
    QDateTime currentTime = startTime;
    while (currentTime <= endTime) {
        WeatherData* interpolated = nullptr;
        
        // Find bracketing points
        const WeatherData* before = nullptr;
        const WeatherData* after = nullptr;
        
        if (findBracketingPoints(sorted, currentTime, before, after)) {
            if (before && after) {
                // Interpolate between points
                if (method == Linear) {
                    interpolated = linearInterpolate(before, after, currentTime);
                } else if (method == StepFunction) {
                    // Use nearest neighbor
                    qint64 distBefore = qAbs(before->timestamp().secsTo(currentTime));
                    qint64 distAfter = qAbs(after->timestamp().secsTo(currentTime));
                    
                    interpolated = new WeatherData();
                    if (distBefore < distAfter) {
                        copyWeatherData(interpolated, before);
                    } else {
                        copyWeatherData(interpolated, after);
                    }
                    interpolated->setTimestamp(currentTime);
                }
            } else if (before) {
                // Past last point, use last value
                interpolated = new WeatherData();
                copyWeatherData(interpolated, before);
                interpolated->setTimestamp(currentTime);
            } else if (after) {
                // Before first point, use first value
                interpolated = new WeatherData();
                copyWeatherData(interpolated, after);
                interpolated->setTimestamp(currentTime);
            }
        }
        
        if (interpolated) {
            result.append(interpolated);
        }
        
        currentTime = currentTime.addSecs(granularityMinutes * 60);
    }
    
    return result;
}

QList<WeatherData*> TemporalInterpolator::smooth(const QList<WeatherData*>& data,
                                                   int windowMinutes,
                                                   SmoothingMethod method)
{
    if (data.isEmpty() || method == None) {
        // Return copies
        QList<WeatherData*> result;
        for (const WeatherData* item : data) {
            WeatherData* copy = new WeatherData();
            copyWeatherData(copy, item);
            result.append(copy);
        }
        return result;
    }
    
    QList<WeatherData*> result;
    int windowSize = qMax(1, windowMinutes);
    
    for (int i = 0; i < data.size(); ++i) {
        WeatherData* smoothed = new WeatherData();
        copyWeatherData(smoothed, data[i]); // Copy base data
        
        if (method == SimpleMovingAverage) {
            // Calculate average of surrounding points
            int startIdx = qMax(0, i - windowSize / 2);
            int endIdx = qMin(data.size() - 1, i + windowSize / 2);
            
            double sumTemp = 0.0, sumPrecip = 0.0, sumWind = 0.0;
            int count = 0;
            
            for (int j = startIdx; j <= endIdx; ++j) {
                sumTemp += data[j]->temperature();
                sumPrecip += data[j]->precipIntensity();
                sumWind += data[j]->windSpeed();
                count++;
            }
            
            if (count > 0) {
                smoothed->setTemperature(sumTemp / count);
                smoothed->setPrecipIntensity(sumPrecip / count);
                smoothed->setWindSpeed(sumWind / count);
            }
        } else if (method == ExponentialMovingAverage) {
            // EMA with alpha = 2 / (windowSize + 1)
            double alpha = 2.0 / (windowSize + 1);
            
            if (i > 0 && !result.isEmpty()) {
                WeatherData* prev = result.last();
                smoothed->setTemperature(alpha * data[i]->temperature() + (1 - alpha) * prev->temperature());
                smoothed->setPrecipIntensity(alpha * data[i]->precipIntensity() + (1 - alpha) * prev->precipIntensity());
                smoothed->setWindSpeed(alpha * data[i]->windSpeed() + (1 - alpha) * prev->windSpeed());
            }
        }
        
        result.append(smoothed);
    }
    
    return result;
}

QList<WeatherData*> TemporalInterpolator::alignToGrid(const QList<WeatherData*>& data,
                                                        const QDateTime& gridStartTime,
                                                        int gridIntervalMinutes,
                                                        int gridPointCount)
{
    if (data.isEmpty()) {
        return QList<WeatherData*>();
    }
    
    QList<WeatherData*> result;
    QDateTime gridTime = gridStartTime;
    
    for (int i = 0; i < gridPointCount; ++i) {
        const WeatherData* before = nullptr;
        const WeatherData* after = nullptr;
        
        if (findBracketingPoints(data, gridTime, before, after)) {
            WeatherData* aligned = nullptr;
            
            if (before && after) {
                aligned = linearInterpolate(before, after, gridTime);
            } else if (before) {
                aligned = new WeatherData();
                copyWeatherData(aligned, before);
                aligned->setTimestamp(gridTime);
            } else if (after) {
                aligned = new WeatherData();
                copyWeatherData(aligned, after);
                aligned->setTimestamp(gridTime);
            }
            
            if (aligned) {
                result.append(aligned);
            }
        }
        
        gridTime = gridTime.addSecs(gridIntervalMinutes * 60);
    }
    
    return result;
}

WeatherData* TemporalInterpolator::linearInterpolate(const WeatherData* before,
                                                       const WeatherData* after,
                                                       const QDateTime& targetTime)
{
    if (!before || !after) {
        return nullptr;
    }
    
    WeatherData* result = new WeatherData();
    result->setTimestamp(targetTime);
    result->setLatitude(before->latitude());
    result->setLongitude(before->longitude());
    
    qint64 timeBefore = before->timestamp().toSecsSinceEpoch();
    qint64 timeAfter = after->timestamp().toSecsSinceEpoch();
    qint64 timeTarget = targetTime.toSecsSinceEpoch();
    
    // Interpolate all numeric fields
    result->setTemperature(interpolateValue(before->temperature(), after->temperature(),
                                            timeBefore, timeAfter, timeTarget));
    result->setFeelsLike(interpolateValue(before->feelsLike(), after->feelsLike(),
                                          timeBefore, timeAfter, timeTarget));
    result->setHumidity(static_cast<int>(interpolateValue(before->humidity(), after->humidity(),
                                                          timeBefore, timeAfter, timeTarget)));
    result->setPressure(interpolateValue(before->pressure(), after->pressure(),
                                        timeBefore, timeAfter, timeTarget));
    result->setWindSpeed(interpolateValue(before->windSpeed(), after->windSpeed(),
                                         timeBefore, timeAfter, timeTarget));
    result->setWindDirection(static_cast<int>(interpolateValue(before->windDirection(), after->windDirection(),
                                                               timeBefore, timeAfter, timeTarget)));
    result->setPrecipProbability(interpolateValue(before->precipProbability(), after->precipProbability(),
                                                   timeBefore, timeAfter, timeTarget));
    result->setPrecipIntensity(interpolateValue(before->precipIntensity(), after->precipIntensity(),
                                               timeBefore, timeAfter, timeTarget));
    result->setCloudCover(static_cast<int>(interpolateValue(before->cloudCover(), after->cloudCover(),
                                                            timeBefore, timeAfter, timeTarget)));
    result->setVisibility(static_cast<int>(interpolateValue(before->visibility(), after->visibility(),
                                                            timeBefore, timeAfter, timeTarget)));
    result->setUvIndex(static_cast<int>(interpolateValue(before->uvIndex(), after->uvIndex(),
                                                         timeBefore, timeAfter, timeTarget)));
    
    // Use "before" value for categorical fields
    result->setWeatherCondition(before->weatherCondition());
    result->setWeatherDescription(before->weatherDescription());
    
    return result;
}

double TemporalInterpolator::interpolateValue(double valueBefore, double valueAfter,
                                              qint64 timeBefore, qint64 timeAfter,
                                              qint64 targetTime)
{
    if (timeBefore == timeAfter) {
        return valueBefore;
    }
    
    double fraction = static_cast<double>(targetTime - timeBefore) / 
                     static_cast<double>(timeAfter - timeBefore);
    
    return valueBefore + fraction * (valueAfter - valueBefore);
}


bool TemporalInterpolator::findBracketingPoints(const QList<WeatherData*>& data,
                                                const QDateTime& targetTime,
                                                const WeatherData*& before,
                                                const WeatherData*& after)
{
    before = nullptr;
    after = nullptr;
    
    for (int i = 0; i < data.size(); ++i) {
        QDateTime dataTime = data[i]->timestamp();
        
        if (dataTime <= targetTime) {
            before = data[i];
        }
        
        if (dataTime >= targetTime && !after) {
            after = data[i];
            break;
        }
    }
    
    return before != nullptr || after != nullptr;
}

// Helper function to copy WeatherData (QObject doesn't support assignment)
void copyWeatherData(WeatherData* dest, const WeatherData* src) {
    dest->setLatitude(src->latitude());
    dest->setLongitude(src->longitude());
    dest->setTimestamp(src->timestamp());
    dest->setTemperature(src->temperature());
    dest->setFeelsLike(src->feelsLike());
    dest->setHumidity(src->humidity());
    dest->setPressure(src->pressure());
    dest->setWindSpeed(src->windSpeed());
    dest->setWindDirection(src->windDirection());
    dest->setPrecipProbability(src->precipProbability());
    dest->setPrecipIntensity(src->precipIntensity());
    dest->setCloudCover(src->cloudCover());
    dest->setVisibility(src->visibility());
    dest->setUvIndex(src->uvIndex());
    dest->setWeatherCondition(src->weatherCondition());
    dest->setWeatherDescription(src->weatherDescription());
}

QDateTime roundToNearestMinute(const QDateTime& dt) {
    QDateTime rounded = dt;
    int msecs = rounded.time().msec();
    if (msecs != 0) {
        rounded = rounded.addMSecs(-msecs);
    }
    int secs = rounded.time().second();
    if (secs >= 30) {
        rounded = rounded.addSecs(60 - secs);
    } else {
        rounded = rounded.addSecs(-secs);
    }
    rounded.setTime(QTime(rounded.time().hour(), rounded.time().minute(), 0, 0));
    return rounded;
}

