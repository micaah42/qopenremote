#include "setting.h"

#include "unistd.h"

QSettings *init()
{
    auto settings = new QSettings{geteuid() == 0 ? QSettings::SystemScope : QSettings::UserScope};
    qInfo() << "init settings file:" << settings->fileName();
    return settings;
}

QSettings &getApplicationSettings()
{
    static QSettings *settings = init();
    return *settings;
}
