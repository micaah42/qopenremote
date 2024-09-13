#include "qlistmodel.h"

QListModelBase::QListModelBase(QObject *parent)
    : QAbstractListModel(parent)
{}

int QListModelBase::rowCount(const QModelIndex &parent) const
{
    return size();
}

QVariant QListModelBase::data(const QModelIndex &index, int role) const
{
    return at(index.row());
}

bool QListModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    set(index.row(), value);
    return true;
}

Qt::ItemFlags QListModelBase::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool QListModelBase::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);

    for (int i = row; i < row + count; i++)
        this->insert(i, this->newVariant());

    endInsertRows();
    return true;
}

bool QListModelBase::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++)
        this->remove(row);

    endRemoveRows();
    return true;
}
