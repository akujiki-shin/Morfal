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

#include "jsontoqtxmbuilder.h"

#include <QSpacerItem>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QString>
#include <QTextEdit>
#include <QListWidget>
#include <QJsonObject>
#include <QXmlStreamReader>

#include "JsonRetrievers/JsonArrayRetriever.h"
#include "JsonRetrievers/JsonConditionRetriever.h"
#include "JsonRetrievers/JsonMapRetriever.h"
#include "JsonRetrievers/JsonNumberRetriever.h"
#include "JsonRetrievers/JsonRetrieverHelper.h"
#include "JsonRetrievers/JsonStringRetriever.h"

#include "ui_mainwindow.h"

#include "utils.h"

QList<JsonToQtXmBuilder*> JsonToQtXmBuilder::m_SubListsDetails;

JsonToQtXmBuilder::JsonToQtXmBuilder(Ui::MainWindow* mainWindowUi, QWidget *parent)
    : super(parent)
    , ui(mainWindowUi)
{
    setLayout(new QVBoxLayout());
    setMaximumWidth(350);
}

bool JsonToQtXmBuilder::BuildFromXml(const QString& xmlPath, const QString& xmlFileName)
{
    bool success = false;

    m_XmlPath = xmlPath;
    const QString& fullPath = xmlPath + "/" + xmlFileName + ".xml";

    QFile file(fullPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QXmlStreamReader reader(&file);
        success = ReadXml(reader);

        if (success)
        {
            AlignNameLabel();
        }
    }

    return success;
}

void JsonToQtXmBuilder::FeedFromJson(const QJsonObject& jsonData)
{
    m_JSEngine.collectGarbage();
    m_ScriptResult.clear();

    for (FeedFunction& script : m_FeedScriptFunctions)
    {
        script(jsonData);
    }

    for (FeedFunction& feedFunction : m_FeedFunctions)
    {
        feedFunction(jsonData);
    }
}

void JsonToQtXmBuilder::SetDiceReveivers(const QStringList* diceExpressionReceivers)
{
    m_DiceExpressionReceivers = diceExpressionReceivers;
}

bool JsonToQtXmBuilder::ReadXml(QXmlStreamReader& reader)
{
    bool foundError = false;
    while (reader.readNext() && !reader.isEndDocument() && !foundError)
    {
        XmlTypes xmlType;
        if (JsonRetrieverHelper::TryReadElementAsType(reader, xmlType, foundError))
        {
            switch (xmlType)
            {
                case XmlTypes::title:
                {
                    BuildTitle(reader);
                }
                break;
                case XmlTypes::label:
                {
                    BuildLabel(reader);
                }
                break;
                case XmlTypes::separatorLine:
                {
                    BuildSeparatorLine(reader);
                }
                break;
                case XmlTypes::textArea:
                {
                    BuildTextArea(reader);
                }
                break;
                case XmlTypes::list:
                {
                    BuildList(reader);
                }
                break;
                case XmlTypes::stat:
                {
                    BuildStat(reader);
                }
                break;
                case XmlTypes::MathScript:
                {
                    RegisterScript(reader);
                }
                break;
            }
        }
    }

    return !foundError;
}

void JsonToQtXmBuilder::BuildTitle(QXmlStreamReader& reader)
{
    QLabel* label = new QLabel(this);
    QFont font;
    font.setBold(true);
    font.setPointSize(12);
    label->setFont(font);

    layout()->addWidget(label);

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [this, label, retrieverList](const QJsonObject& jsonData)
    {
        QString text;

        for (JSonRetriever* jsonRetriever : retrieverList)
        {
            text += jsonRetriever->ToString(jsonData, &m_ScriptResult);
        }

        label->setText(text);
    };

    m_FeedFunctions.append(feedFunction);
}

void JsonToQtXmBuilder::BuildLabel(QXmlStreamReader& reader)
{
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    const QString& name = reader.attributes().value("Name").toString();
    const bool italic = (reader.attributes().value("Italic").toString() == "true");
    const bool isUrl = (reader.attributes().value("IsUrl").toString() == "true");
    const QString linkColor = reader.attributes().value("LinkColor").toString();

    QFont font;
    font.setItalic(italic);

    QFrame* frame = new QFrame(this);
    frame->setStyleSheet("QFrame {border: none;}");
    frame->setLayout(hLayout);

    layout()->addWidget(frame);

    if (!name.isEmpty())
    {
        QLabel* nameLabel = new QLabel(name, this);
        nameLabel->setFont(font);
        hLayout->addWidget(nameLabel);

        m_NameLabelList.append(nameLabel);

        nameLabel->ensurePolished();
        int width = nameLabel->fontMetrics().horizontalAdvance(name);
        m_MaxNameLabelWidth = qMax(m_MaxNameLabelWidth, width);
    }

    QLabel* label = new QLabel(this);
    label->setFont(font);
    label->setWordWrap(true);

    if (isUrl)
    {
        label->setOpenExternalLinks(true);
    }

    hLayout->addWidget(label);

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [this, isUrl, linkColor, label, retrieverList](const QJsonObject& jsonData)
    {
        QString text;

        for (JSonRetriever* jsonRetriever : retrieverList)
        {
            text += jsonRetriever->ToString(jsonData, &m_ScriptResult);
        }

        if (isUrl)
        {
            text = "<a href=\"" + text + "\"><span style=\"color:" + linkColor + ";\">" + text + "</span></a>";
        }

        label->setText(text);
    };

    m_FeedFunctions.append(feedFunction);
}

