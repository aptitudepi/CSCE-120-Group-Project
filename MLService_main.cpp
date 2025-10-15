// main.cpp for ML Service
#include <QCoreApplication>
#include "MLService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    MLService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "MLService.moc"
