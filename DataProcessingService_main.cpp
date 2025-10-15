// main.cpp for Data Processing Service
#include <QCoreApplication>
#include "DataProcessingService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DataProcessingService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "DataProcessingService.moc"
