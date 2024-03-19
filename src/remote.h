#ifndef REMOTE_H
#define REMOTE_H

#include <QMap>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QObject>

template<class A, class B>
class BiMap
{
public:
    const A &b2a(const B &b) const { return _a2b[b]; };
    A &b2a(const B &b) { return _a2b[b]; };

    const B &a2b(const A &a) const { return _b2a[a]; };
    B &a2b(const A &a) { return _b2a[a]; };

    void insert(const A &a, const B &b)
    {
        _a2b.insert(a, b);
        _b2a.insert(b, a);
    };

    void remove(const A &a, const B &b)
    {
        _a2b.remove(a, b);
        _b2a.remove(b, a);
    };

    const QMap<A, B> a2bMap() const { return _a2b; };
    const QMap<B, A> b2aMap() const { return _b2a; };

protected:
    // changing a single map can break validity!
    [[deprecated]] QMap<A, B> a2bMap() { return _a2b; };
    [[deprecated]] QMap<A, B> b2aMap() { return _b2a; };

private:
    QMap<A, B> _a2b;
    QMap<B, A> _b2a;
};

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
    BiMap<QString, QObject *> _objects;
    QMap<QString, std::function<QVariant()>> _read;
    QMap<QString, std::function<void(const QVariant &value)>> _write;
    QMap<QPair<QObject *, int>, std::function<void()>> _notify;

    QMap<QString, QMetaMethod> _functions;
    QMap<QString, QMetaProperty> _properties;

    int _notifierSlotIdx;
    QMetaMethod _notifierSlot;
};

#endif // REMOTE_H
