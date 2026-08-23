#ifndef VARIANTUTILS_H
#define VARIANTUTILS_H

#include <QVariantList>

template<class T>
QList<T> variantList2Typed(const QVariantList &variantList)
{
    QList<T> result;
    result.reserve(variantList.size());

    for (auto const &variant : variantList)
        result.append(variant.value<T>());

    return result;
};

template<class T>
QVariantList typedList2Variant(const QList<T> &typedList)
{
    QVariantList result;
    result.reserve(typedList.size());

    for (auto const &t : typedList)
        result.append(QVariant::fromValue(t));

    return result;
};

#endif // VARIANTUTILS_H
