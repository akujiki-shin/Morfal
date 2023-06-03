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

#include "underlinedrowdelegate.h"

#include <QPainter>
#include <QApplication>
#include <QTableWidget>
#include <QWidget>

UnderlinedRowDelegate::UnderlinedRowDelegate(QObject* parent)
    : super(parent)
{

}

void UnderlinedRowDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    super::paint(painter, option, index);

    if (const QTableWidget* widget = qobject_cast<const QTableWidget*>(option.widget))
    {
        painter->save();

        for (auto& [row, rowBrushes] : m_RowBrushesMap.toStdMap())
        {
            for (int i = 0; i <  widget->columnCount(); ++i)
            {
                if (QTableWidgetItem* toDisable = widget->item(row, i))
                {
                    toDisable->setBackground(rowBrushes.m_BackgroundBrush);
                    toDisable->setForeground(rowBrushes.m_ForegroundBrush);
                }
            }
        }

        if (index.row() == m_UnderLinedRow)
        {
            QStyle* style = widget ? widget->style() : QApplication::style();
            QRect focusRect = style->subElementRect(QStyle::SE_ItemViewItemFocusRect, &option, option.widget);

            painter->setBrush(Qt::NoBrush);

            QPen pen(QColor(255, 211, 135));
            pen.setWidth(4);
            pen.setStyle(Qt::SolidLine);
            painter->setPen(pen);

            focusRect.setTop(focusRect.bottom());
            painter->drawRect(focusRect);
        }

        painter->restore();
    }
}

void UnderlinedRowDelegate::SetUnderlinedRow(int row)
{
    m_UnderLinedRow = row;
}

void UnderlinedRowDelegate::SetRowBrushes(int row, const QBrush& foregroundBrush, const QBrush& backgroundBrush)
{
    RowBrushes& brushes = m_RowBrushesMap[row];
    brushes.m_ForegroundBrush = foregroundBrush;
    brushes.m_BackgroundBrush = backgroundBrush;
}
