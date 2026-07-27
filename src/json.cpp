#include "json.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QSequentialIterable>

#include "listmodel.h"

namespace {
Q_LOGGING_CATEGORY(self, "json", QtWarningMsg)
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
    qCDebug(self) << "stringifying variant of type" << variant.typeName();
    auto value = serialize(variant);

    switch (value.type()) {
    case QJsonValue::Object:
        qCDebug(self) << "serialized to QJsonObject";
        return QJsonDocument{value.toObject()}.toJson();
    case QJsonValue::Array:
        qCDebug(self) << "serialized to QJsonArray";
        return QJsonDocument{value.toArray()}.toJson();
    default:
        qCWarning(self) << "serialized to unexpected type:" << value.type() << value;
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

QJsonValue JSON::serialize(const QVariant &variant, const VisitedObjects &visitedObjects)
{
#if QT_VERSION_MAJOR == 5
    auto metaType = QMetaType{variant.userType()};
    auto typeId = variant.userType();
#else
    auto metaType = variant.metaType();
    auto typeId = variant.typeId();
#endif

    qCDebug(self) << "serializing variant of type" << metaType.name() << "(" << typeId << ")";

    if (variant.isNull()) {
        qCDebug(self) << "variant is null";
        return QJsonValue::Null;
    }

    if (!variant.isValid()) {
        qCWarning(self) << "variant is invalid";
        return QJsonValue::Undefined;
    }

    auto serializer = _serializers.find(typeId);
    if (serializer != _serializers.end()) {
        qCDebug(self) << "using custom serializer for type" << metaType.name();
        return serializer->serialize(variant);
    }

    if (metaType.flags().testFlag(QMetaType::PointerToQObject)) {
        auto metaObject = metaType.metaObject();
        qCDebug(self) << "serializing QObject:" << metaObject->className();

        if (visitedObjects->contains(variant.value<QObject *>())) {
            qCCritical(self) << "circular serialization detected:" << variant;
            return QJsonValue::Null;
        }

        visitedObjects->insert(variant.value<QObject *>());

        QJsonObject object{{"__typeId", variant.typeId()}, {"__typeName", metaType.name()}};
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            qCDebug(self) << "serializing QObject property:" << property.name() << property.metaType().name();
            object[property.name()] = serialize(property.read(variant.value<QObject *>()), visitedObjects);
        }
        return object;
    }

    if (metaType.flags().testFlag(QMetaType::PointerToGadget)) {
        auto metaObject = metaType.metaObject();
        qCDebug(self) << "serializing Gadget:" << metaObject->className();

        QJsonObject object{{"__typeId", variant.typeId()}, {"__typeName", metaType.name()}};
        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            qCDebug(self) << "serializing Gadget property:" << property.name() << property.metaType().name();
            object[property.name()] = serialize(property.readOnGadget(variant.constData()), visitedObjects);
        }
        return object;
    }

    else if (variant.canConvert<QVariantList>() && typeId != QMetaType::QString) {
        QJsonArray array;
        qCDebug(self) << "serializing QVariantList with" << variant.value<QVariantList>().size() << "elements";
        const auto list = variant.value<QVariantList>();
        for (const auto &x : std::as_const(list))
            array.append(serialize(x, visitedObjects));
        return array;
    }
    qCDebug(self) << "using default toJsonValue conversion for type" << metaType.name();

    return variant.toJsonValue();
}

QVariant JSON::deserialize(const QJsonValue &value, const QMetaType &type)
{
    QMetaType targetType = type;

    if (!targetType.isValid() && value.isObject()) {
        qCDebug(self) << "no valid target type given, trying to determine with properties";

        auto typeName = value["__typeName"].toString().toUtf8();
        targetType = QMetaType::fromName(typeName);
        qCDebug(self) << "loaded from __typeName:" << targetType;
    }

    if (!targetType.isValid()) {
        qCDebug(self) << "no target type found, using default conversion for:" << value;

        auto variant = value.toVariant();

        if (!variant.isValid())
            qCWarning(self) << "default conversion failed:" << value;

        return variant;
    }

    qCDebug(self) << "trying to deserialize" << value << "into" << type;

    auto serializer = _serializers.find(targetType.id());
    if (serializer != _serializers.end())
        return serializer->deserialize(value);

    if (targetType.flags().testFlag(QMetaType::PointerToQObject)) {
        if (!value.isObject()) {
            qCWarning(self) << "value should deserialize into a pointer to object but is not an object" << targetType << targetType.flags();
            return QVariant(targetType);
        }

        auto metaObject = targetType.metaObject();
        QObject *object = metaObject->newInstance();

        //  if (metaObject->inherits(QMetaType::fromType<ListModelBase>().metaObject())) {
        //  };

        if (object == nullptr) {
            qCCritical(self) << "failed to create object:" << metaObject->className();
            return QVariant(targetType);
        }

        qCDebug(self) << "created" << targetType << object;

        for (auto i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);

            if (!property.isWritable()) {
                qCDebug(self) << "cannot write to property:" << property.name();
                continue;
            }

            if (value[property.name()].isUndefined()) {
                qCWarning(self) << "expected" << property.name() << "for" << targetType.name() << "in" << value;
                continue;
            }

            auto propertyValue = deserialize(value[property.name()], property.metaType());

            if (!propertyValue.isValid()) {
                qCWarning(self) << "failed to deserialize property!";
                continue;
            }

            qCDebug(self)                                         //@
                << "setting property" << QString{property.name()} //@
                << QString{property.metaType().name()}            //@
                << QString{propertyValue.metaType().name()}       //@
                << propertyValue;

            if (!property.write(object, propertyValue)) {
                qCWarning(self) << "failed to write" << QString{property.name()} << "on" << object;
            }
        }

        return QVariant::fromValue(object);
    }

    else if (value.isArray()) {
        const auto array = value.toArray();

        QVariantList list;
        list.reserve(array.size());

        for (auto const &element : array) {
            auto deserializedElement = deserialize(element);
            list.append(deserializedElement);
        }

        return QVariant::fromValue(list);
    }

    auto variant = value.toVariant();
    if (!variant.isValid())
        qCWarning(self) << "default conversion failed:" << value;

    return variant;
}