void JsonToQtXmBuilder::AlignNameLabel()
{
    for (QLabel* label : m_NameLabelList)
    {
        label->setMinimumWidth(m_MaxNameLabelWidth);
        label->setMaximumWidth(m_MaxNameLabelWidth);
    }
}

void JsonToQtXmBuilder::BuildStat(QXmlStreamReader& reader)
{
    QFrame* statsFrame = GetOrCreateStatsFrame();

    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->setContentsMargins(0, 0, 0, 0);
    const QString& name = reader.attributes().value("Name").toString();

    QLabel* nameLabel = new QLabel(name, this);
    nameLabel->setMaximumWidth(75);

    QLabel* label = new QLabel(this);
    label->setWordWrap(false);
    QFrame* frame = new QFrame(this);
    frame->setStyleSheet("QFrame {border: none;}");
    frame->setLayout(vLayout);
    frame->setMinimumWidth(60);
    vLayout->addWidget(nameLabel);
    vLayout->addWidget(label);

    statsFrame->layout()->addWidget(frame);

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [this, label, retrieverList](const QJsonObject& jsonData)
    {
        QString text;

        for (JSonRetriever* jsonRetriever : retrieverList)
        {
            text += jsonRetriever->ToString(jsonData, &m_ScriptResult);
        }

        label->setText(text);
    };

    m_FeedFunctions.append(feedFunction);
}

void JsonToQtXmBuilder::BuildSeparatorLine(QXmlStreamReader& /*reader*/)
{
    QFrame* frame = new QFrame(this);
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);

    layout()->addWidget(frame);
}

void JsonToQtXmBuilder::BuildTextArea(QXmlStreamReader& reader)
{
    QTextEdit* textEdit = new QTextEdit(this);
    layout()->addWidget(textEdit);

    const bool expanding = (reader.attributes().value("Expanding").toString() == "true");
    if (expanding)
    {
        textEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    }
    else
    {
        textEdit->setMaximumHeight(120);
    }

    connect(textEdit, &QTextEdit::cursorPositionChanged, this, [this, textEdit]()
    {
        int cursor = textEdit->textCursor().position();
        const QString& fullText = textEdit->toPlainText();

        QString dice;
        if (DiceHelper::TryGetDiceExpression(fullText, dice, cursor))
        {
            emit DiceExpressionSelected(dice, false);
        }
    });

    textEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(textEdit, &QTextEdit::customContextMenuRequested, this, [this, textEdit](const QPoint& pos)
    {
        int cursor = textEdit->cursorForPosition(pos).position();
        const QString& fullText = textEdit->toPlainText();

        QString dice;
        if (DiceHelper::TryGetDiceExpression(fullText, dice, cursor))
        {
            QPoint globalPosition = textEdit->mapToGlobal(pos);

            QMenu menu;
            menu.addAction(tr("Set custom dice"), this, [dice, this]()
            {
                emit DiceExpressionSelected(dice, true);
            });

            if (m_DiceExpressionReceivers != nullptr)
            {
                for (const QString& receiver : *m_DiceExpressionReceivers)
                {
                    menu.addSeparator();

                    QString sendMenuName = "Set " + receiver;
                    menu.addAction(sendMenuName, this, [receiver, dice, this]()
                    {
                        emit DiceExpressionSentToReceiver(receiver, dice, DiceOperation::Set);
                    });

                    QString addMenuName = "Add to " + receiver;
                    menu.addAction(addMenuName, this, [receiver, dice, this]()
                    {
                        emit DiceExpressionSentToReceiver(receiver, dice, DiceOperation::Add);
                    });

                    QString appendMenuName = "Append to " + receiver;
                    menu.addAction(appendMenuName, this, [receiver, dice, this]()
                    {
                        emit DiceExpressionSentToReceiver(receiver, dice, DiceOperation::Append);
                    });
                }
            }

            menu.exec(globalPosition);
        }
    });

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [this, textEdit, retrieverList](const QJsonObject& jsonData)
    {
        QString text;

        for (JSonRetriever* jsonRetriever : retrieverList)
        {
            text += jsonRetriever->ToString(jsonData, &m_ScriptResult);
        }

        textEdit->setHtml(text);
    };

    m_FeedFunctions.append(feedFunction);
}

