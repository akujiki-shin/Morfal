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

#include <QWidget>
#include <QJSEngine>
#include <QStyleOption>

#include "JsonRetrievers/JsonRetriever.h"
#include "JsonRetrievers/JsonRetrieverHelper.h"

#include "dicehelper.h"

class QString;
class QJsonObject;
class QXmlStreamReader;
class QJSValue;
class QListWidget;

class QFrame;
class JSonIntRetriever;
class JSonFloatRetriever;
class JSonStringRetriever;
class JSonMapRetriever;
class JSonArrayRetriever;
class JSonConditionRetriever;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class JsonToQtXmBuilder : public QWidget
{
private:
    Q_OBJECT

    using super = QWidget;

    using FeedFunction = std::function<void(const QJsonObject& jsonData)>;
    using SignalFunction = void(JsonToQtXmBuilder::*)(const QJsonObject& jsonData);

public:
    enum class XmlTypes : short
    {
        title,
        label,
        stat,
        separatorLine,
        textArea,
        MathScript,
        list
    };

    Q_ENUM(XmlTypes)

signals:
    void DiceExpressionSelected(const QString& diceExpression, bool replaceCustomDice);
    void DiceExpressionSentToReceiver(const QString& receiverName, const QString& diceExpression, DiceOperation diceOperation);

public:
    explicit JsonToQtXmBuilder(Ui::MainWindow* mainWindowUi, QWidget *parent = nullptr);

    bool BuildFromXml(const QString& xmlPath, const QString& xmlFileName);
    void FeedFromJson(const QJsonObject& jsonData);

    void SetDiceReveivers(const QStringList* diceExpressionReceivers);

    inline bool HasSubList() const { return m_HasSubList; }

    static QList<JsonToQtXmBuilder*> m_SubListsDetails;

private:
    bool ReadXml(QXmlStreamReader& reader);
    void BuildTitle(QXmlStreamReader& reader);
    void BuildLabel(QXmlStreamReader& reader);
    void BuildStat(QXmlStreamReader& reader);
    void BuildSeparatorLine(QXmlStreamReader& reader);
    void BuildTextArea(QXmlStreamReader& reader);

    void BuildList(QXmlStreamReader& reader);

    void RegisterScript(QXmlStreamReader& reader);

    void AlignNameLabel();

    QListWidget* GetOrCreateActionAndItems();

    QFrame* GetOrCreateSubListsFrame();
    QFrame* GetOrCreateStatsFrame();

private:
    QList<FeedFunction> m_FeedFunctions;
    QList<FeedFunction> m_FeedScriptFunctions;

    QMap<QString, QJSValue> m_ScriptResult;
    QJSEngine m_JSEngine;

    int m_MaxNameLabelWidth = 0;
    QList<QLabel*> m_NameLabelList;

    QFrame* m_SubListsFrame { nullptr };
    QFrame* m_StatsFrame { nullptr };
    const QStringList* m_DiceExpressionReceivers { nullptr };

    QString m_XmlPath;
    bool m_HasSubList { false };
    Ui::MainWindow* ui { nullptr };
};
