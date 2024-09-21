#include "qlistmodel.h"

QVariant QListModelBase::takeAtX(int i)
{
    auto temp = this->atX(i);
    this->removeAt(i);
    return temp;
}

QVariant QListModelBase::takeFirstX()
{
    return this->takeAtX(0);
}

QVariant QListModelBase::takeLastX()
{
    return this->takeAtX(this->size() - 1);
}

QHash<int, QByteArray> QListModelBase::roleNames() const
{
    static const QHash<int, QByteArray> roleNames{{Qt::UserRole + 1, "value"}};
    return roleNames;
}

Qt::ItemFlags QListModelBase::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int QListModelBase::rowCount(const QModelIndex &parent) const
{
    return this->size();
}

bool QListModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    this->setX(index.row(), value);
    return true;
}
