#include "objectregistry2.h"

#include <QLoggingCategory>
#include <QMetaProperty>

namespace {
Q_LOGGING_CATEGORY(self, "registry")
}

const QMetaMethod RegisteredValue::notifierSlot = QMetaMethod::fromSignal(&RegisteredValue::changed);

ObjectRegistry2::ObjectRegistry2(QObject *parent)
    : QObject{parent}
{
    qCInfo(self) << "create object registry" << this;
}

RegisteredValue::Ptr ObjectRegistry2::registerValue(const QString &key, QObject *object)
{
    return this->registerValue(key, QVariant::fromValue(object));
}

RegisteredValue::Ptr ObjectRegistry2::registerValue(
    const QString &key, const QVariant &value, QObject *owner, const QMetaProperty &property, int index
)
{
    qCInfo(self) << "register value:" << key << value;

    auto it = _registeredValues.find(key);

    if (it != _registeredValues.end()) {
        qCWarning(self) << "key already used (key, old, new):" << key << (*it)->value << value;
        return {};
    }

    auto registeredValue = RegisteredValue::Ptr::create();
    registeredValue->property = property;
    registeredValue->owner = owner;
    registeredValue->key = key;
    registeredValue->value = value;

    if (value.canConvert<QObject *>() && !value.isNull()) {
        auto object = value.value<QObject *>();
        auto metaObject = object->metaObject();

        qCDebug(self) << "recurse object:" << key << metaObject->className();

        for (int i = 0; i < metaObject->propertyCount(); ++i) {
            auto property = metaObject->property(i);
            auto propertyName = QString{"%1.%2"}.arg(key, property.name());
            auto propertyValue = property.read(object);
            auto childRegisteredValue = this->registerValue(propertyName, propertyValue, object, property);
            registeredValue->children.append(childRegisteredValue);

            if (property.hasNotifySignal()) {
                connect(object, property.notifySignal(), childRegisteredValue.get(), RegisteredValue::notifierSlot);
                connect(childRegisteredValue.get(), &RegisteredValue::changed, this, [this, registeredValue = childRegisteredValue.get()]() {
                    auto newValue = registeredValue->property.read(registeredValue->owner);
                    emit valueChanged(registeredValue->key, newValue);
                    qCDebug(self) << "registered value changed:" << registeredValue->key << "=>" << newValue;

                    if (newValue.canConvert<QObject *>() || newValue.canConvert<QVariantList>()) {
                        this->deregisterValue(registeredValue->key);
                        this->registerValue(registeredValue->key, newValue, registeredValue->owner, registeredValue->property);
                    }
                });
            }
        }

        for (int i = 0; i < metaObject->methodCount(); i++) {
            auto method = metaObject->method(i);
            auto methodName = QString{"%1.%2"}.arg(key, method.name());

            qCDebug(self) << "register method:" << methodName << method.methodSignature();

            _registeredMethods[methodName] = [method, object](const QVariantList &arguments) {
                return callMethod(method, object, arguments);
            };

            registeredValue->methodKeys.append(methodName);
        }
    }

    else if (value.canConvert<QVariantList>() && value.typeId() != QMetaType::QString) {
        auto list = value.toList();
        qCDebug(self) << "recurse list:" << key << list;
        for (int i = 0; i < list.size(); i++) {
            auto item = list[i];
            auto itemName = QString{"%1.%2"}.arg(key, QString::number(i));
            auto childRegisteredValue = this->registerValue(itemName, item, owner, property, i);
            registeredValue->children.append(childRegisteredValue);
        }
    }

    _registeredValues.insert(key, registeredValue);
    qCDebug(self) << "registered value:" << key;
    return registeredValue;
}

void ObjectRegistry2::deregisterValue(const QString &key)
{
    qCInfo(self) << "deregister value:" << key;
    const auto value = _registeredValues.take(key);

    for (auto const &key : std::as_const(value->methodKeys))
        _registeredMethods.remove(key);

    for (auto const &child : std::as_const(value->children))
        this->deregisterValue(child->key);
}

