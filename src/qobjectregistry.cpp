#include "qobjectregistry.h"

#include <vector>

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(self, "registry", QtWarningMsg)
}

QObjectRegistry::QObjectRegistry(QObject *parent)
    : QObject{parent}
{
    // we need a the notifier slot meta method for our connect signatures
    _notifierSlotIdx = QObjectRegistry::metaObject()->indexOfMethod("onNotifySignal()");
    _notifierSlot = QObjectRegistry::metaObject()->method(_notifierSlotIdx);
}

void QObjectRegistry::registerObject(const QString &name, QObject *object)
{
    if (object == nullptr)
        return;

    auto metaObject = object->metaObject();
    qCInfo(self) << this << "register object" << metaObject->className() << name;

    // register properties

    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        auto property = metaObject->property(i);
        auto propertyName = QString{"%1.%2"}.arg(name, property.name());
        this->registerProperty(propertyName, object, property);
    }

    // register methods

    for (int i = 0; i < metaObject->methodCount(); ++i) {
        auto method = metaObject->method(i);
        auto methodName = QString{"%1.%2"}.arg(name, method.name());
        this->registerMethod(methodName, object, method);
    }

    connect(object, &QObject::destroyed, this, [this, object]() { this->deregisterObject(object); });
}

void QObjectRegistry::deregisterObject(const QString &name)
{
    _properties.removeIf([name](decltype(_properties)::iterator it) { return it.key().startsWith(name); });
    _methods.removeIf([name](decltype(_methods)::iterator it) { return it.key().startsWith(name); });
}

void QObjectRegistry::deregisterObject(QObject *object)
{
    qCInfo(self) << "deregister object:" << object;

    auto it0 = std::remove_if(_properties.begin(), _properties.end(), [object](const QPair<QObject *, QMetaProperty> &v) { return object == v.first; });
    _properties.erase(it0, _properties.end());

    auto it1 = std::remove_if(_methods.begin(), _methods.end(), [object](const QPair<QObject *, QMetaMethod> &v) { return object == v.first; });
    _methods.erase(it1, _methods.end());
}

QVariant QObjectRegistry::get(const QString &key)
{
    auto it = _properties.find(key);

    if (it == _properties.end()) {
        qCCritical(self) << "no getter for key:" << key;
        return QVariant{};
    }

    return it->second.read(it->first);
#if 0
    return (*it)();
#endif
}

void QObjectRegistry::set(const QString &key, const QVariant &value)
{
    auto it = _properties.find(key);

    if (it == _properties.end()) {
        qCCritical(self) << "no setter for key:" << key;
        return;
    }

    if (!value.canConvert(it->second.metaType())) {
        qCCritical(self) << "cannot convert" << value << "to" << it->second.typeName();
        return;
    }

    it->second.write(it->first, value);
}

QVariant QObjectRegistry::call(const QString &function, const QVariantList &arguments)
{
    return {};
}

void QObjectRegistry::onNotifySignal()
{
    auto notifyIt = _notify.find({sender(), senderSignalIndex()});

    if (notifyIt == _notify.end()) {
        qCCritical(self) << "failed to find notifies for" << sender() << senderSignalIndex();
        return;
    }

    (*notifyIt)();
}

const QMap<QString, QPair<QObject *, QMetaProperty> > &QObjectRegistry::properties() const
{
    return _properties;
}

const QMap<QString, QPair<QObject *, QMetaMethod> > &QObjectRegistry::methods() const
{
    return _methods;
}

