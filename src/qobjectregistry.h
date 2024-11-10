#ifndef QOBJECTREGISTRY_H
#define QOBJECTREGISTRY_H

#include <QMap>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QObject>

class QObjectRegistry : public QObject
{
    Q_OBJECT
public:
    explicit QObjectRegistry(QObject *parent = nullptr);

    void registerObject(const QString &name, QObject *object);
    void deregisterObject(const QString &name);
    void deregisterObject(QObject *object);

    const QMap<QString, QPair<QObject *, QMetaProperty>> &properties() const;
    const QMap<QString, QPair<QObject *, QMetaMethod>> &methods() const;

public slots:
    QVariant call(const QString &function, const QVariantList &arguments);

    void set(const QString &key, const QVariant &value);
    QVariant get(const QString &key);

signals:
    void signalEmitted(const QString &key, const QVariantList &args); // todo
    void valueChanged(const QString &key, const QVariant &value);

private slots:
    void registerProperty(const QString &propertyName, QObject *object, const QMetaProperty &property);
    void registerMethod(const QString &methodName, QObject *object, const QMetaMethod &method);
    void onNotifySignal();

private:
    QMap<QString, QPair<QObject *, QMetaMethod>> _methods;
    //QMap<QString, QPair<QObject *, QMetaProperty>> _properties;
    QMap<QString, std::function<void(const QVariant &)>> _set;
    QMap<QString, std::function<QVariant()>> _get;

    QMap<QPair<QObject *, int>, std::function<void()>> _notify;
    QMap<QString, std::function<QVariant(const QVariantList &)>> _call;

    int _notifierSlotIdx;
    QMetaMethod _notifierSlot;
};

#endif // QOBJECTREGISTRY_H
