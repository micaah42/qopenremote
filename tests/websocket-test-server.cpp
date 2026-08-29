#include <QCommandLineParser>
#include <QCoreApplication>

#include <objectregistry2.h>
#include <websocketserver.h>

class TestServerObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    int value() const { return _value; }
    void setValue(int value)
    {
        if (_value == value)
            return;

        _value = value;
        emit valueChanged();
    }

    Q_INVOKABLE int add(int amount) const { return _value + amount; }

signals:
    void valueChanged();

private:
    int _value = 0;
};

int main(int argc, char *argv[])
{
    QCoreApplication application{argc, argv};
    QCommandLineParser parser;
    QCommandLineOption portOption{{"p", "port"}, "WebSocket port.", "port", "21130"};
    parser.addOption(portOption);
    parser.process(application);

    bool isPortValid = false;
    const auto port = parser.value(portOption).toUShort(&isPortValid);
    if (!isPortValid || port == 0)
        return 1;

    ObjectRegistry2 registry;
    TestServerObject testObject;
    registry.registerValue("testServer", &testObject);
    WebSocketServer server{registry};
    server.setPort(port);
    if (!server.open())
        return 1;

    return application.exec();
}

#include "websocket-test-server.moc"