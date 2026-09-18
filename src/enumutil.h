#ifndef ENUMUTIL_H
#define ENUMUTIL_H

#include "qopenremote_global.h"

#include <QMetaEnum>
#include <QObject>

class EnumUtilBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QList<int> values READ values CONSTANT FINAL)
    Q_PROPERTY(QStringList keys READ keys CONSTANT FINAL)

public:
    explicit EnumUtilBase(const QMetaEnum &enum_, QObject *parent = nullptr);
    const QList<int> &values() const;
    const QStringList &keys() const;

public slots:
    int keyToValue(const QString &key) const { return _enum.keyToValue(qUtf8Printable(key)); };
    QString valueToKey(int value) const { return _enum.valueToKey(value); };

private:
    QMetaEnum _enum;
    QList<int> _values;
    QStringList _keys;
};

template<typename Enum>
class EnumUtil : public EnumUtilBase
{
public:
    explicit EnumUtil(QObject *parent = nullptr)
        : EnumUtilBase{QMetaEnum::fromType<Enum>(), parent}
    {}
};

template<typename Enum>
QString enumValueToKey(Enum value)
{
    static auto const metaEnum = QMetaEnum::fromType<Enum>();
    return metaEnum.valueToKey(static_cast<int>(value));
}

template<typename Enum, Enum errorValue>
Enum enumKeyToValue(const QString &key)
{
    static auto const metaEnum = QMetaEnum::fromType<Enum>();

    bool ok;
    auto value = static_cast<Enum>(metaEnum.keyToValue(qUtf8Printable(key), &ok));

    if (!ok)
        return errorValue;
    else
        return value;
}

#endif // ENUMUTIL_H
