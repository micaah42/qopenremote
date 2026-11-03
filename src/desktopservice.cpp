#include "desktopservice.h"

#include <QDesktopServices>
#include <QFile>
#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(self, "desktopservice")
}

QString DesktopService::readFile(const QString &filePath)
{
    QFile file{filePath};
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(self) << "failed to open file:" << filePath << file.errorString();
        return QString();
    }

    return file.readAll();
}

bool DesktopService::openUrl(const QUrl &url) const
{
    return QDesktopServices::openUrl(url);
}
