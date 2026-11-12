#ifndef CONNECTIONUTILS_H
#define CONNECTIONUTILS_H

#include "qopenremote_global.h"

#include <QObject>
#include <QSharedPointer>

namespace ConnectionUtils {

typedef QSharedPointer<QMetaObject::Connection> ConnectionPtr;
QOPENREMOTE_EXPORT void disconnectAndClear(QList<ConnectionPtr> &_connections);

} // namespace ConnectionUtils

#endif // CONNECTIONUTILS_H
