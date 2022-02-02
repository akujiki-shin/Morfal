#include "charactersheet.h"

#include "ui_mainwindow.h"

#include <QJsonDocument>
#include <QPropertyAnimation>

#include "minimalscopeprofiler.h"

CharacterSheet::CharacterSheet(Ui::MainWindow* mainWindowUi, const QString& playerSettingPath, const std::map<QString, QString>& monsterSettingPaths, QObject *parent)
    : super(parent)
    , ui(mainWindowUi)
    , m_PlayerSettingPath(playerSettingPath)
{
    QList<QWidget*>& playerPartsToHide = m_SheetPartsToHide["player"];
    playerPartsToHide.append(ui->playerJsonToQtXmBuilder);
    playerPartsToHide.append(ui->spellsPlayerJsonToQtXmBuilder);
    playerPartsToHide.append(ui->itemsAndActionsPlayerJsonToQtXmBuilder);

    QList<QWidget*>& playerPartsToShow = m_SheetPartsToShow["player"];
    playerPartsToShow.append(ui->playerJsonToQtXmBuilder);
    playerPartsToShow.append(ui->itemsAndActionsPlayerJsonToQtXmBuilder);

    {
        profileName("Build character sheet from xml");

        for (const auto& [dataFileName, path] : monsterSettingPaths)
        {
            MonsterSheet& sheet = m_MonsterSheets[dataFileName];
            m_CurrentMonsterCategory = dataFileName;

            sheet.m_MainInformation = new JsonToQtXmBuilder(ui->characterSheetFrame);
            sheet.m_Spells = new JsonToQtXmBuilder(ui->characterSheetFrame);
            sheet.m_ItemAndActions = new JsonToQtXmBuilder(ui->characterSheetFrame);

            ui->characterSheetScrollAreaContent->layout()->addWidget(sheet.m_MainInformation);
            ui->characterSpellsAndActionsDetailsContent->layout()->addWidget(sheet.m_Spells);
            ui->characterSpellsAndActionsDetailsContent->layout()->addWidget(sheet.m_ItemAndActions);

            sheet.m_MainInformation->BuildFromXml(path + "/monsterDetails.xml");
            sheet.m_Spells->BuildFromXml(path + "/spellsDetails.xml");
            sheet.m_ItemAndActions->BuildFromXml(path + "/itemsAndActionsDetails.xml");

            sheet.m_MainInformation->setMinimumWidth(0);
            sheet.m_Spells->setMinimumWidth(0);
            sheet.m_ItemAndActions->setMinimumWidth(0);

            QList<QWidget*>& monsterPartsToHide = m_SheetPartsToHide[dataFileName];
            monsterPartsToHide.append(sheet.m_MainInformation);
            monsterPartsToHide.append(sheet.m_Spells);
            monsterPartsToHide.append(sheet.m_ItemAndActions);

            QList<QWidget*>& monsterPartsToShow = m_SheetPartsToShow[dataFileName];
            monsterPartsToShow.append(sheet.m_MainInformation);
            monsterPartsToShow.append(sheet.m_ItemAndActions);

            connect(sheet.m_MainInformation, &JsonToQtXmBuilder::SpellSelectionChanged,
                    this, [this, spells = sheet.m_Spells, itemsAndActions = sheet.m_ItemAndActions](const QJsonObject& object)
            {
                profileName("Show spell sheet part")
                ShowSheetPart(spells);
                spells->FeedFromJson(object);
                HideSheetPart(itemsAndActions);
            });

            connect(sheet.m_MainInformation, &JsonToQtXmBuilder::ItemAndActionSelectionChanged,
                    this, [this, spells = sheet.m_Spells, itemsAndActions = sheet.m_ItemAndActions](const QJsonObject& object)
            {
                profileName("Show items & actions sheet part")
                ShowSheetPart(itemsAndActions);
                itemsAndActions->FeedFromJson(object);
                HideSheetPart(spells);
            });

            ConnectDice(sheet.m_MainInformation);
            ConnectDice(sheet.m_Spells);
            ConnectDice(sheet.m_ItemAndActions);
        }

        ui->playerJsonToQtXmBuilder->BuildFromXml(m_PlayerSettingPath + "/playerDetails.xml");
        ui->spellsPlayerJsonToQtXmBuilder->BuildFromXml(m_PlayerSettingPath + "/spellsPlayerDetails.xml");
        ui->itemsAndActionsPlayerJsonToQtXmBuilder->BuildFromXml(m_PlayerSettingPath + "/itemsAndActionsPlayerDetails.xml");
    }

    ui->playerJsonToQtXmBuilder->setMinimumWidth(0);
    ui->spellsPlayerJsonToQtXmBuilder->setMinimumWidth(0);
    ui->itemsAndActionsPlayerJsonToQtXmBuilder->setMinimumWidth(0);

    ConnectDice(ui->playerJsonToQtXmBuilder);
    ConnectDice(ui->spellsPlayerJsonToQtXmBuilder);
    ConnectDice(ui->itemsAndActionsPlayerJsonToQtXmBuilder);

    connect(ui->playerJsonToQtXmBuilder, &JsonToQtXmBuilder::SpellSelectionChanged,
            this, [this](const QJsonObject& object)
    {
        profileName("Show spell sheet part")
        ShowSheetPart(ui->spellsPlayerJsonToQtXmBuilder);
        ui->spellsPlayerJsonToQtXmBuilder->FeedFromJson(object);
        HideSheetPart(ui->itemsAndActionsPlayerJsonToQtXmBuilder);
    });

    connect(ui->playerJsonToQtXmBuilder, &JsonToQtXmBuilder::ItemAndActionSelectionChanged,
            this, [this](const QJsonObject& object)
    {

        profileName("Show items & actions sheet part")
        ShowSheetPart(ui->itemsAndActionsPlayerJsonToQtXmBuilder);
        ui->itemsAndActionsPlayerJsonToQtXmBuilder->FeedFromJson(object);
        HideSheetPart(ui->spellsPlayerJsonToQtXmBuilder);
    });

    HideAllSheets();
    ShowSheet("player");
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

    auto sheetIterator = m_MonsterSheets.find(category);
    bool foundCategory = (sheetIterator != m_MonsterSheets.end());
    MonsterSheet& sheet = foundCategory ? sheetIterator->second : m_MonsterSheets.begin()->second;
    sheet.m_MainInformation->FeedFromJson(jsonData);
    m_CurrentMonsterCategory = category;

    if (sheet.m_Spells->layout()->isEmpty() && sheet.m_ItemAndActions->layout()->isEmpty())
    {
        ui->characterSpellsAndActionsDetails->setMaximumHeight(0);
    }
    else
    {
        static const int maxSize = 30000;
        ui->characterSpellsAndActionsDetails->setMaximumHeight(maxSize);
    }
}

