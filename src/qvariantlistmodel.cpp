#include "qvariantlistmodel.h"

QVariantListModel::QVariantListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

const QVariant &QVariantListModel::at(int i) const
{
    return _list.at(0);
}

QHash<int, QByteArray> QVariantListModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{Qt::UserRole, "value"}};
    return roleNames;
}

int QVariantListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _list.size();
}

QVariant QVariantListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::UserRole) {
        return QVariant();
    }

    return _list[index.row()];
}

bool QVariantListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (_list[index.row()] == value)
        return false;

    if (role != Qt::UserRole) {
        return false;
    }

    _list[index.row()] = value;
    emit dataChanged(index, index, {role});

    return true;
}

Qt::ItemFlags QVariantListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool QVariantListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i)
        _list.insert(row, QVariant());

    endInsertRows();
    return true;
}

bool QVariantListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; ++i)
        _list.removeAt(row);

    endRemoveRows();
    return true;
}
