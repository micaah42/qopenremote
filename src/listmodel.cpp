#include "listmodel.h"

ListModelBase::ListModelBase(QObject *parent)
    : QAbstractListModel{parent}
{
    connect(this, &QAbstractListModel::rowsInserted, this, &ListModelBase::lengthChanged);
    connect(this, &QAbstractListModel::rowsRemoved, this, &ListModelBase::lengthChanged);

    connect(this, &QAbstractListModel::rowsInserted, this, &ListModelBase::asListChanged);
    connect(this, &QAbstractListModel::rowsRemoved, this, &ListModelBase::asListChanged);
    connect(this, &QAbstractListModel::layoutChanged, this, &ListModelBase::asListChanged);
    connect(this, &QAbstractListModel::rowsMoved, this, &ListModelBase::asListChanged);

    connect(this, &QAbstractListModel::dataChanged, this, &ListModelBase::asListChanged);
}

QVariant ListModelBase::takeAtX(int i)
{
    auto temp = this->atX(i);
    this->removeAt(i);
    return temp;
}

QVariant ListModelBase::takeFirstX()
{
    return this->takeAtX(0);
}

QVariant ListModelBase::takeLastX()
{
    return this->takeAtX(this->size() - 1);
}

QHash<int, QByteArray> ListModelBase::roleNames() const
{
    static const QHash<int, QByteArray> roleNames{{Qt::UserRole + 1, "value"}};
    return roleNames;
}

Qt::ItemFlags ListModelBase::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int ListModelBase::rowCount(const QModelIndex &parent) const
{
    return this->size();
}

bool ListModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    this->setX(index.row(), value);
    return true;
}

int ListModelBase::length() const
{
    return this->size();
}
