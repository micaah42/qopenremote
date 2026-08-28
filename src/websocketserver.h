#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <objectregistry2.h>

class WebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketServer(ObjectRegistry2 &registry, QObject *parent = nullptr);

signals:
    void clientConnected(QWebSocket* client);

protected:
    void onNewConnection();

private:
    ObjectRegistry2 &_registry;
    QWebSocketServer *_server;
    QList<QWebSocket *> _sockets;
};

#endif // WEBSOCKETSERVER_H
