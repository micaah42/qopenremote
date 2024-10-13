#ifndef JSON_H
#define JSON_H

#include <QJsonValue>
#include <QVariant>

class JSON
{
public:
    template<class T>
    static QByteArray stringify(const T &t);
    static QByteArray stringify(const QVariant &variant);

    template<class T>
    static T parse(const QByteArray &json);
    static QVariant parse(const QByteArray &json);

    template<class T>
    static QJsonValue serialize(const T &t);
    static QJsonValue serialize(const QVariant &variant);

    template<class T>
    static T deserialize(const QJsonValue &value);
    static QVariant deserialize(const QJsonValue &value);

private:
    // QHash<int, std::function<QJsonValue(const QVariant &)>> _serialize;
};

template<class T>
T JSON::parse(const QByteArray &json)
{
    return parse(json).value<T>();
}

template<class T>
QByteArray JSON::stringify(const T &t)
{
    return stringify(QVariant::fromValue(t));
}

template<class T>
T JSON::deserialize(const QJsonValue &value)
{
    return deserialize(value).value<T>();
}

template<class T>
QJsonValue JSON::serialize(const T &t)
{
    return serialize(QVariant::fromValue(t));
}
#endif // JSON_H