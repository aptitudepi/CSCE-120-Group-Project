#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <QString>

/**
 * @brief Minimal .env loader to populate environment variables from a file.
 *
 * The loader reads KEY=VALUE pairs, ignoring blank lines and comments (# ...).
 * Values are trimmed and surrounding quotes are removed. Subsequent calls are
 * no-ops unless forceReload is set to true.
 */
class EnvLoader
{
public:
    static void loadFromFile(const QString& explicitPath = QString(), bool forceReload = false);

private:
    static bool s_loaded;
};

#endif // ENVLOADER_H

