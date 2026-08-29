#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>

#include <objectregistry2.h>

class WebSocketServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged FINAL)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged FINAL)

public:
    explicit WebSocketServer(ObjectRegistry2 &registry, QObject *parent = nullptr);

    QString address() const;
    void setAddress(const QString &address);

    quint16 port() const;
    void setPort(quint16 port);

public slots:
    bool open();
    void close();

signals:
    void addressChanged();
    void portChanged();
    void clientConnected(QWebSocket* client);

protected:
    void onNewConnection();

private:
    ObjectRegistry2 &_registry;
    QWebSocketServer *_server;
    QString _address{"127.0.0.1"};
    quint16 _port = 21120;
    QList<QWebSocket *> _sockets;
};

#endif // WEBSOCKETSERVER_H
