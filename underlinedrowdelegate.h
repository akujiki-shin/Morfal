#pragma once

#include <QStyledItemDelegate>

class UnderlinedRowDelegate : public QStyledItemDelegate
{
private:
    Q_OBJECT

    using super = QStyledItemDelegate;

    struct RowBrushes
    {
        QBrush m_ForegroundBrush;
        QBrush m_BackgroundBrush;
    };

public:
    UnderlinedRowDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void SetUnderlinedRow(int row);
    void SetRowBrushes(int row, const QBrush& foregroundBrush, const QBrush& backgroundBrush);

private:
    int m_UnderLinedRow { -1 };
    QMap<int, RowBrushes> m_RowBrushesMap;
};
