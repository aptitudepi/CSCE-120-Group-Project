// main.cpp for Alert Service
#include <QCoreApplication>
#include "AlertService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    AlertService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "AlertService.moc"
