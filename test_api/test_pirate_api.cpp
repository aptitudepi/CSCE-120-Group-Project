#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>
#include <QTimer>

#include <cstdio>

int main(int argc, char *argv[])
{
    printf("Starting TestPirateAPI...\n");
    fflush(stdout);

    QCoreApplication app(argc, argv);

    QString apiKey = "6fyepOdzDm02NMczwko9y6FlHmJXQAmG";
    double lat = 30.628;
    double lon = -96.3344;
    QString urlStr = QString("https://api.pirateweather.net/forecast/%1/%2,%3")
                         .arg(apiKey, QString::number(lat, 'f', 4), QString::number(lon, 'f', 4));

    printf("Requesting URL: %s\n", qPrintable(urlStr));
    fflush(stdout);

    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(urlStr)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "HyperlocalWeather/1.0");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (reply->error() != QNetworkReply::NoError) {
            printf("Error: %s\n", qPrintable(reply->errorString()));
        } else {
            QByteArray data = reply->readAll();
            printf("Success! Received %lld bytes\n", data.size());
            printf("Response snippet: %s\n", data.left(200).constData());
        }
        fflush(stdout);
        reply->deleteLater();
        app.quit();
    });

    // Timeout after 10 seconds
    QTimer::singleShot(10000, [&]() {
        printf("Timeout reached!\n");
        fflush(stdout);
        if (reply->isRunning()) {
            reply->abort();
        }
        app.quit();
    });

    return app.exec();
}
