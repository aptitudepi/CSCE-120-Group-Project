// main.cpp for Database Service
#include <QCoreApplication>
#include "DatabaseService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DatabaseService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}

#include "DatabaseService.moc"
