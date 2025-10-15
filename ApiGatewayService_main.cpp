// main.cpp for API Gateway Service
#include <QCoreApplication>
#include "ApiGatewayService.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ApiGatewayService service;
    if (!service.start()) {
        return -1;
    }

    return app.exec();
}
