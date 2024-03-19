#include "remote.h"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(self, "registry")
}

QObjectRegistry::QObjectRegistry(QObject *parent)
    : QObject{parent}
{
    // we need a the notifier slot meta method for our connect signatures
    _notifierSlotIdx = this->metaObject()->indexOfMethod("onNotifySignal()");
    _notifierSlot = this->metaObject()->method(_notifierSlotIdx);
}

void QObjectRegistry::registerObject(const QString &name, QObject *object)
{
    auto metaObject = object->metaObject();
    qCInfo(self) << this << "register" << metaObject->className() << name;

    _objects.insert(name, object);

    // register properties

    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        auto prop = metaObject->property(i);
        auto propertyName = QString{"%1.%2"}.arg(name, prop.name());
        qCDebug(self) << "register property:" << prop.typeName() << prop.name();

        if (!prop.isReadable()) {
            qCCritical(self) << "cannot handle ungettable property";
            continue;
        }

        if (prop.hasNotifySignal()) {
            _notify[{object, prop.notifySignalIndex()}] = [this, propertyName, prop, object]() {
                const auto propertyValue = prop.read(object);
                qCDebug(self) << "value changed" << propertyName << propertyValue;

                emit valueChanged(propertyName, propertyValue);
            };

            connect(object, prop.notifySignal(), this, _notifierSlot);
        }

        if (prop.isWritable()) {
            _write[propertyName] = [prop, object](const QVariant &value) { prop.write(object, value); };
        }

        // handle special types

        auto propType = QMetaType{prop.userType()};

        if (!propType.isValid()) {
            qCCritical(self) << "invalid property type:" << prop.name();
            continue;
        }

        if (propType.flags().testFlag(QMetaType::PointerToQObject)) {
            auto propertyValue = prop.read(object);
            qCDebug(self) << "recurse:" << propertyName << propertyValue;

            this->registerObject(propertyName, propertyValue.value<QObject *>());
        }

        // handle list like properties

        auto propertyValue = prop.read(object);

        if (propertyValue.canConvert<QVariantList>()) {
            auto variantList = propertyValue.value<QVariantList>();
            qCDebug(self) << "found qlist:" << variantList;

            for (int i = 0; i < variantList.size(); ++i) {
                auto variantType = QMetaType{variantList[i].userType()};

                if (variantType.flags().testFlag(QMetaType::PointerToQObject)) {
                    auto variant = variantList[i];
                    auto variantName = QString{"%1.%2"}.arg(propertyName, QString::number(i));

                    qCDebug(self) << "recurse:" << variantName << variant;
                    this->registerObject(variantName, variant.value<QObject *>());
                }
            }

            if (!variantList.isEmpty()) {
                qCInfo(self) << variantList.first().typeName();
            }
        }

        _read[propertyName] = [object, prop]() { return prop.read(object); };
    }

    // register methods
}

QVariant QObjectRegistry::value(const QString &key)
{
    return {};
}

void QObjectRegistry::setValue(const QString &key, const QVariant &value) {}

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
