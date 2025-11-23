#include "utils/EnvLoader.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

bool EnvLoader::s_loaded = false;

static QString resolveEnvPath(const QString& explicitPath)
{
    if (!explicitPath.isEmpty()) {
        return explicitPath;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        QDir(appDir).absoluteFilePath(".env"),
        QDir(appDir).absoluteFilePath("../.env"),
        QDir::current().absoluteFilePath(".env")
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

void EnvLoader::loadFromFile(const QString& explicitPath, bool forceReload)
{
    if (s_loaded && !forceReload) {
        return;
    }

    QString path = resolveEnvPath(explicitPath);
    if (path.isEmpty()) {
        qDebug() << ".env file not found; skipping environment load";
        s_loaded = true;
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open .env file at" << path;
        s_loaded = true;
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        int equalsIndex = line.indexOf('=');
        if (equalsIndex <= 0) {
            continue;
        }

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        if (value.startsWith('"') && value.endsWith('"') && value.length() >= 2) {
            value = value.mid(1, value.length() - 2);
        } else if (value.startsWith('\'') && value.endsWith('\'') && value.length() >= 2) {
            value = value.mid(1, value.length() - 2);
        }

        if (!key.isEmpty()) {
            qputenv(key.toUtf8(), value.toUtf8());
        }
    }

    s_loaded = true;
}

