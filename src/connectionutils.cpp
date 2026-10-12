#include "connectionutils.h"

void ConnectionUtils::disconnectAndClear(QList<ConnectionPtr> &_connections)
{
    for (auto const &connection : _connections)
        QObject::disconnect(*connection);

    _connections.clear();
}
