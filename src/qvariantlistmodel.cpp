#include "qvariantlistmodel.h"

QVariantListModel::QVariantListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

const QVariant &QVariantListModel::at(int i) const
{
    return _list.at(i);
}

void QVariantListModel::reserve(int size)
{
    _list.reserve(size);
}

void QVariantListModel::append(const QList<QVariant> &variants)
{
    beginInsertRows(QModelIndex(), _list.size(), _list.size() + variants.size() - 1);
    _list.append(variants);
    endInsertRows();
}

void QVariantListModel::append(const QVariant &variant)
{
    beginInsertRows(QModelIndex(), _list.size(), _list.size());
    _list.append(variant);
    endInsertRows();
}

void QVariantListModel::prepend(const QVariant &variant)
{
    beginInsertRows(QModelIndex(), 0, 0);
    _list.append(variant);
    endInsertRows();
}

void QVariantListModel::insert(int i, const QVariant &variant)
{
    beginInsertRows(QModelIndex(), i, i);
    _list.append(variant);
    endInsertRows();
}

void QVariantListModel::replace(int i, const QVariant &variant)
{
    _list[i] = variant;
    emit dataChanged(index(i), index(i));
}

int QVariantListModel::removeAll(const QVariant &variant)
{
    int ret = 0;

    for (int i = 0; i < _list.size(); ++i)
        if (variant == _list[i]) {
            this->removeAt(i);
            ret++;
            i--;
        }

    return ret;
}

bool QVariantListModel::removeOne(const QVariant &variant)
{
    for (int i = 0; i < _list.size(); ++i)
        if (variant == _list[i]) {
            this->removeAt(i);
            return true;
        }

    return false;
}

void QVariantListModel::removeAt(int i)
{
    beginRemoveRows(QModelIndex(), i, i);
    _list.removeAt(i);
    endRemoveRows();
}

QVariant QVariantListModel::takeAt(int i)
{
    auto ret = _list[i];
    this->removeAt(i);
    return ret;
}

QVariant QVariantListModel::takeFirst()
{
    return this->takeAt(0);
}

QVariant QVariantListModel::takeLast()
{
    return this->takeAt(_list.size() - 1);
}

void QVariantListModel::swapItemsAt(int i, int j)
{
    _list.swapItemsAt(i, j);
    emit dataChanged(index(i), index(i));
    emit dataChanged(index(j), index(j));
}

QHash<int, QByteArray> QVariantListModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{Qt::UserRole + 1, "value"}};
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

    // if (role != Qt::DisplayRole) {
    //     return QVariant();
    // }

    return _list[index.row()];
}

bool QVariantListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (_list[index.row()] == value)
        return false;

    // if (role != Qt::UserRole) {
    //     return false;
    // }

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

bool QVariantListModel::moveRows(const QModelIndex &srcParent, int srcRow, int count, const QModelIndex &dstParent, int dstChild)
{
    if (!dstParent.isValid() || !srcParent.isValid())
        return false;

    if (srcRow == dstChild || srcRow == dstChild - 1 || count <= 0)
        return false;

    if (srcRow < 0 || srcRow + count - 1 >= rowCount(srcParent))
        return false;

    if (dstChild < 0 || dstChild > rowCount(dstParent))
        return false;

    if (!beginMoveRows(QModelIndex(), srcRow, srcRow + count - 1, QModelIndex(), dstChild))
        return false;

    int fromRow = srcRow;

    if (dstChild < srcRow)
        fromRow += count - 1;
    else
        dstChild--;
    while (count--)
        _list.move(fromRow, dstChild);

    endMoveRows();
    return true;
}

void QVariantListModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), VerticalSortHint);

    QVector<QPair<QVariant, int>> list;
    const int lstCount = _list.size();
    list.reserve(lstCount);

    for (int i = 0; i < lstCount; ++i)
        list.append({_list[i], i});

    if (order == Qt::AscendingOrder)
        std::sort(list.begin(), list.end(), [this](const QPair<QVariant, int> &a, const QPair<QVariant, int> &b) { return _compFn(a.first, b.first); });
    else
        std::sort(list.begin(), list.end(), [this](const QPair<QVariant, int> &a, const QPair<QVariant, int> &b) { return _compFn(b.first, a.first); });

    _list.clear();

    QVector<int> forwarding(lstCount);

    for (int i = 0; i < lstCount; ++i) {
        _list.append(list.at(i).first);
        forwarding[list.at(i).second] = i;
    }

    QModelIndexList oldList = persistentIndexList();
    QModelIndexList newList;

    const int numOldIndexes = oldList.count();
    newList.reserve(numOldIndexes);

    for (int i = 0; i < numOldIndexes; ++i)
        newList.append(index(forwarding.at(oldList.at(i).row()), 0));

    changePersistentIndexList(oldList, newList);
    emit layoutChanged(QList<QPersistentModelIndex>(), VerticalSortHint);
}
