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
