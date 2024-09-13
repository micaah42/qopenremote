#ifndef QLISTMODEL_H
#define QLISTMODEL_H

#include <QAbstractListModel>

class QListModelBase : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit QListModelBase(QObject *parent = nullptr);

    // handling for qml viewmodel

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public slots:

    // variant based core list functions

    virtual bool hasTypeOf(const QVariant &variant) const = 0;
    virtual QVariant newVariant() const = 0;
    virtual const QVariant &at(int i) const = 0;
    virtual void set(int i, const QVariant &variant) = 0;
    virtual void insert(int i, const QVariant &variant) = 0;
    virtual void remove(int i) = 0;
    virtual int size() const = 0;

    // convenience

    void append(const QVariant &variant) { insert(size(), variant); };
    void prepend(const QVariant &variant) { insert(0, variant); };
};

template<class T>
class QListModel : public QListModelBase
{
public:
    explicit QListModel(QObject *parent = nullptr);

    virtual bool hasTypeOf(const QVariant &variant) const { return variant.canConvert<T>(); };

    virtual QVariant newVariant() const override { return QVariant::fromValue(this->newT()); };
    T newT() const { return T(); };

    virtual const QVariant &at(int i) const override { return QVariant::fromValue(_ts[i]); };

    const T &operator[](int i) const { return _ts[i]; };
    T &operator[](int i) { return _ts[i]; };

    virtual void set(int i, const QVariant &variant) override
    {
        Q_ASSERT(hasTypeOf(variant));
        _ts[i] = variant.value<T>();
    };

    virtual void insert(int i, const QVariant &variant) override
    {
        Q_ASSERT(hasTypeOf(variant));

        beginInsertRows(QModelIndex(), i, i);
        _ts.insert(i, variant.value<T>());
        endInsertRows();
    };

    virtual void remove(int i) override
    {
        beginRemoveRows(QModelIndex(), i, i);
        _ts.remove(i);
        endRemoveRows();
    };
    virtual int size() const override { return _ts.size(); };

private:
    QList<T> _ts;
};

#endif // QLISTMODEL_H
