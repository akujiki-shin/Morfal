// Morfal is a Virtual Pen and Paper (VPP): a Game Master's toolbox designed for those who play Role Playing Games with pen and paper only,
// as opposed to Virtual Table Tops (VTT) which are made to handle tactical gameplay (tokens, square/hexagons, area of effect, etc...).
// Copyright (C) 2022  akujiki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
