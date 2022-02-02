#pragma once

#include <QSortFilterProxyModel>

template<class PredicateHolder>
class FreezableSortFilterProxyModel : public QSortFilterProxyModel
{
private:
    using super = QSortFilterProxyModel;

public:
    FreezableSortFilterProxyModel(QObject* parent) : super(parent) {}

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceIndex) const override
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceIndex);
        if (ShouldIgnoreFilter(index))
        {
            return true;
        }

        return super::filterAcceptsRow(sourceRow, sourceIndex);
    }

    using Predicate = bool (PredicateHolder::*)(const QString&) const;

    void SetPredicate(PredicateHolder* predicateHolder, Predicate predicate)
    {
        m_Predicate = predicate;
        m_PredicateHolder = predicateHolder;
    }

private:
    bool ShouldIgnoreFilter(const QModelIndex& index) const
    {
        const QString& data = index.data().toString();
        return (m_PredicateHolder->*m_Predicate)(data);
    }

    Predicate m_Predicate { nullptr };
    PredicateHolder* m_PredicateHolder { nullptr };
};