void QObjectRegistry::registerProperty(const QString &propertyName, QObject *object, const QMetaProperty &property)
{
    qCInfo(self) << "register property:" << property.typeName() << propertyName;

    _properties[propertyName] = {object, property};

    const auto propertyValue = property.read(object);
    emit valueChanged(propertyName, propertyValue);

    if (!property.isReadable()) {
        qCCritical(self) << "cannot handle ungettable property";
        return;
    }

    if (property.hasNotifySignal()) {
        _notify[{object, property.notifySignalIndex()}] = [this, propertyName, property, object]() {
            const auto propertyValue = property.read(object);
            qCDebug(self) << "object property changed" << propertyName << propertyValue;
            emit valueChanged(propertyName, propertyValue);
        };

        connect(object, &QObject::destroyed, this, [this, object, index = property.notifySignalIndex()] { _notify.remove({object, index}); });
        connect(object, property.notifySignal(), this, _notifierSlot);
    }

    // handle special types

    auto propType = QMetaType{property.userType()};

    if (!propType.isValid()) {
        qCCritical(self) << "invalid property type:" << property.name();
        return;
    }

    // handle qobject*

    if (propType.flags().testFlag(QMetaType::PointerToQObject)) {
        auto propertyValue = property.read(object).value<QObject *>();
        qCDebug(self) << "recurse object:" << propertyName << propertyValue;

        this->registerObject(propertyName, propertyValue);

        if (property.hasNotifySignal()) {
            _notify[{object, property.notifySignalIndex()}] = [this, propertyName, property, object]() {
                const auto newValue = property.read(object);

                qCDebug(self) << "value changed" << propertyName << newValue;
                emit valueChanged(propertyName, newValue);

                // todo: this overwrites the read, write and call methods of the old object, but the old objects
                // destroy signal would still remove them from cb maps if it gets deleted

                this->deregisterObject(propertyName);
                this->registerObject(propertyName, newValue.value<QObject *>());
            };

            connect(object, &QObject::destroyed, this, [this, object, index = property.notifySignalIndex()] { _notify.remove({object, index}); });
            connect(object, property.notifySignal(), this, _notifierSlot);
        }
    }

    // handle simple arrays

    if (propertyValue.canConvert<QVariantList>() && propertyValue.typeId() != QMetaType::QString) {
        auto variantList = propertyValue.value<QVariantList>();
        qCDebug(self) << "recurse list like:" << variantList << propType.name();

        _notify[{object, property.notifySignalIndex()}] = [this, propertyName, property, object]() {
            const auto newValue = property.read(object).toList();

            qCDebug(self) << "list value changed:" << propertyName << newValue;
            emit valueChanged(propertyName, newValue);

            this->deregisterObject(propertyName + '.');

            for (int i = 0; i < newValue.size(); ++i) {
                auto variantType = QMetaType{newValue[i].userType()};

                if (variantType.flags().testFlag(QMetaType::PointerToQObject)) {
                    auto variant = newValue[i].value<QObject *>();
                    auto variantName = QString{"%1.%2"}.arg(propertyName, QString::number(i));

                    qCDebug(self) << "recurse:" << variantName << variant;
                    this->registerObject(variantName, variant);
                }
            }
        };

        for (int i = 0; i < variantList.size(); ++i) {
            auto variantType = QMetaType{variantList[i].userType()};

            if (variantType.flags().testFlag(QMetaType::PointerToQObject)) {
                auto variant = variantList[i].value<QObject *>();
                auto variantName = QString{"%1.%2"}.arg(propertyName, QString::number(i));

                qCDebug(self) << "recurse:" << variantName << variant;
                this->registerObject(variantName, variant);
            }
        }
    }
}

void QObjectRegistry::registerMethod(const QString &methodName, QObject *object, const QMetaMethod &method)
{
    if (method.access() != QMetaMethod::Public)
        return;

    if (method.methodType() == QMetaMethod::Signal)
        return;

    _call[methodName] = [method, object](const QVariantList &args) {
        if (method.parameterCount() != args.size()) {
            qCCritical(self) << "invalid arg size:" << method.parameterCount() << args.size();
            return QVariant();
        }

        QVariantList variants;

        for (int i = 0; i < args.size(); i++) {
            if (method.parameterType(i) == QMetaType::QVariant) {
                variants.append(args[i]);
                continue;
            }

            QVariant copy{args[i]};

            if (!copy.convert(QMetaType{method.parameterType(i)})) {
                qCCritical(self) << "cannot convert" << args[i] << "to" << method.parameterNames()[i];
                return QVariant();
            }

            variants.append(copy);
        }

        QList<QGenericArgument> gArgs;
        for (auto const &variant : variants) {
            QGenericArgument gArg(method.returnMetaType().name(), const_cast<void *>(variant.constData()));
            gArgs.append(gArg);
        }

#if QT_VERSION_MAJOR == 5
        QVariant returnValue(method.returnType(), static_cast<void *>(nullptr));
#else
        QVariant returnValue(method.returnMetaType(), static_cast<void *>(nullptr));
#endif
        QGenericReturnArgument gReturn(method.typeName(), const_cast<void *>(returnValue.constData()));

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
            return QVariant{};
        }

        return returnValue;
    };
}
