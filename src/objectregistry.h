#ifndef OBJECTREGISTRY2_H
#define OBJECTREGISTRY2_H

#include <QMetaProperty>
#include <QObject>
#include <QPointer>
#include <QVariant>

class RegisteredValue : public QObject
{
    Q_OBJECT
public:
    using ConnPtr = QSharedPointer<QMetaObject::Connection>;
    using Ptr = QSharedPointer<RegisteredValue>;
    const static QMetaMethod notifierSlot;

    explicit RegisteredValue();
    ~RegisteredValue();

    QString key;
    QVariant value;
    QStringList methodKeys;
    QMetaProperty property;
    int index = -1;
    QPointer<QObject> owner;
    QList<Ptr> children;

signals:
    void changed();
};

class ObjectRegistry2 : public QObject
{
    Q_OBJECT
public:
    explicit ObjectRegistry2(QObject *parent = nullptr);

public slots:

    RegisteredValue::Ptr registerValue(const QString &key, QObject *object);

    RegisteredValue::Ptr registerValue(
        const QString &key, const QVariant &value, QObject *owner = nullptr, const QMetaProperty &property = QMetaProperty(), int index = -1
    );
    void deregisterValue(const QString &key);

    QVariant call(const QString &key, const QVariantList &arguments);
    bool set(const QString &key, const QVariant &value);
    QVariant get(const QString &key);

signals:
    void signalEmitted(const QString &key, const QVariantList &args); // todo
    void valueChanged(const QString &key, const QVariant &value);

private:
    static QVariant callMethod(const QMetaMethod &method, QObject *object, const QVariantList &arguments);
    QMap<QString, RegisteredValue::Ptr> _registeredValues;
    QMap<QString, std::function<QVariant(const QVariantList &)>> _registeredMethods;
};

#endif // OBJECTREGISTRY2_H
