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

public slots:
    QVariant call(const QString &function, const QVariantList &arguments);

    void set(const QString &key, const QVariant &value);
    QVariant get(const QString &key);

signals:
    void valueChanged(const QString &key, const QVariant &value);

private slots:
    void registerProperty(const QString &propertyName, QObject *object, const QMetaProperty &property);
    void registerMethod(const QString &methodName, QObject *object, const QMetaMethod &method);
    void onNotifySignal();

private:
    QMap<QString, QPair<QObject *, QMetaProperty>> _properties;
    QMap<QString, QPair<QObject *, QMetaMethod>> _methods;

    //QMap<QString, std::function<QVariant()>> _read;
    QMap<QPair<QObject *, int>, std::function<void()>> _notify;
    //QMap<QString, std::function<void(const QVariant &value)>> _write;
    QMap<QString, std::function<QVariant(const QVariantList &)>> _call;

    int _notifierSlotIdx;
    QMetaMethod _notifierSlot;
};

#endif // QOBJECTREGISTRY_H
