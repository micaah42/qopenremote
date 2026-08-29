#include "websocketserver.h"

#include <QLoggingCategory>

#include <jsonadapter.h>

namespace {
Q_LOGGING_CATEGORY(self, "server", QtWarningMsg)
}

WebSocketServer::WebSocketServer(ObjectRegistry2 &registry, QObject *parent)
    : QObject{parent}
    , _registry{registry}
    , _server{new QWebSocketServer{"talking-clock", QWebSocketServer::NonSecureMode, this}}
{
    connect(_server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

QString WebSocketServer::address() const
{
    return _address;
}

void WebSocketServer::setAddress(const QString &address)
{
    if (_address == address)
        return;

    _address = address;
    emit addressChanged();
}

quint16 WebSocketServer::port() const
{
    return _port;
}

void WebSocketServer::setPort(quint16 port)
{
    if (_port == port)
        return;

    _port = port;
    emit portChanged();
}

bool WebSocketServer::open()
{
    if (_server->isListening())
        return true;

    if (!_server->listen(QHostAddress(_address), _port)) {
        qCCritical(self) << "failed to start websocket server:" << _server->errorString();
        return false;
    }

    return true;
}

void WebSocketServer::close()
{
    _server->close();
}

void WebSocketServer::onNewConnection()
{
    if (!_server->hasPendingConnections())
        return;

    auto socket = _server->nextPendingConnection();
    auto adapter = new JSONAdapter{_registry, socket};
    qCInfo(self) << "client connected" << socket;

    emit clientConnected(socket);

    connect(socket, &QWebSocket::textMessageReceived, adapter, [adapter](const QString &message) { adapter->handleMessage(message.toUtf8()); });
    connect(adapter, &JSONAdapter::sendMessage, socket, [socket](const QByteArray &message) { socket->sendTextMessage(message); });

    connect(socket, &QWebSocket::disconnected, this, [socket]() {
        qCInfo(self) << "client disconnected:" << socket;
        socket->deleteLater();
    });
}
