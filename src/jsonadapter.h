#ifndef JSONADAPTER_H
#define JSONADAPTER_H

#include <QObject>

#include "qobjectregistry.h"

class JSONAdapter : public QObject
{
    Q_OBJECT
public:
    explicit JSONAdapter(QObjectRegistry &registry, QObject *parent = nullptr);
    //static QJsonValue serialize(const QVariant &variant);

public slots:
    void handleMessage(const QByteArray &message);

signals:
    void sendMessage(const QByteArray &message);

private slots:
    void onValueChanged(const QString &key, const QVariant &value);

    void handleSubscribe(const QString &key);
    void handleCall(const QString &key, const QJsonArray &array);
    void handleSet(const QString &key, const QJsonValue &array);
    void handleGet(const QString &key);

private:
    QMap<QString, int> _subscribed;
    QObjectRegistry &_registry;
};

#endif // JSONADAPTER_H
