// main.cpp for Location Service
#include <QCoreApplication>
#include "LocationService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    LocationService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "LocationService.moc"
