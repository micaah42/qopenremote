#include "qopenremoteadapter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(self, "adapter.json")
}

QOpenRemoteAdapter::QOpenRemoteAdapter(QObjectRegistry &registry, QObject *parent)
    : QObject{parent}
    , _registry{registry}
{
    connect(&registry, &QObjectRegistry::valueChanged, this, &QOpenRemoteAdapter::onValueChanged);
}

void QOpenRemoteAdapter::handleMessage(const QByteArray &message)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(message, &error);

    if (error.error != QJsonParseError::NoError) {
        qCWarning(self) << "parse error" << error.errorString() << "in" << message;
        return;
    }

    if (!doc.isObject()) {
        qCWarning(self) << "json doc is not an object:" << doc;
        return;
    }

    auto object = doc.object();

    if (!object["type"].isString()) {
        qCWarning(self) << "no type attribute in object!";
        return;
    }

    auto type = object["type"].toString();

    if (!object["key"].isString()) {
        qCWarning(self) << "no key attribute in object!";
        return;
    }

    auto key = object["key"].toString();

    if (type == "call")
        handleCall(key, object["args"].toArray());
    else if (type == "get")
        handleGet(key);
    else if (type == "set")
        handleSet(key, object["value"]);
    else if (type == "subscribe")
        handleSubscribe(key);
    else
        qCCritical(self) << "invalid type:" << type << key;
}

void QOpenRemoteAdapter::onValueChanged(const QString &key, const QVariant &value)
{
    if (_subscribed[key] == 0)
        return;

    QJsonObject object{
        {"type", "notify"},
        {"key", key},
        {"value", value.toJsonValue()},
    };

    emit sendMessage(QJsonDocument{object}.toJson());
}

void QOpenRemoteAdapter::handleSubscribe(const QString &key)
{
    _subscribed[key] += 1;

    auto value = _registry.get(key);

    QJsonObject object{
        {"type", "notify"},
        {"value", value.toJsonValue()},
        {"key", key},
    };

    emit sendMessage(QJsonDocument{object}.toJson());
}

void QOpenRemoteAdapter::handleCall(const QString &key, const QJsonArray &array)
{
    auto returnValue = _registry.call(key, array.toVariantList());

    QJsonObject object{
        {"type", "return"},
        {"value", returnValue.toJsonValue()},
        {"key", key},
    };

    emit sendMessage(QJsonDocument{object}.toJson());
}

void QOpenRemoteAdapter::handleSet(const QString &key, const QJsonValue &value)
{
    _registry.set(key, value);
}

void QOpenRemoteAdapter::handleGet(const QString &key)
{
    auto value = _registry.get(key);

    QJsonObject object{
        {"type", "return"},
        {"value", value.toJsonValue()},
        {"key", key},
    };

    emit sendMessage(QJsonDocument{object}.toJson());
}
