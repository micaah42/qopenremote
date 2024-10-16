#include "json.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaProperty>

namespace {
Q_LOGGING_CATEGORY(self, "JSON")
}

QByteArray JSON::stringify(const QVariant &variant)
{
    auto value = serialize(variant);
    auto doc = value.isArray() ? QJsonDocument{value.toArray()} : QJsonDocument{value.toObject()};
    return doc.toJson();
}

QVariant JSON::parse(const QByteArray &json)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError) {
        qCWarning(self) << "failed to parse json:" << error.errorString();
        return QVariant();
    }

    auto value = doc.isObject() ? QJsonValue{doc.object()} : QJsonValue{doc.array()};
    return deserialize(value);
}

QJsonValue JSON::serialize(const QVariant &variant)
{
    auto metaType = variant.metaType();

    if (metaType.flags().testFlag(QMetaType::PointerToQObject)) {
        auto metaObject = metaType.metaObject();

        QJsonObject object{{"__typeId", variant.typeId()}};
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            object[property.name()] = serialize(property.read(variant.value<QObject *>()));
        }
        return object;
    }

    else if (metaType.flags().testFlag(QMetaType::PointerToGadget)) {
        auto metaObject = metaType.metaObject();

        QJsonObject object{{"__typeId", variant.typeId()}};
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            object[property.name()] = serialize(property.readOnGadget(variant.constData()));
        }
        return object;
    }

    else if (variant.canView<QVariantList>() && !variant.canConvert<QString>()) {
        QJsonArray array;
        const auto list = variant.value<QVariantList>();
        for (const auto &x : qAsConst(list))
            array.append(serialize(x));
        return array;
    }

    return variant.toJsonValue();
}

QVariant JSON::deserialize(const QJsonValue &value)
{
    if (value.isObject()) {
        if (!value["__typeId"].isDouble()) {
            qCWarning(self) << "expected '__typeId' string in object" << value;
            return QJsonValue::Undefined;
        }

        QMetaType metaType{value["__typeId"].toInt()};

        if (!metaType.isDefaultConstructible()) {
            qCWarning(self) << "no default ctor. cannot deserialize into" << metaType.name();
            return QJsonValue::Undefined;
        }

        if (metaType.flags() | QMetaType::PointerToQObject) {
            auto metaObject = metaType.metaObject();
            QObject *object = metaObject->newInstance();

            for (auto i = 0; i < metaObject->propertyCount(); ++i) {
                auto property = metaObject->property(i);

                if (value[property.name()].isUndefined()) {
                    qCWarning(self) << "expected" << property.name() << "for" << metaType.name() << "in" << value;
                    return QJsonValue::Undefined;
                }

                auto propertyValue = deserialize(value[property.name()]);
                if (propertyValue == QJsonValue::Undefined) {
                    qCWarning(self) << "failed to serialize property!";
                    return QJsonValue::Undefined;
                }

                qCInfo(self) << "set property" << property.name() << propertyValue << value[property.name()];
                property.write(object, propertyValue);
            }

            return QVariant::fromValue(object);
        }
    }

    return value.toVariant();
}
