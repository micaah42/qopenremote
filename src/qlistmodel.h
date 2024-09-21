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

class QListModelBase : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit QListModelBase(QObject *parent = nullptr)
        : QAbstractListModel{parent} {};

public slots:
    virtual QVariant atX(int i) const = 0;
    virtual void setX(int i, const QVariant &value) = 0;

    virtual void reserve(int size) = 0;
    virtual void clear() = 0;

    virtual void append(const QList<QVariant> &variants) = 0;
    virtual void append(const QVariant &variant) = 0;
    virtual void prepend(const QVariant &variant) = 0;

    virtual void insert(int i, const QVariant &variant) = 0;

    virtual int removeAll(const QVariant &variant) = 0;
    virtual bool removeOne(const QVariant &variant) = 0;
    virtual void removeAt(int i) = 0;

    QVariant takeAtX(int i);
    QVariant takeFirstX();
    QVariant takeLastX();

    virtual void swapItemsAt(int i, int j) = 0;
    virtual void move(int from, int to) = 0;

    virtual bool contains(const QVariant &variant) const = 0;
    virtual int lastIndexOf(const QVariant &variant, int from = -1) const = 0;
    virtual int indexOf(const QVariant &variant, int from = 0) const = 0;
    virtual int count(const QVariant &variant) const = 0;
    virtual int size() const = 0;

public:
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override { return this->atX(index.row()); };
    // virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    // virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    // virtual bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    // virtual void sort(int column, Qt::SortOrder order) override=;
};

template<class T>
class QListModel : public QListModelBase
{
public:
    explicit QListModel(QObject *parent = nullptr)
        : QListModelBase{parent} {};

    virtual QVariant atX(int i) const override { return QVariant::fromValue(_list.at(i)); };
    const T &at(int i) const { return _list.at(i); };

    virtual void setX(int i, const QVariant &value) override
    {
        Q_ASSERT(value.canConvert<T>());
        this->set(i, value.value<T>());
    };

    void set(int i, const T &t)
    {
        _list[i] = t;
        emit dataChanged(index(i), index(i));
    };

    virtual void reserve(int size) override { _list.reserve(size); };

    virtual void clear() override
    {
        beginRemoveRows(QModelIndex(), 0, _list.size() - 1);
        _list.clear();
        endRemoveRows();
    };

    virtual void append(const QList<QVariant> &variants) override
    {
        auto ts = qFromVariantList<T>(variants);
        this->append(ts);
    };

    void append(const QList<T> &ts)
    {
        beginInsertRows(QModelIndex(), _list.size(), _list.size() + ts.size() - 1);
        _list.append(ts);
        endInsertRows();
    };

    virtual void append(const QVariant &variant) override
    {
        Q_ASSERT(variant.canConvert<T>());
        this->append(variant.value<T>());
    };
    void append(const T &t)
    {
        beginInsertRows(QModelIndex(), _list.size(), _list.size());
        _list.append(t);
        endInsertRows();
    };

    virtual void prepend(const QVariant &variant) override
    {
        Q_ASSERT(variant.canConvert<T>());
        this->prepend(variant.value<T>());
    };
    void prepend(const T &t)
    {
        beginInsertRows(QModelIndex(), 0, 0);
        _list.prepend(t);
        endInsertRows();
    };

    virtual void insert(int i, const QVariant &variant) override
    {
        Q_ASSERT(variant.canConvert<T>());
        this->insert(i, variant.value<T>());
    };
    void insert(int i, const T &t)
    {
        beginInsertRows(QModelIndex(), i, i);
        _list.insert(i, t);
        endInsertRows();
    };

    virtual int removeAll(const QVariant &variant) override
    {
        Q_ASSERT(variant.canConvert<T>());
        return this->removeAll(variant.value<T>());
    };
    int removeAll(const T &t)
    {
        int i = 0;

        while (this->removeOne(t))
            i++;

        return i;
    };

    virtual bool removeOne(const QVariant &variant) override
    {
        Q_ASSERT(variant.canConvert<T>());
        return this->removeOne(variant.value<T>());
    };

    bool removeOne(const T &t)
    {
        int index = this->indexOf(t);

        if (index == -1)
            return false;

        this->removeAt(index);
        return true;
    };

    virtual void removeAt(int i) override
    {
        beginRemoveRows(QModelIndex(), i, i);
        _list.remove(i);
        endRemoveRows();
    };

    virtual void swapItemsAt(int i, int j) override
    {
        _list.swapItemsAt(i, j);
        emit dataChanged(index(i), index(i));
        emit dataChanged(index(j), index(j));
    };

    virtual void move(int from, int to) override
    {
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        _list.move(from, to);
        endMoveRows();
    };

    virtual bool contains(const QVariant &variant) const override
    {
        Q_ASSERT(variant.canConvert<T>());
        return this->contains(variant.value<T>());
    };
    bool contains(const T &t) const { return _list.contains(t); };

    virtual int lastIndexOf(const QVariant &variant, int from = -1) const override
    {
        Q_ASSERT(variant.canConvert<T>());
        return this->lastIndexOf(variant.value<T>(), from);
    };
    int lastIndexOf(const T &t, int from = -1) const { return _list.lastIndexOf(t, from); };

    virtual int indexOf(const QVariant &variant, int from) const override
    {
        Q_ASSERT(variant.canConvert<T>());
        return this->indexOf(variant.value<T>(), from);
    };
    int indexOf(const T &t, int from = -1) const { return _list.indexOf(t, from); };

    virtual int count(const QVariant &variant) const override
    {
        Q_ASSERT(variant.canConvert<T>());
        return this->count(variant.value<T>());
    };

    int count(const T &t) const { return _list.count(t); };
    virtual int size() const override { return _list.size(); };

private:
    QList<T> _list;
};

#endif // QLISTMODEL_H
