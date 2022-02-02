#include "monsterdelegate.h"

#include <QLineEdit>
#include <QCompleter>
#include <QPainter>

MonsterDelegate::MonsterDelegate(int specializedColumn, const SearchableMultiListDataWiget::Data* data, const QStringList* itemList, QObject* parent )
    : super(parent)
    , m_SpecializedColumn(specializedColumn)
    , m_Data(data)
    , m_ItemList(itemList)
{
}

QWidget* MonsterDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == m_SpecializedColumn)
    {
        QLineEdit* editor = new QLineEdit(parent);

        QCompleter* completer = new QCompleter(*m_ItemList, parent);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setMaxVisibleItems(10);
        editor->setCompleter(completer);

        m_DisableCompleter = false;

        connect(completer, QOverload<const QString &>::of(&QCompleter::activated),this,
                [this, editor, index](const QString& /*text*/)
        {
            setModelData(editor, const_cast<QAbstractItemModel*>(index.model()), index);
            CloseCompleter(editor);
        });

        connect(editor, &QLineEdit::editingFinished, this, &MonsterDelegate::EditingFinished);

        connect(this, &MonsterDelegate::closeEditor, this, &MonsterDelegate::OnCloseEditor);

        return editor;
    }

    return super::createEditor(parent, option, index);
}

void MonsterDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == m_SpecializedColumn)
    {
        QLineEdit* lineEditor = qobject_cast<QLineEdit*>(editor);
        lineEditor->setText(index.data().toString());

        if (!m_DisableCompleter)
        {
            lineEditor->completer()->complete();
        }
    }
    else
    {
        super::setEditorData(editor, index);
    }
}

void MonsterDelegate::OnCloseEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint /*hint = NoHint*/)
{
    CloseCompleter(editor);
}

void MonsterDelegate::EditingFinished()
{
    QLineEdit* editor = qobject_cast<QLineEdit*>(sender());

    emit closeEditor(editor);
    emit commitData(editor);
}

void MonsterDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (index.column() == m_SpecializedColumn)
    {
        QLineEdit* lineEditor = qobject_cast<QLineEdit*>(editor);
        model->setData(index, lineEditor->text(), Qt::DisplayRole);
    }
    else
    {
        super::setModelData(editor, model, index);
    }
}

void MonsterDelegate::CloseCompleter(QWidget* editor) const
{
    if (const QLineEdit* lineEditor = qobject_cast<QLineEdit*>(editor))
    {
        if (lineEditor->completer() != nullptr)
        {
            lineEditor->completer()->popup()->close();
            m_DisableCompleter = true;
        }
    }
}
