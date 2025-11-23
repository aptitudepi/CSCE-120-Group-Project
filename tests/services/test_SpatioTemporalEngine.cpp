#include <gtest/gtest.h>
#include "nowcast/SpatioTemporalEngine.h"
#include "models/WeatherData.h"
#include <QDateTime>
#include <QtMath>

namespace {
WeatherData* makeWeatherData(double lat, double lon, double temp,
                             const QDateTime& timestamp) {
    WeatherData* data = new WeatherData();
    data->setLatitude(lat);
    data->setLongitude(lon);
    data->setTimestamp(timestamp);
    data->setTemperature(temp);
    data->setWindSpeed(10.0);
    data->setHumidity(50);
    data->setPressure(1000.0);
    data->setPrecipIntensity(0.1);
    return data;
}

QDateTime nowUtc() {
    return QDateTime::currentDateTimeUtc().toUTC().addSecs(-QDateTime::currentDateTimeUtc().time().second());
}
} // namespace

TEST(SpatioTemporalEngineTest, SpatialSmoothingHandlesMissingPoints) {
    SpatioTemporalEngine engine;
    auto config = engine.spatialConfig();
    config.strategy = SpatialInterpolator::EqualWeight;
    engine.setSpatialConfig(config);
    QList<WeatherData*> grid;
    QDateTime ts = nowUtc();
    grid.append(makeWeatherData(30.0, -90.0, 70.0, ts)); // center
    grid.append(makeWeatherData(30.01, -90.0, 72.0, ts)); // north
    grid.append(makeWeatherData(29.99, -90.0, 68.0, ts)); // south
    grid.append(makeWeatherData(30.0, -89.99, 74.0, ts)); // east
    grid.append(nullptr); // west missing

    WeatherData* smoothed = engine.applySpatialSmoothing(grid, 30.0, -90.0);
    ASSERT_NE(smoothed, nullptr);
    EXPECT_NEAR(smoothed->temperature(), 71.0, 0.5);

    qDeleteAll(grid);
    delete smoothed;
}

TEST(SpatioTemporalEngineTest, GenerateGridIncludesVerticalOffsets) {
    SpatioTemporalEngine engine;
    QList<QPointF> grid = engine.generateGrid(30.0, -90.0);
    ASSERT_EQ(grid.size(), 7);

    // Expect diagonally offset points present
    bool hasDiagonalUp = false;
    bool hasDiagonalDown = false;
    for (const QPointF& point : grid) {
        if (point.x() > 30.0 && point.y() > -90.0) {
            hasDiagonalUp = true;
        }
        if (point.x() < 30.0 && point.y() < -90.0) {
            hasDiagonalDown = true;
        }
    }
    EXPECT_TRUE(hasDiagonalUp);
    EXPECT_TRUE(hasDiagonalDown);
}

TEST(SpatioTemporalEngineTest, TemporalInterpolationFillsIntermediateSteps) {
    SpatioTemporalEngine engine;
    QList<WeatherData*> raw;
    QDateTime start = nowUtc();
    raw.append(makeWeatherData(30.0, -90.0, 70.0, start));
    raw.append(makeWeatherData(30.0, -90.0, 80.0, start.addSecs(3600)));

    QList<WeatherData*> interpolated = engine.applyTemporalInterpolation(raw);
    ASSERT_FALSE(interpolated.isEmpty());

    bool foundMidpoint = false;
    QDateTime target = start.addSecs(600);
    for (WeatherData* data : interpolated) {
        if (qAbs(data->timestamp().secsTo(target)) <= 0) {
            foundMidpoint = true;
            EXPECT_NEAR(data->temperature(), 71.666, 0.5);
            break;
        }
    }
    EXPECT_TRUE(foundMidpoint);

    qDeleteAll(raw);
    qDeleteAll(interpolated);
}

TEST(SpatioTemporalEngineTest, CombineApisFallsBackWhenMissing) {
    SpatioTemporalEngine engine;
    QMap<QString, QList<WeatherData*>> apiForecasts;
    QDateTime start = nowUtc();

    QList<WeatherData*> apiA;
    apiA.append(makeWeatherData(30.0, -90.0, 70.0, start));
    apiA.append(makeWeatherData(30.0, -90.0, 72.0, start.addSecs(600)));

    QList<WeatherData*> apiB;
    apiB.append(makeWeatherData(30.0, -90.0, 74.0, start));
    // missing the second timestamp

    apiForecasts["API_A"] = apiA;
    apiForecasts["API_B"] = apiB;

    QList<WeatherData*> combined = engine.combineAPIForecasts(apiForecasts);
    ASSERT_EQ(combined.size(), 2);
    EXPECT_NEAR(combined[0]->temperature(), 72.0, 0.1);
    EXPECT_NEAR(combined[1]->temperature(), 72.0, 0.1); // falls back to API_A

    qDeleteAll(combined);
    qDeleteAll(apiA);
    qDeleteAll(apiB);
}

