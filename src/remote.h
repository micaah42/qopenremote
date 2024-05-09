#ifndef REMOTE_H
#define REMOTE_H

#include <QMap>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QObject>

#include "bijectivemap.h"

class QObjectRegistry : public QObject
{
    Q_OBJECT
public:
    explicit QObjectRegistry(QObject *parent = nullptr);
    void registerObject(const QString &name, QObject *object);

public slots:
    QVariant value(const QString &key);
    void setValue(const QString &key, const QVariant &value);

    QVariant call(const QString &function, const QVariantList &arguments);

signals:
    void valueChanged(const QString &key, const QVariant &value);

private slots:
    void onNotifySignal();

private:
    BijectiveMap<QString, QObject *> _objects;
    QMap<QString, std::function<QVariant()>> _read;
    QMap<QString, std::function<void(const QVariant &value)>> _write;
    QMap<QPair<QObject *, int>, std::function<void()>> _notify;

    QMap<QString, QMetaMethod> _functions;
    QMap<QString, QMetaProperty> _properties;

    int _notifierSlotIdx;
    QMetaMethod _notifierSlot;
};

#endif // REMOTE_H
