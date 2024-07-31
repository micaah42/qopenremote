#ifndef QLISTMODEL_H
#define QLISTMODEL_H

#include <qvariantlistmodel.h>

template<class T>
QVariantList qToVariantList(const QList<T> list)
{
    QVariantList variants;
    for (auto const &v : qAsConst(list))
        variants.append(QVariant::fromValue(v));

    return variants;
}

template<class T>
const QList<T> qFromVariantList(QVariantList variants)
{
    QList<T> list;
    for (auto const &v : qAsConst(variants))
        list.append(v.value<T>());

    return list;
}

template<class T>
class QListModel : public QVariantListModel
{
public:
    explicit QListModel(QObject *parent = nullptr);

    // qlist like interface for c++

    const T &at(int i) const;
    const T &operator[](int i) const;

    void append(const QList<T> &variants);
    void append(const T &variant);
    void prepend(const T &variant);

    void replace(int i, const T &variant);
    void insert(int i, const T &variant);

    void removeAt(int i);
    bool removeOne(const T &variant);
    int removeAll(const T &variant);

    T takeAt(int i);
    T takeFirst();
    T takeLast();

    // bool contains(const T &variant) const;
    // int lastIndexOf(const T &variant, int from = -1) const;
    // int indexOf(const T &variant, int from = 0) const;
    // int count(const T &variant) const;
};

template<class T>
QListModel<T>::QListModel(QObject *parent)
    : QVariantListModel{parent}
{}

template<class T>
const T &QListModel<T>::at(int i) const
{
    return *reinterpret_cast<const T *>(QVariantListModel::at(i).constData());
}

template<class T>
const T &QListModel<T>::operator[](int i) const
{
    return *reinterpret_cast<const T *>(QVariantListModel::at(i).constData());
}

template<class T>
void QListModel<T>::append(const QList<T> &variants)
{
    QVariantListModel::append(qToVariantList(variants));
}

template<class T>
void QListModel<T>::append(const T &variant)
{
    QVariantListModel::append(QVariant::fromValue(variant));
}

template<class T>
void QListModel<T>::prepend(const T &variant)
{
    QVariantListModel::prepend(QVariant::fromValue(variant));
}

template<class T>
void QListModel<T>::replace(int i, const T &variant)
{
    QVariantListModel::replace(i, QVariant::fromValue(variant));
}

template<class T>
void QListModel<T>::insert(int i, const T &variant)
{
    QVariantListModel::insert(i, QVariant::fromValue(variant));
}

template<class T>
int QListModel<T>::removeAll(const T &variant)
{
    return QVariantListModel::removeAll(QVariant::fromValue(variant));
}

template<class T>
bool QListModel<T>::removeOne(const T &variant)
{
    return QVariantListModel::removeOne(QVariant::fromValue(variant));
}

template<class T>
void QListModel<T>::removeAt(int i)
{
    QVariantListModel::removeAt(i);
}

template<class T>
T QListModel<T>::takeAt(int i)
{
    QVariantListModel::takeAt(i);
}

template<class T>
T QListModel<T>::takeFirst()
{
    return QVariantListModel::takeFirst();
}

template<class T>
T QListModel<T>::takeLast()
{
    return QVariantListModel::takeLast();
}

#endif // QLISTMODEL_H
