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

#endif // ENUMUTIL_H
