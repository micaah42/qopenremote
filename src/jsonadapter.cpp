#include "jsonadapter.h"

#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include "json.h"

namespace {
Q_LOGGING_CATEGORY(self, "adapter.json", QtWarningMsg)
}

JSONAdapter::JSONAdapter(ObjectRegistry2 &registry, QObject *parent)
    : QObject{parent}
    , _registry{registry}
{
    connect(&registry, &ObjectRegistry2::valueChanged, this, &JSONAdapter::onValueChanged);
}

void JSONAdapter::handleMessage(const QByteArray &message)
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

void JSONAdapter::onValueChanged(const QString &key, const QVariant &value)
{
    if (_subscribed[key] == 0)
        return;

    QJsonObject object{
        {"type", "notify"},
        {"key", key},
        {"value", JSON::serialize(value)},
    };

    qCDebug(self) << "send notify" << object;
    emit sendMessage(QJsonDocument{object}.toJson());
}

void JSONAdapter::handleSubscribe(const QString &key)
{
    qCInfo(self) << "subscribed to key:" << key;
    _subscribed[key] += 1;

    auto value = _registry.get(key);

    QJsonObject object{
        {"type", "notify"},
        {"value", JSON::serialize(value)},
        {"key", key},
    };

    emit sendMessage(QJsonDocument{object}.toJson());
}

// try {
//     if (watcher->future().isCanceled()) {
//         QJsonObject object{{"type", "error"}, {"key", key}, {"error", "cancelled"}};
//         emit sendMessage(QJsonDocument{object}.toJson());
//         return;
//     }
//     if (watcher->future().resultCount()) {
//         QJsonObject object{{"type", "return"}, {"value", JSON::serialize(watcher->result())}, {"key", key}};
//         emit sendMessage(QJsonDocument{object}.toJson());
//     } else {
//         QJsonObject object{{"type", "return"}, {"value", QJsonValue()}, {"key", key}};
//         emit sendMessage(QJsonDocument{object}.toJson());
//     }
// } catch (const std::exception &error) {
//     QJsonObject object{{"type", "error"}, {"key", key}, {"error", QString::fromUtf8(error.what())}};
//     emit sendMessage(QJsonDocument{object}.toJson());
// } catch (...) {
//     QJsonObject object{{"type", "error"}, {"key", key}, {"error", "unknown error"}};
//     emit sendMessage(QJsonDocument{object}.toJson());
// }

void JSONAdapter::handleCall(const QString &key, const QJsonArray &array)
{
    qCInfo(self) << "calling" << key << array;
    auto returnValue = _registry.call(key, array.toVariantList());

    if (returnValue.metaType() == QMetaType::fromType<QFuture<QVariant>>()) {
        auto *watcher = new QFutureWatcher<QVariant>{this};
        connect(watcher, &QFutureWatcher<QVariant>::finished, this, [this, key, watcher]() {
            watcher->deleteLater();

            QJsonValue result;

            try {
                const auto results = watcher->future().results();
                if (!results.empty())
                    result = JSON::serialize(results.first());
            } catch (const QException &e) {
                result = QString::fromUtf8(e.what());
            } catch (const std::exception &error) {
                result = QString::fromUtf8(error.what());
            }

            QString resultKey = watcher->future().isCanceled() ? "error" : "value";
            QString typeValue = watcher->future().isCanceled() ? "error" : "return";

            QJsonObject object{{"type", typeValue}, {"key", key}, {resultKey, result}};
            emit sendMessage(QJsonDocument{object}.toJson());
        });
        watcher->setFuture(returnValue.value<QFuture<QVariant>>());
        return;
    }

    QJsonObject object{
        {"type", "return"},
        {"value", JSON::serialize(returnValue)},
        {"key", key},
    };

    emit sendMessage(QJsonDocument{object}.toJson());
}

void JSONAdapter::handleSet(const QString &key, const QJsonValue &value)
{
    qCDebug(self) << "handle set" << key << value;
    _registry.set(key, value);
}

void JSONAdapter::handleGet(const QString &key)
{
    auto value = _registry.get(key);

    QJsonObject object{
        {"type", "return"},
        {"value", JSON::serialize(value)},
        {"key", key},
    };

    qCDebug(self) << "handle get" << object;
    emit sendMessage(QJsonDocument{object}.toJson());
}