QFrame* JsonToQtXmBuilder::GetOrCreateSubListsFrame()
{
    if (m_SubListsFrame == nullptr)
    {
        m_SubListsFrame = new QFrame(this);
        m_SubListsFrame->setLayout(new QHBoxLayout());
        layout()->addWidget(m_SubListsFrame);
    }

    return m_SubListsFrame;
}

QFrame* JsonToQtXmBuilder::GetOrCreateStatsFrame()
{
    if (m_StatsFrame == nullptr)
    {
        m_StatsFrame = new QFrame(this);
        m_StatsFrame->setLayout(new QHBoxLayout());
        layout()->addWidget(m_StatsFrame);
    }

    return m_StatsFrame;
}

void JsonToQtXmBuilder::BuildList(QXmlStreamReader& reader)
{
    const QString& listName = reader.attributes().value("ListName").toString();
    const QString& xmlBuilderName = reader.attributes().value("XmlBuilderName").toString();

    m_HasSubList = true;

    JsonToQtXmBuilder* currentListDetails = new JsonToQtXmBuilder(ui, parentWidget());
    currentListDetails->BuildFromXml(m_XmlPath, xmlBuilderName);
    currentListDetails->setMinimumWidth(0);
    currentListDetails->setMaximumSize(0, 0);
    ui->characterSpellsAndActionsDetailsContent->layout()->addWidget(currentListDetails);

    m_SubListsDetails.append(currentListDetails);

    QLabel* nameLabel = new QLabel(listName, this);
    QListWidget* list = new QListWidget(this);
    list->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    connect(list, &QListWidget::itemSelectionChanged, this, [currentListDetails, list]()
    {
        QJsonObject jsonObject;

        if (list->selectedItems().count() > 0)
        {
            QListWidgetItem* item = list->selectedItems().front();
            jsonObject = item->data(Qt::UserRole).toJsonObject();
        }

        for (JsonToQtXmBuilder* listDetails : m_SubListsDetails)
        {
            if (listDetails != currentListDetails)
            {
                listDetails->setMaximumSize(0, 0);
            }
            else
            {
                static const int maxSize = 30000;
                listDetails->setMaximumSize(maxSize, maxSize);
                listDetails->FeedFromJson(jsonObject);
            }
        }
    });

    QFrame* frame = new QFrame(this);
    frame->setStyleSheet("QFrame {border: none;}");

    QFrame* listFrame = GetOrCreateSubListsFrame();
    listFrame->layout()->addWidget(frame);

    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->setContentsMargins(0, 0, 0, 0);
    frame->setLayout(vLayout);
    vLayout->addWidget(nameLabel);
    vLayout->addWidget(list);

    const QString& itemsNamePath = reader.attributes().value("itemsNamePath").toString();

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [itemsNamePath, list, retrieverList](const QJsonObject& jsonData)
    {
        QList<QJsonObject> objectList;
        QList<QString> path;

        for (JSonRetriever* jsonRetriever : retrieverList)
        {
            QList<QJsonObject> localObjectList;
            jsonRetriever->ToObjectList(jsonData, localObjectList);

            for (int i = 0; i < localObjectList.count(); ++i)
            {
                const QString& pathFromRetriever = jsonRetriever->GetItemNamePath();
                if (pathFromRetriever.isEmpty())
                {
                    path.append(itemsNamePath);
                }
                else
                {
                    path.append(pathFromRetriever);
                }
            }

            objectList.append(localObjectList);
        }

        for (int i = list->count() - 1; i >= 0; --i)
        {
             delete list->takeItem(i);
        }

        list->clear();

        for (int i = 0; i < objectList.count(); ++i)
        {
            const QJsonObject& object = objectList[i];
            const QString& currentItemNamePath = path[i];
            JSonStringRetriever nameRetriever("", currentItemNamePath, "");
            const QString& name = nameRetriever.ToString(object);

            QListWidgetItem* item = new QListWidgetItem();
            item->setData(Qt::DisplayRole, name);
            item->setData(Qt::UserRole, object);
            list->addItem(item);
        }
    };

    m_FeedFunctions.append(feedFunction);
}

void JsonToQtXmBuilder::RegisterScript(QXmlStreamReader& reader)
{
    JsonRetrieverHelper::RegisterMathScript(reader, m_JSEngine, m_ScriptResult, m_FeedScriptFunctions);
}