void CharacterSheet::FeedPlayerFromJson(const QJsonObject& jsonData)
{
    profile();

    HideAllSheets();
    ShowSheet("player");

    ui->playerJsonToQtXmBuilder->FeedFromJson(jsonData);

    if (ui->spellsPlayerJsonToQtXmBuilder->layout()->isEmpty() && ui->itemsAndActionsPlayerJsonToQtXmBuilder->layout()->isEmpty())
    {
        ui->characterSpellsAndActionsDetails->setMaximumHeight(0);
    }
    else
    {
        static const int maxSize = 30000;
        ui->characterSpellsAndActionsDetails->setMaximumHeight(maxSize);
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

void CharacterSheet::FeedPlayerFromJson(const QString& jsonString)
{
    QJsonDocument jsonData = QJsonDocument::fromJson(jsonString.toUtf8());
    if (!jsonData.isNull() && jsonData.isObject())
    {
        FeedPlayerFromJson(jsonData.object());
    }
}

void CharacterSheet::ClearMonster()
{
    QJsonObject empty;
    auto iterator = m_MonsterSheets.find(m_CurrentMonsterCategory);
    if (iterator != m_MonsterSheets.end())
    {
        iterator->second.m_MainInformation->FeedFromJson(empty);
    }
}

void CharacterSheet::ClearPlayer()
{
    QJsonObject empty;
    ui->playerJsonToQtXmBuilder->FeedFromJson(empty);
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
