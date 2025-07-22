#include "json.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaProperty>

namespace {
Q_LOGGING_CATEGORY(self, "JSON", QtWarningMsg)
}

QHash<int, JSON::Serializer> JSON::_serializers = {
    {
        static_cast<int>(QMetaType::QDateTime),
        {
            [](const QJsonValue &v) -> QVariant { return v.isNull() ? QDateTime() : QDateTime::fromString(v.toString(), Qt::ISODateWithMs); },
            [](const QVariant &v) -> QJsonValue {
                auto dateTime = v.value<QDateTime>();
                return dateTime.isValid() ? QJsonValue(dateTime.toString(Qt::ISODateWithMs)) : QJsonValue::Null;
            },
        },
    },
    {
        static_cast<int>(QMetaType::QTime),
        {
            [](const QJsonValue &v) -> QVariant { return v.isNull() ? QTime() : QTime::fromString(v.toString(), "HH:mm:ss.zzz"); },
            [](const QVariant &v) -> QJsonValue {
                auto time = v.value<QTime>();
                return time.isValid() ? QJsonValue(time.toString("HH:mm:ss.zzz")) : QJsonValue::Null;
            },
        },
    },
    {
        static_cast<int>(QMetaType::QDate),
        {
            [](const QJsonValue &v) -> QVariant { return v.isNull() ? QDate() : QDate::fromString(v.toString(), "yyyy-MM-dd"); },
            [](const QVariant &v) -> QJsonValue {
                auto date = v.value<QDate>();
                return date.isValid() ? QJsonValue(date.toString("yyyy-MM-dd")) : QJsonValue::Null;
            },
        },
    },
    {
        static_cast<int>(QMetaType::QColor),
        {
            [](const QJsonValue &v) -> QVariant { return v.isNull() ? QColor() : QColor{v.toString()}; },
            [](const QVariant &v) -> QJsonValue {
                auto color = v.value<QColor>();
                return color.isValid() ? QJsonValue(color.name()) : QJsonValue::Null;
            },
        },
    },
};

QByteArray JSON::stringify(const QVariant &variant)
{
    auto value = serialize(variant);

    switch (value.type()) {
    case QJsonValue::Object:
        return QJsonDocument{value.toObject()}.toJson();
    case QJsonValue::Array:
        return QJsonDocument{value.toArray()}.toJson();
    default:
        return "";
    }
}

QVariant JSON::parse(const QByteArray &json, const QMetaType &type)
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
#if QT_VERSION_MAJOR == 5
    auto metaType = QMetaType{variant.userType()};
    auto typeId = variant.userType();
#else
    auto metaType = variant.metaType();
    auto typeId = variant.typeId();
#endif

    if (variant.isNull())
        return QJsonValue::Null;

    if (!variant.isValid())
        return QJsonValue::Undefined;

    auto serializer = _serializers.find(typeId);

    if (serializer != _serializers.end())
        return serializer->serialize(variant);

    if (metaType.flags().testFlag(QMetaType::PointerToQObject)) {
        auto metaObject = metaType.metaObject();

        QJsonObject object{{"__typeId", variant.typeId()}, {"__typeName", metaType.name()}};
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            object[property.name()] = serialize(property.read(variant.value<QObject *>()));
        }
        return object;
    }

    if (metaType.flags().testFlag(QMetaType::PointerToGadget)) {
        auto metaObject = metaType.metaObject();

        QJsonObject object{{"__typeId", variant.typeId()}, {"__typeName", metaType.name()}};
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            object[property.name()] = serialize(property.readOnGadget(variant.constData()));
        }
        return object;
    }

    else if (variant.canConvert<QVariantList>() && typeId != QMetaType::QString) {
        QJsonArray array;
        const auto list = variant.value<QVariantList>();
        for (const auto &x : std::as_const(list))
            array.append(serialize(x));
        return array;
    }

    return variant.toJsonValue();
}

QVariant JSON::deserialize(const QJsonValue &value, const QMetaType &type)
{
    if (type.isValid()) {
        auto serializer = _serializers.find(type.id());

        if (serializer != _serializers.end())
            return serializer->deserialize(value);
    }

    if (value.isObject()) {
        if (!value["__typeId"].isDouble()) {
            qCWarning(self) << "expected '__typeId' string in object" << value;
            return QJsonValue::Undefined;
        }

        QMetaType metaType = type;

        if (!metaType.isValid()) {
            auto id = value["__typeId"].toInt();
            metaType = QMetaType{id};
            qCDebug(self) << "trying __typeId" << id;
        }

        if (!metaType.isValid()) {
            auto typeName = value["__typeName"].toString().toUtf8();
            metaType = QMetaType::fromName(typeName);
            qCDebug(self) << "trying __typeName" << typeName;

            if (!metaType.isValid()) {
                qCCritical(self) << "failed to deserialize" << value;
                return QJsonValue::Undefined;
            }
        }

        if (type.isValid() && metaType != type) {
            qCCritical(self) << "expected:" << type << "but got" << metaType << value;
            return QJsonValue::Undefined;
        }

#if QT_VERSION_MAJOR == 6
        if (!metaType.isDefaultConstructible()) {
            qCWarning(self) << "no default ctor. cannot deserialize into" << metaType.name();
            return QJsonValue::Undefined;
        }
#endif

        if (metaType.flags() | QMetaType::PointerToQObject) {
            auto metaObject = metaType.metaObject();
            QObject *object = metaObject->newInstance();

            for (auto i = 0; i < metaObject->propertyCount(); ++i) {
                auto property = metaObject->property(i);

                if (value[property.name()].isUndefined()) {
                    qCWarning(self) << "expected" << property.name() << "for" << metaType.name() << "in" << value;
                    return QJsonValue::Undefined;
                }

                auto propertyValue = deserialize(value[property.name()], property.metaType());

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

    else if (value.isArray()) {
        QVariantList list;
        const auto array = value.toArray();

        for (auto const &value : array)
            list.append(deserialize(value));

        return list;
    }

    return value.toVariant();
}
