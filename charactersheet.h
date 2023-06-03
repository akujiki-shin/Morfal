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

#include <QObject>

#include "dicehelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPropertyAnimation;
class JsonToQtXmBuilder;

class CharacterSheet : public QObject
{
private:
    Q_OBJECT

    using super = QObject;

public:
    explicit CharacterSheet(Ui::MainWindow* mainWindowUi, const std::map<QString, QString>& playerSettingPaths, const std::map<QString, QString>& monsterSettingPaths, QObject *parent = nullptr);

    void FeedMonsterFromJson(const QJsonObject& jsonData, const QString& category);
    void FeedMonsterFromJson(const QString& jsonString, const QString& category);

    void ClearMonster();

    void RegisterDiceExpressionReceiver(const QString& receiverName);

signals:
    void DiceExpressionSelected(const QString& diceExpression, bool replaceCustomDice);
    void DiceExpressionSentToReceiver(const QString& receiverName, const QString& diceExpression, DiceOperation diceOperation);

private:
    void BuildFromPaths(const std::map<QString, QString>& paths);
    void HideAllSheets();
    void HideSheet(const QString& type);
    void ShowSheet(const QString& type);
    void HideSheetPart(QWidget* widget);
    void ShowSheetPart(QWidget* widget);
    void ConnectDice(JsonToQtXmBuilder* builder);

private:
    std::map<QString, QList<QWidget*>> m_SheetPartsToShow;
    std::map<QString, QList<QWidget*>> m_SheetPartsToHide;

    Ui::MainWindow* ui { nullptr };

    QStringList m_DiceExpressionReceivers;

    std::map<QString, JsonToQtXmBuilder*> m_Sheets;
    QString m_CurrentMonsterCategory{};
};
