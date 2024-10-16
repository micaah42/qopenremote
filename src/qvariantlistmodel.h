#ifndef QVARIANTLISTMODEL_H
#define QVARIANTLISTMODEL_H

#include <QAbstractListModel>

class QVariantListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit QVariantListModel(QObject *parent = nullptr);

    /* qlist like interface for c++ */
    const QVariant &operator[](int i) const;

    const QVariant &at(int i) const;

    void reserve(int size);
    void clear();

    void append(const QList<QVariant> &variants);
    void append(const QVariant &variant);
    void prepend(const QVariant &variant);

    void insert(int i, const QVariant &variant);
    void replace(int i, const QVariant &variant);

    int removeAll(const QVariant &variant);
    bool removeOne(const QVariant &variant);
    void removeAt(int i);

    QVariant takeAt(int i);
    QVariant takeFirst();
    QVariant takeLast();

    void swapItemsAt(int i, int j);
    void move(int from, int to);

    bool contains(const QVariant &variant) const;
    int lastIndexOf(const QVariant &variant, int from = -1) const;
    int indexOf(const QVariant &variant, int from = 0) const;
    int count(const QVariant &variant) const;

    int size() const { return _list.size(); };

    /* model interface for qml list views */

    virtual QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    void sort(int column, Qt::SortOrder order) override;

private:
    std::function<bool(const QVariant &, const QVariant &)> _compFn;
    QVariantList _list;
};

#endif // QVARIANTLISTMODEL_H