bool ObjectRegistry2::set(const QString &key, const QVariant &value)
{
    qCInfo(self) << "set value:" << key << value;
    auto it = _registeredValues.find(key);

    if (it == _registeredValues.end()) {
        qCWarning(self) << "failed to set: no value registered under key" << key;
        return false;
    }

    const auto &registeredValue = **it;
    if (!registeredValue.owner) {
        qCWarning(self) << "cannot set: no owner for the registered value (key):" << key;
        return false;
    }

    if (!registeredValue.property.isValid()) {
        qCWarning(self) << "cannot set: no property for the registered value (key):" << key;
        return false;
    }

    if (!registeredValue.property.write(registeredValue.owner, value)) {
        qCWarning(self) << "cannot set: writing failed (key, typeName, value):" << key << registeredValue.property.typeName() << value;
        return false;
    }

    qCDebug(self) << "set value succeeded:" << key;
    return true;
}

QVariant ObjectRegistry2::get(const QString &key)
{
    qCInfo(self) << "get value:" << key;
    auto it = _registeredValues.find(key);

    if (it == _registeredValues.end()) {
        qCWarning(self) << "failed to get: no value registered under key" << key;
        return {};
    }

    const auto &registeredValue = **it;
    if (registeredValue.owner && registeredValue.property.isValid()) {
        auto value = registeredValue.property.read(registeredValue.owner);
        qCDebug(self) << "get value result:" << key << value;

        if (registeredValue.index == -1)
            return value;

        auto list = value.toList();
        auto index = registeredValue.index;

        if (index < 0 || index > list.size()) {
            qCWarning(self) << "get value index out of range!";
            return {};
        }

        return list[index];
    }

    qCDebug(self) << "get value result:" << key << registeredValue.value;
    return registeredValue.value;
}

QVariant ObjectRegistry2::callMethod(const QMetaMethod &method, QObject *object, const QVariantList &args)
{
    qCInfo(self) << "call method:" << method.methodSignature() << args;
    if (method.parameterCount() != args.size()) {
        qCCritical(self) << "invalid arg size:" << method.parameterCount() << args.size();
        return QVariant();
    }

    QVariantList variants;
    auto parameterNames = method.parameterNames();

    for (int i = 0; i < args.size(); i++) {
        if (method.parameterType(i) == QMetaType::QVariant) {
            variants.append(args[i]);
            continue;
        }

        QVariant copy{args[i]};
        auto convertTarget = QMetaType{method.parameterType(i)};
        if (!copy.convert(convertTarget)) {
            qCCritical(self) << "cannot convert" << args[i] << "to" << parameterNames[i];
            return QVariant();
        }

        qCDebug(self) << "converted method argument:" << i << args[i] << "=>" << copy;
        variants.append(copy);
    }

    QList<QGenericArgument> gArgs;

    for (auto const &variant : variants) {
        QGenericArgument gArg(variant.typeName(), const_cast<void *>(variant.constData()));
        gArgs.append(gArg);
    }

    QVariant returnValue(method.returnMetaType());
    QGenericReturnArgument gReturn(method.typeName(), returnValue.data());

    try {
        bool ok = method.invoke( //
                object,
                Qt::DirectConnection,
                gReturn,
                gArgs.value(0),
                gArgs.value(1),
                gArgs.value(2),
                gArgs.value(3),
                gArgs.value(4),
                gArgs.value(5),
                gArgs.value(6),
                gArgs.value(7),
                gArgs.value(8),
                gArgs.value(9));

        if (!ok) {
            qCWarning(self) << "calling" << method.methodSignature() << "failed.";
            return QVariant{};
        }
    }

    catch (const std::exception &error) {
        qCCritical(self) << "invoking method threw error:" << QString(error.what());
        return QVariant{};
    }

    if (method.returnType() == QMetaType::Void) {
        qCDebug(self) << "method returned void:" << method.methodSignature();
        return QVariant{};
    }

    qCDebug(self) << "method result:" << method.methodSignature() << returnValue;
    return QVariant{method.returnMetaType(), returnValue.constData()};
}

QVariant ObjectRegistry2::call(const QString &key, const QVariantList &arguments)
{
    qCInfo(self) << "call value:" << key << arguments;
    const auto method = _registeredMethods.constFind(key);
    if (method == _registeredMethods.cend()) {
        qCWarning(self) << "failed to call: no method registered under key" << key;
        return {};
    }

    return (*method)(arguments);
}

RegisteredValue::RegisteredValue() {}

RegisteredValue::~RegisteredValue() {}
