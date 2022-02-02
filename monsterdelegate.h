#pragma once

#include "underlinedrowdelegate.h"

#include "searchablemultilistdatawiget.h"

class CharacterSheet;

class MonsterDelegate : public UnderlinedRowDelegate
{
private:
    Q_OBJECT

    using super = UnderlinedRowDelegate;

public:
    MonsterDelegate(int specializedColumn, const SearchableMultiListDataWiget::Data* data, const QStringList* itemList, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&option, const QModelIndex& index) const override;

protected:
    void setEditorData(QWidget* editor, const QModelIndex &index) const override;

    void OnCloseEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint = NoHint);

    void EditingFinished();

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    void CloseCompleter(QWidget* editor) const;

private:
    int m_SpecializedColumn { 0 };
    const SearchableMultiListDataWiget::Data* m_Data { nullptr };
    const QStringList* m_ItemList { nullptr };
    mutable bool m_DisableCompleter { false };
};
