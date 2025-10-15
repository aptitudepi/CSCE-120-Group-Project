// main.cpp for Weather Data Service
#include <QCoreApplication>
#include "WeatherDataService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    WeatherDataService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "WeatherDataService.moc"
