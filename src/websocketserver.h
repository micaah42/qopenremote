#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <qobjectregistry.h>

class WebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketServer(QObjectRegistry &registry, QObject *parent = nullptr);

signals:
    void clientConnected(QWebSocket* client);

private slots:
    void onNewConnection();

private:
    QObjectRegistry &_registry;
    QWebSocketServer _server;
    QList<QWebSocket *> _sockets;
};

#endif // WEBSOCKETSERVER_H
