#ifndef SORTFILTERPROXYLISTMODEL_H
#define SORTFILTERPROXYLISTMODEL_H

#include <QObject>
#include <QSortFilterProxyModel>

#include "listmodel.h"

class SortFilterProxyListModelBase : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(ListModelBase *listModel READ listModel WRITE setListModel NOTIFY listModelChanged FINAL)

public:
    explicit SortFilterProxyListModelBase(QObject *parent = nullptr);

    virtual ListModelBase *listModel() const = 0;
    virtual void setListModel(ListModelBase *newListModel) = 0;
signals:
    void listModelChanged();
};

template<class T>
class SortFilterProxyListModel : public SortFilterProxyListModelBase
{
public:
    explicit SortFilterProxyListModel(QObject *parent = nullptr)
        : SortFilterProxyListModelBase{parent}
    {
        connect(this, &SortFilterProxyListModelBase::sourceModelChanged, this, [this]() {
            if (this->sourceModel() == nullptr)
                return;

            auto newListModel = dynamic_cast<ListModel<T> *>(this->sourceModel());
            _listModel = newListModel;
            this->sort(0);

            if (_listModel == nullptr) {
                qWarning() << "invalid source model set:" << this->sourceModel() << "resetting to null!";
                this->setSourceModel(nullptr);
            }
        });
    }

    virtual ListModelBase *listModel() const override { return _listModel; };
    virtual ListModel<T> *listModelTyped() const { return _listModel; };

    virtual void setListModel(ListModelBase *newListModel) override
    {
        auto newListModelTyped = dynamic_cast<ListModel<T> *>(newListModel);

        if (newListModelTyped == nullptr) {
            qWarning() << "invalid list model set:" << newListModel << "ignoring...";
            return;
        }

        this->setListModel(newListModelTyped);
    };

    virtual void setListModel(ListModel<T> *newListModel)
    {
        _listModel = newListModel;
        this->setSourceModel(_listModel);
        this->sort(0);
    };

    virtual bool filterAcceptsRow(int index, const T t) const = 0;
    virtual bool lessThan(const T a, const T b) const = 0;

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        if (!_listModel)
            return false;

        return this->filterAcceptsRow(source_row, _listModel->at(source_row));
    }

    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override
    {
        T a = _listModel->at(source_left.row());
        T b = _listModel->at(source_right.row());
        return this->lessThan(a, b);
    };

private:
    ListModel<T> *_listModel = nullptr;

    // QSortFilterProxyModel interface
protected:
};

#endif // SORTFILTERPROXYLISTMODEL_H
