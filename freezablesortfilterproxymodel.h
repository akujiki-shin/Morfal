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

        if (ShouldForceFilterOut(index))
        {
            return false;
        }

        return super::filterAcceptsRow(sourceRow, sourceIndex);
    }

    using IgnoreFilterPredicate = bool (PredicateHolder::*)(const QString&) const;
    using ForceFilterOutPredicate = bool (PredicateHolder::*)(const QString& fileName, const QString& itemName) const;

    void SetIgnoreFilterPredicate(PredicateHolder* predicateHolder, IgnoreFilterPredicate predicate)
    {
        m_IgnoreFilterPredicate = predicate;
        m_PredicateHolder = predicateHolder;
    }

    void SetForceFilterOutPredicate(PredicateHolder* predicateHolder, ForceFilterOutPredicate predicate)
    {
        m_ForceFilterOutPredicate = predicate;
        m_PredicateHolder = predicateHolder;
    }

    void SetDataFileName(const QString& fileName)
    {
        m_FileName = fileName;
    }

private:
    bool ShouldIgnoreFilter(const QModelIndex& index) const
    {
        bool ignoreFilter = false;

        if (m_IgnoreFilterPredicate != nullptr)
        {
            const QString& data = index.data().toString();
            ignoreFilter = (m_PredicateHolder->*m_IgnoreFilterPredicate)(data);
        }

        return ignoreFilter;
    }

    bool ShouldForceFilterOut(const QModelIndex& index) const
    {
        bool forceFilterOut = false;

        if (m_ForceFilterOutPredicate != nullptr)
        {
            const QString& data = index.data().toString();
            forceFilterOut = (m_PredicateHolder->*m_ForceFilterOutPredicate)(m_FileName, data);
        }

        return forceFilterOut;
    }

private:
    IgnoreFilterPredicate m_IgnoreFilterPredicate { nullptr };
    ForceFilterOutPredicate m_ForceFilterOutPredicate { nullptr };
    PredicateHolder* m_PredicateHolder { nullptr };
    QString m_FileName{""};
};
