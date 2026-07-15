#include "enumutil.h"

EnumUtilBase::EnumUtilBase(const QMetaEnum &enum_, QObject *parent)
    : QObject{parent}
    , _enum{enum_}
{
    for (int i = 0; i < _enum.keyCount(); i++) {
        const auto key = _enum.key(i);
        _values.append(_enum.keyToValue(key));
        _keys.append(key);
    }
}

const QList<int> &EnumUtilBase::values() const
{
    return _values;
}

const QStringList &EnumUtilBase::keys() const
{
    return _keys;
}
