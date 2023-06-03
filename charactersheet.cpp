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

#include "charactersheet.h"

#include "ui_mainwindow.h"

#include <QJsonDocument>
#include <QPropertyAnimation>

#include "minimalscopeprofiler.h"
#include "jsontoqtxmbuilder.h"

CharacterSheet::CharacterSheet(Ui::MainWindow* mainWindowUi,
                               const std::map<QString, QString>& playerSettingPaths,
                               const std::map<QString, QString>& monsterSettingPaths,
                               QObject *parent)
    : super(parent)
    , ui(mainWindowUi)
{
    profileName("Build character sheet from xml");

    BuildFromPaths(monsterSettingPaths);
    BuildFromPaths(playerSettingPaths);

    HideAllSheets();

    if (playerSettingPaths.size() > 0)
    {
        const QString& firstCategory = playerSettingPaths.begin()->first;
        ShowSheet(firstCategory);
    }
}

void CharacterSheet::BuildFromPaths(const std::map<QString, QString>& paths)
{
    for (const auto& [dataFileName, path] : paths)
    {
        JsonToQtXmBuilder*& sheet = m_Sheets[dataFileName];
        m_CurrentMonsterCategory = dataFileName;

        sheet = new JsonToQtXmBuilder(ui, ui->characterSheetFrame);

        ui->characterSheetScrollAreaContent->layout()->addWidget(sheet);

        sheet->BuildFromXml(path, "main");

        sheet->setMinimumWidth(0);

        QList<QWidget*>& monsterPartsToHide = m_SheetPartsToHide[dataFileName];
        monsterPartsToHide.append(sheet);

        QList<QWidget*>& monsterPartsToShow = m_SheetPartsToShow[dataFileName];
        monsterPartsToShow.append(sheet);

        ConnectDice(sheet);
    }

    for (JsonToQtXmBuilder* subListDetails : JsonToQtXmBuilder::m_SubListsDetails)
    {
        ConnectDice(subListDetails);
    }
}

void CharacterSheet::ConnectDice(JsonToQtXmBuilder* builder)
{
    builder->SetDiceReveivers(&m_DiceExpressionReceivers);

    connect(builder, &JsonToQtXmBuilder::DiceExpressionSelected,
            this, [this](const QString& diceExpression, bool replaceCustomDice)
    {
        emit DiceExpressionSelected(diceExpression, replaceCustomDice);
    });

    connect(builder, &JsonToQtXmBuilder::DiceExpressionSentToReceiver,
            this, [this](const QString& receiverName, const QString& dice, DiceOperation diceOperation)
    {
        emit DiceExpressionSentToReceiver(receiverName, dice, diceOperation);
    });
}

void CharacterSheet::FeedMonsterFromJson(const QJsonObject& jsonData, const QString& category)
{
    profile();

    HideAllSheets();
    ShowSheet(category);

    auto sheetIterator = m_Sheets.find(category);
    bool foundCategory = (sheetIterator != m_Sheets.end());
    JsonToQtXmBuilder* sheet = foundCategory ? sheetIterator->second : m_Sheets.begin()->second;
    sheet->FeedFromJson(jsonData);
    m_CurrentMonsterCategory = category;

    if (sheet->HasSubList())
    {
        static const int maxSize = 30000;
        ui->characterSpellsAndActionsDetails->setMaximumHeight(maxSize);
    }
    else
    {
        ui->characterSpellsAndActionsDetails->setMaximumHeight(0);
    }
}

void CharacterSheet::FeedMonsterFromJson(const QString& jsonString, const QString& category)
{
    QJsonDocument jsonData = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!jsonData.isNull() && jsonData.isObject())
    {
        FeedMonsterFromJson(jsonData.object(), category);
    }
}

void CharacterSheet::ClearMonster()
{
    QJsonObject empty;
    auto iterator = m_Sheets.find(m_CurrentMonsterCategory);
    if (iterator != m_Sheets.end())
    {
        iterator->second->FeedFromJson(empty);
    }
}

void CharacterSheet::RegisterDiceExpressionReceiver(const QString& receiverName)
{
    if (!m_DiceExpressionReceivers.contains(receiverName))
    {
        m_DiceExpressionReceivers.append(receiverName);
    }
}

void CharacterSheet::HideSheetPart(QWidget* widget)
{
    widget->setMaximumSize(0, 0);
}

void CharacterSheet::ShowSheetPart(QWidget* widget)
{
    static const int maxSize = 30000;
    widget->setMaximumSize(maxSize, maxSize);
}

void CharacterSheet::HideAllSheets()
{
    for (auto& [type, parts] : m_SheetPartsToHide)
    {
        for (QWidget* part : parts)
        {
            HideSheetPart(part);
        }
    }
}

void CharacterSheet::HideSheet(const QString& type)
{
    QList<QWidget*>& parts = m_SheetPartsToHide[type];
    for (QWidget* part : parts)
    {
        HideSheetPart(part);
    }
}

void CharacterSheet::ShowSheet(const QString& type)
{
    QList<QWidget*>& parts = m_SheetPartsToShow[type];
    for (QWidget* part : parts)
    {
        ShowSheetPart(part);
    }
}
