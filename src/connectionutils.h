#ifndef CONNECTIONUTILS_H
#define CONNECTIONUTILS_H

#include <QObject>
#include <QSharedPointer>

namespace ConnectionUtils {

typedef QSharedPointer<QMetaObject::Connection> ConnectionPtr;
void disconnectAndClear(QList<ConnectionPtr> &_connections);

} // namespace ConnectionUtils

#endif // CONNECTIONUTILS_H
