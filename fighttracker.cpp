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

#include "fighttracker.h"

#include "ui_mainwindow.h"

#include <QRandomGenerator>
#include <QString>
#include <QtCore>
#include <QColor>
#include <QJsonDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegExp>
#include <QRegularExpressionValidator>
#include <QCompleter>
#include <QPushButton>
#include <QXmlStreamReader>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <algorithm>
#include <sstream>

#include "utils.h"
#include "dicehelper.h"
#include "minimalscopeprofiler.h"
#include "monsterdelegate.h"
#include "underlinedrowdelegate.h"
#include "interactivemap.h"

#include "JsonRetrievers/JsonRetrieverHelper.h"
#include "JsonRetrievers/JsonRetriever.h"

FightTracker::FightTracker(QWidget *parent)
    : super(parent)
{
}

void FightTracker::Initialize()
{
    m_Table[(int)TableType::Monster] = ui->monstersFightTable;
    m_Table[(int)TableType::Player] = ui->playersFightTable;

    //QRegularExpressionValidator* diceValidator = new QRegularExpressionValidator(DiceHelper::DiceRegExp, this);
    //ui->customDiceLineEdit->setValidator(diceValidator);

    QHBoxLayout* diceLayout = new QHBoxLayout();
    diceLayout->setContentsMargins(0, 0, 0, 0);
    ui->customDiceFrame->setLayout(diceLayout);

    connect(ui->addMonsterButton, &QPushButton::pressed, this, &FightTracker::AddMonsterButtonClicked);
    connect(ui->addPlayerButton, &QPushButton::pressed, this, &FightTracker::AddPlayerButtonClicked);

    ConnectDiceButton(ui->d4Button);
    ConnectDiceButton(ui->d6Button);
    ConnectDiceButton(ui->d8Button);
    ConnectDiceButton(ui->d10Button);
    ConnectDiceButton(ui->d12Button);
    ConnectDiceButton(ui->d20Button);
    ConnectDiceButton(ui->d100Button);

    connect(ui->customDiceThrowButton, &QPushButton::pressed, this, [this]()
        {
            QString result = "";
            int total = 0;

            for (int i = 0; i < ui->diceCount->value(); ++i)
            {
                if (!result.isEmpty())
                {
                    result += " | ";
                }

                QString roll;
                int value;
                DiceHelper::RollDice(ui->customDiceLineEdit->text(), roll, value, ui->detailedThrowCheckBox->isChecked(), false);

                result += roll;
                total += value;
            }

            if (ui->diceCount->value() > 1)
            {
                result = "[ " + QString::number(total) + " ]   " + result;
            }

            ui->diceResultLabel->setPlainText(result);
        });

    connect(m_CharacterSheet, &CharacterSheet::DiceExpressionSelected,
            this, [this](const QString& diceExpression, bool replaceCustomDice)
    {
        if (replaceCustomDice)
        {
            ui->customDiceLineEdit->setText(diceExpression);
            emit ui->customDiceThrowButton->pressed();
        }
        else
        {
            QString roll;
            int value;
            DiceHelper::RollDice(diceExpression, roll, value, ui->detailedThrowCheckBox->isChecked());

            ui->diceResultLabel->setPlainText(roll);
        }
    });

    connect(m_CharacterSheet, &CharacterSheet::DiceExpressionSentToReceiver,
            this, [this](const QString& receiverName, const QString& dice, DiceOperation diceOperation)
    {
        SetDiceColumnByFighterName(ui->monstersFightTable, m_UserSelectedFighterName, receiverName, dice, diceOperation);
        SetDiceColumnByFighterName(ui->playersFightTable, m_UserSelectedFighterName, receiverName, dice, diceOperation);
    });

    connect(ui->reinitFightButton, &QPushButton::pressed, this, &FightTracker::ReinitFightButtonClicked);
    connect(ui->restButton, &QPushButton::pressed, this, &FightTracker::RestButtonClicked);
    connect(ui->sortFightButton, &QPushButton::pressed, this, &FightTracker::SortFightButtonClicked);
    connect(ui->nextTurnButton, &QPushButton::pressed, this, &FightTracker::NextTurnButtonClicked);

    connect(ui->clearAllAlterationsButton, &QPushButton::pressed, this, [this]()
        {
            ClearAllAlterations();
        });

    connect(ui->loadPlayersButton, &QPushButton::pressed, this, &FightTracker::LoadPlayersButtonClicked);

    connect(ui->trackerSaveMonsterButton, &QPushButton::pressed, this, &FightTracker::SaveMonsterButtonClicked);
    connect(ui->trackerLoadMonsterButton, &QPushButton::pressed, this, &FightTracker::LoadMonsterButtonClicked);

    connect(ui->rollPlayerInitiativeButton, &QPushButton::pressed, this, [this]()
        {
            for (const QString& sheetType : m_PlayerSheetType)
            {
                RollInitiative(ui->playersFightTable, TableType::Player, sheetType);
            }
        });

    connect(ui->monstersFightTable, &QTableWidget::cellChanged, this, [this](int row, int column)
        {
           if (m_IsFeedingTableItemStack == 0 && column == (int)Columns::Initiative)
           {
               QTableWidgetItem* item = ui->monstersFightTable->item(row, column);
               if (item != nullptr && !m_IsUpdatingEnabledState)
               {
                   SortFighters();
               }
           }
        });

    connect(ui->playersFightTable, &QTableWidget::cellChanged, this, [this](int row, int column)
        {
           if (m_IsFeedingTableItemStack == 0 && column == (int)Columns::Initiative)
           {
               QTableWidgetItem* item = ui->monstersFightTable->item(row, column);
               if (item != nullptr && !m_IsUpdatingEnabledState)
               {
                   SortFighters();
               }
           }
        });

    ui->playersFightTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->playersFightTable, &QTableWidget::customContextMenuRequested,
            this, [this](const QPoint &pos)
    {
        if (m_CurrentAlterationItems.empty() && !m_HealthWidget->isVisible())
        {
            QPoint globalPosition = ui->playersFightTable->mapToGlobal(pos);

            QMenu menu;

            menu.addAction(tr("Remove"), this, [this]()
            {
                RemoveSelectedRow(ui->playersFightTable);
            });

            if (IsAnyDiceColumnSelect(TableType::Monster) || IsAnyDiceColumnSelect(TableType::Player))
            {
                menu.addAction(tr("Set dice"), this, [this]()
                {
                    SetSelectedDiceColumnByFighterName(TableType::Monster);
                    SetSelectedDiceColumnByFighterName(TableType::Player);
                });
            }

            menu.exec(globalPosition);
        }
    });

    ui->monstersFightTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->monstersFightTable, &QTableWidget::customContextMenuRequested,
            this, [this](const QPoint &pos)
    {
        if (m_CurrentAlterationItems.empty() && !m_HealthWidget->isVisible())
        {
            QPoint globalPosition = ui->monstersFightTable->mapToGlobal(pos);

            QMenu menu;

            menu.addAction(tr("Remove"), this, [this]()
            {
                RemoveSelectedRow(ui->monstersFightTable);
            });

            menu.addAction(tr("Duplicate"), this, [this]()
            {
                DuplicateSelectedRow(ui->monstersFightTable);
            });

            if (IsAnyDiceColumnSelect(TableType::Monster) || IsAnyDiceColumnSelect(TableType::Player))
            {
                menu.addAction(tr("Set dice"), this, [this]()
                {
                    SetSelectedDiceColumnByFighterName(TableType::Monster);
                    SetSelectedDiceColumnByFighterName(TableType::Player);
                });
            }

            menu.exec(globalPosition);
        }
    });

    QString stylesheetPath;
    bool isUsingDefaultStyle = (Utils::TryGetFirstFilePathIn("settings", ".qss", stylesheetPath) == false);
    if (isUsingDefaultStyle)
    {
        ui->playersFightTable->setStyleSheet("QTableWidget::item{ selection-background-color:#93b6f5} QTableWidget::item:disabled {background-color:#000000;}");
        ui->monstersFightTable->setStyleSheet("QTableWidget::item{ selection-background-color:#93b6f5} QTableWidget::item:disabled {background-color:#000000;}");
    }

    SetRound(1);

    m_AlterationWidget = new QWidget(this);
    m_AlterationWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    QVBoxLayout* alterationLayout = new QVBoxLayout();
    m_AlterationWidget->setLayout(alterationLayout);

    QPushButton* doneButton = new QPushButton(this);
    doneButton->setText("Done");
    alterationLayout->addWidget(doneButton);

    QPushButton* clearAlteartionButton = new QPushButton(this);
    clearAlteartionButton->setText("Clear");
    alterationLayout->addWidget(clearAlteartionButton);

    connect(doneButton, &QPushButton::pressed, this, &FightTracker::ValidateAlterationWidget);
    connect(clearAlteartionButton, &QPushButton::pressed, this, &FightTracker::ClearAlterationWidget);

    m_HealthWidget = new QWidget(this);
    m_HealthWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    QVBoxLayout* healthLayout = new QVBoxLayout();
    m_HealthWidget->setLayout(healthLayout);

    QPushButton* closeHealthButton = new QPushButton(this);
    closeHealthButton->setText(tr("Close"));
    healthLayout->addWidget(closeHealthButton);

    m_HealthSpinBox = new QSpinBox(this);
    m_HealthSpinBox->setValue(1);
    m_HealthSpinBox->setMinimum(0);
    m_HealthSpinBox->setMaximum(10000);
    healthLayout->addWidget(m_HealthSpinBox);

    QPushButton* damageButton = new QPushButton(this);
    damageButton->setText(tr("Damage"));
    healthLayout->addWidget(damageButton);

    QPushButton* healButton = new QPushButton(this);
    healButton->setText(tr("Heal"));
    healthLayout->addWidget(healButton);

    QPushButton* setHealhButton = new QPushButton(this);
    setHealhButton->setText(tr("Set"));
    healthLayout->addWidget(setHealhButton);

    connect(damageButton, &QPushButton::pressed, this, &FightTracker::DamageButtonClicked);
    connect(healButton, &QPushButton::pressed, this, &FightTracker::HealButtonClicked);
    connect(setHealhButton, &QPushButton::pressed, this, &FightTracker::SetHealhButtonClicked);
    connect(closeHealthButton, &QPushButton::pressed, this, &FightTracker::CloseHealhButtonClicked);

    connect(ui->wantedChallengeRating, &QSpinBox::valueChanged,
            this, [this](int /* newValue */)
    {
        UpdateTabName();
    });

    connect(ui->playersFightTable, &QTableWidget::cellPressed,
            this, [this](int row, int column)
    {
        if (column == ToIndex(ColumnsFromEnd::Alteration, TableType::Player))
        {
            ClickOnAlteration(ui->playersFightTable, row, column);
        }
        else if (ui->playersFightTable->horizontalHeaderItem(column)->text().toLower() == "hp")
        {
            ClickOnHealth(ui->playersFightTable, row, column);
        }
    });

    connect(ui->monstersFightTable, &QTableWidget::cellPressed,
            this, [this](int row, int column)
    {
        if (column == ToIndex(ColumnsFromEnd::Alteration, TableType::Monster))
        {
            ClickOnAlteration(ui->monstersFightTable, row, column);
        }
        else if (ui->monstersFightTable->horizontalHeaderItem(column)->text().toLower() == "hp")
        {
            ClickOnHealth(ui->monstersFightTable, row, column);
        }
    });

    connect(ui->monstersFightTable, &QTableWidget::itemSelectionChanged, this, &FightTracker::MonsterSelectionChanged);
    connect(ui->playersFightTable, &QTableWidget::itemSelectionChanged, this, &FightTracker::PlayerSelectionChanged);

    ReadXmlSettings();
    ReadXmlScipts();

    AddColumnFromXml(TableType::Player, *m_PlayerSettingPath);
    AddColumnFromXml(TableType::Player, *m_MonsterSettingPath);
    AddColumnFromXml(TableType::Monster, *m_MonsterSettingPath);

    if (!m_LastLoadedPlayersPath.isEmpty())
    {
        for (const QString& path : m_LastLoadedPlayersPath)
        {
            LoadPlayers(path);
        }
    }
    else
    {
        LoadPlayers();
    }

    for (int i = 0; i < ui->monstersFightTable->columnCount(); ++i)
    {
        if (ui->monstersFightTable->horizontalHeaderItem(i)->text().toLower() == "name")
        {
            m_MonsterDelegate = new MonsterDelegate(i, m_InfoData, m_InfoDataItemList, this);
            ui->monstersFightTable->setItemDelegate(m_MonsterDelegate);
            break;
        }
    }

    m_PlayerDelegate = new UnderlinedRowDelegate(this);
    ui->playersFightTable->setItemDelegate(m_PlayerDelegate);

    connect(ui->GenerateEncounterButton, &QPushButton::clicked, this, &FightTracker::GenerateEncounterButtonClicked);

    connect(ui->tabs, &QTabWidget::currentChanged,
            this, [this](int index)
    {
        if (index == 1)
        {
            UpdateEncounterComboBox();
        }

        CloseHealhButtonClicked();
        ValidateAlterationWidget();
    });

    m_AlreadyFoughtBrush.setColor(QColorConstants::Black);
    m_CurrentFighterBrush.setColor(QColorConstants::Green);

    ui->monstersFightTable->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    ui->playersFightTable->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
}

void FightTracker::InitializeFilters()
{
    const auto& data = ui->jsonDataList->GetData();

    ui->encounterDataFilterComboBox->addItem("None");

    for (const auto& [categoryName, categoryData] : data)
    {
        ui->encounterDataFilterComboBox->addItem(categoryName);
    }

    UpdateEncounterComboBox();
}

void FightTracker::SetFiltersCurrentText(const QString& dataFilterText, const QString& zoneFilterText)
{
    ui->encounterDataFilterComboBox->setCurrentText(dataFilterText);
    ui->encounterZoneFilterComboBox->setCurrentText(zoneFilterText);
}

void FightTracker::UpdateEncounterComboBox()
{
    const QString& currentText = ui->encounterZoneFilterComboBox->currentText();

    ui->encounterZoneFilterComboBox->clear();

    std::vector<QString> zoneNames;
    m_InteractiveMap->GetZoneNames(zoneNames);

    ui->encounterZoneFilterComboBox->addItem("None");

    for (const QString& name : zoneNames)
    {
        ui->encounterZoneFilterComboBox->addItem(name);
    }

    ui->encounterZoneFilterComboBox->setCurrentText(currentText);
}

void FightTracker::SetInteractiveMap(InteractiveMap* interactiveMap)
{
    m_InteractiveMap = interactiveMap;
}

int FightTracker::ToIndex(ColumnsFromEnd column, TableType tableType)
{
    return m_Table[(int)tableType]->columnCount() - 1 - (int)column;
}

void FightTracker::ReadXmlSettings()
{
    QString xmlPath = m_FightTrackerSettingPath + "/FightTracker.xml";

    QFile file(xmlPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QXmlStreamReader reader(&file);
        while (reader.readNext() && !reader.isEndDocument())
        {
            if (!reader.isEndElement())
            {
                const QString& elementName = reader.name().toString();
                if (elementName == "customDice")
                {
                    const QString& name = reader.attributes().value("Name").toString();

                    QPushButton* button = new QPushButton(name, this);
                    ui->customDiceFrame->layout()->addWidget(button);
                    ConnectDiceButton(button);
                }
                else if (elementName == "alteration")
                {
                    const QString& name = reader.attributes().value("Name").toString();
                    QString description = reader.attributes().value("Description").toString();
                    QStringList sentences = description.split(".");
                    description = "";

                    for (const QString& sentence : sentences)
                    {
                        if (!description.isEmpty())
                        {
                            description += "\n";
                        }

                        description += sentence;
                    }

                    QFrame* alterationFrame = new QFrame(m_AlterationWidget);
                    QHBoxLayout* alterationlLayout = new QHBoxLayout(alterationFrame);
                    alterationlLayout->setSpacing(0);

                    QCheckBox* checkBox = new QCheckBox(name, this);
                    checkBox->setToolTipDuration(10000);
                    checkBox->setToolTip(description);

                    QLabel* label = new QLabel(" time limit ", this);
                    label->setAlignment(Qt::AlignRight);

                    QSpinBox* spinBox = new QSpinBox(this);
                    spinBox->setAlignment(Qt::AlignRight);
                    spinBox->setMaximumWidth(40);

                    m_AlterationCheckBoxes.append(checkBox);
                    m_AlterationDurationSpinBoxes.append(spinBox);

                    alterationlLayout->addWidget(checkBox);
                    alterationlLayout->addWidget(label);
                    alterationlLayout->addWidget(spinBox);

                    m_AlterationWidget->layout()->addWidget(alterationFrame);
                }
                else if (elementName == "MathScript")
                {
                    JsonRetrieverHelper::RegisterMathScript(reader, m_JSEngine, m_ScriptResult, m_GlobalFeedScriptFunctions);
                }
                else if (elementName == "cr")
                {
                    m_ChallengeRatingVarName = reader.attributes().value("VarName").toString();
                }
                else if (elementName == "CumulatedFromMonsters")
                {
                    QString varName = reader.attributes().value("VarName").toString();

                    while (reader.readNext() && !reader.isEndDocument())
                    {
                        const QString& innerElementName = reader.name().toString();
                        if (reader.isEndElement() && innerElementName == "CumulatedFromMonsters")
                        {
                            break;
                        }
                        else if (!reader.isEndElement() && innerElementName == "MathScript")
                        {
                            JsonRetrieverHelper::RegisterMathScript(reader, m_JSEngine, m_ScriptResult, m_CumulatedFeedScriptFunctionsMap[varName]);
                        }
                        else if (!reader.isEndElement() && innerElementName == "Script")
                        {
                            JsonRetrieverHelper::RegisterXmlScript(reader, *m_MonsterCategoryToSetting, m_XmlScripts, m_ScriptResult, m_CumulatedFeedScriptFunctionsMap[varName]);
                        }
                    }
                }
            }
        }
    }
}

void FightTracker::ReadXmlScipts()
{
    QFileInfo settingsDir;
    if (Utils::TryFindDir("settings", settingsDir))
    {
        QDirIterator rulesDir = Utils::GetFileInfoFromPath(settingsDir.filePath() + "/rules").dir();

        std::vector<QFileInfo> rulesSubDirectories;
        Utils::GetDirectSubDirList(rulesDir.path(), rulesSubDirectories);

        for (const QFileInfo& ruleDir : rulesSubDirectories)
        {
            QDirIterator scriptDirIterator = Utils::GetFileInfoFromPath(ruleDir.filePath() + "/scripts/.").dir();
            while (scriptDirIterator.hasNext())
            {
                scriptDirIterator.next();

                if (Utils::FileMatchesExtension(scriptDirIterator, ".xml"))
                {
                    const QString& ruleName = ruleDir.fileName();
                    const QString& scriptName =  Utils::CleanFileName(scriptDirIterator.fileName());
                    m_XmlScripts[scriptName][ruleName].Initialize(scriptDirIterator, &m_JSEngine);
                }
            }
        }
    }
}

void FightTracker::AddColumnFromXml(TableType tableType, const std::map<QString, QString>& settingPaths)
{
    QTableWidget* table;
    QPushButton* initiativeButton;

    if (tableType == TableType::Monster)
    {
        table = ui->monstersFightTable;
        initiativeButton = ui->rollMonstersInitiativeButton;
    }
    else
    {
        table = ui->playersFightTable;
        initiativeButton = ui->rollPlayerInitiativeButton;
    }

    for (const auto& [category, path] : settingPaths)
    {
        const QString& sheetType = category;

        if (tableType == TableType::Monster)
        {
            connect(ui->rollMonstersInitiativeButton, &QPushButton::pressed, this, [this, table, tableType, sheetType]()
                {
                   RollInitiative(table, tableType, sheetType);
                });
        }

        AddColumnFromXml(table, tableType, sheetType, path + "/fightTrackerColumns.xml");
    }
}

void FightTracker::AddColumnFromXml(QTableWidget* table, TableType tableType, const QString& sheetType, const QString& xml)
{
    QFile file(xml);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QXmlStreamReader reader(&file);
        ReadXml(reader, table, tableType, sheetType);
    }
}

void FightTracker::ReadXml(QXmlStreamReader& reader, QTableWidget* table, TableType tableType, const QString& sheetType)
{
    profileName("Build fight tracker columns from xml");

    while (reader.readNext() && !reader.isEndDocument())
    {
        if (!reader.isEndElement())
        {
            const QString& elementName = reader.name().toString();
            if (elementName == "tableColumn")
            {
                bool needToCrreeateColumn = true;
                int columnIndex = table->columnCount() - (int)ColumnsFromEnd::Count;
                const QString& name = reader.attributes().value("Name").toString();
                const QString& diceResultColumnName = reader.attributes().value("DiceResultColumn").toString();
                const bool isDiceReceiver = reader.attributes().value("IsDiceReceiver").toString() == "true";
                const bool isDiceColumn = !diceResultColumnName.isEmpty();

                for (int i = 0; i < table->columnCount(); i++)
                {
                    const QString& currentColumnName = table->model()->headerData(i, Qt::Horizontal).toString();
                    if (currentColumnName == name)
                    {
                        columnIndex = i;
                        needToCrreeateColumn = false;
                        break;
                    }
                }

                const bool editable = (reader.attributes().value("Editable").toString() == "true");

                if (needToCrreeateColumn)
                {
                    QTableWidgetItem* item = new QTableWidgetItem(name);
                    table->insertColumn(columnIndex);
                    table->setHorizontalHeaderItem(columnIndex, item);

                    if (isDiceColumn)
                    {
                        m_DiceColumnIndexes[(int)tableType].append({columnIndex, columnIndex + 1});

                        m_DiceColumnNameMap[(int)tableType][columnIndex] = name;

                        if (isDiceReceiver)
                        {
                            m_CharacterSheet->RegisterDiceExpressionReceiver(name);
                        }

                        QHeaderView* header = table->horizontalHeader();
                        connect(header, &QHeaderView::sectionPressed, this, [this, diceColumn = columnIndex, tableType](int column)
                            {
                               if (column == diceColumn)
                               {
                                   int rowCount = m_Table[(int)tableType]->rowCount();
                                   for (int i = 0; i < rowCount; ++i)
                                   {
                                       RollDiceColumn(tableType, i);
                                   }
                               }
                            });

                        item = new QTableWidgetItem(diceResultColumnName);
                        table->insertColumn(columnIndex+1);
                        table->setHorizontalHeaderItem(columnIndex+1, item);

                        connect(table, &QTableWidget::cellPressed, this, [this, diceColumn = columnIndex, tableType](int row, int column)
                            {
                               if (QApplication::mouseButtons() & Qt::MiddleButton
                                && column == diceColumn)
                               {
                                   RollDiceColumn(tableType, row);
                               }
                            });
                    }
                }

                if (name.toLower() == "hp")
                {
                    m_HPColumnIndex[(int)tableType] = columnIndex;
                }
                else if (name.toLower() == "hp max")
                {
                    m_HPMaxColumnIndex[(int)tableType] = columnIndex;
                }

                QList<JSonRetriever*> retrieverList;
                JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

                FeedRowFunction feedFunction = [this, columnIndex, retrieverList, editable](QTableWidget* table, int row, const QJsonObject& jsonData)
                {
                    QString result;

                    for (JSonRetriever* jsonRetriever : retrieverList)
                    {
                        result += jsonRetriever->ToString(jsonData, &m_ScriptResult);
                    }

                    if (QTableWidgetItem* item = table->item(row, columnIndex))
                    {
                        QRegExp isNumberRegExp("\\d*");

                        if (isNumberRegExp.exactMatch(result))
                        {
                            item->setData(Qt::DisplayRole, result.toInt());
                        }
                        else
                        {
                            item->setData(Qt::DisplayRole, result);
                        }

                        item->setData(Qt::DisplayRole, result);

                        if (!editable)
                        {
                            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                        }
                    }
                };

                m_FeedFunctions[sheetType].append(feedFunction);
            }
            else if (elementName == "initiativeModifier")
            {
                QList<JSonRetriever*> retrieverList;
                JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

                m_InitModifierFeedFunction[sheetType] = [this, retrieverList](const QJsonObject& jsonData)
                {
                    m_InitModifier = 0;

                    for (JSonRetriever* jsonRetriever : retrieverList)
                    {
                        m_InitModifier += jsonRetriever->ToString(jsonData, &m_ScriptResult).toInt();
                    }
                };
            }
            else if (elementName == "MathScript")
            {
                JsonRetrieverHelper::RegisterMathScript(reader, m_JSEngine, m_ScriptResult, m_FeedScriptFunctions[sheetType]);
            }
        }
    }
}

void FightTracker::AddFromJSonData(const std::vector<JSonCategoryInfoPtr>& jsonObjects, TableType tableType)
{
    for (int i = 0; i < jsonObjects.size(); ++i)
    {
        const JSonCategoryInfoPtr& infoPtr = jsonObjects[i];
        JSonCategoryInfo info = {*infoPtr.first, *infoPtr.second};
        AddFromJSonData(info, tableType);
    }

    SortFighters();
}

void FightTracker::AddFromJSonData(const std::vector<JSonCategoryInfo>& jsonObjects, TableType tableType)
{
    for (int i = 0; i < jsonObjects.size(); ++i)
    {
        AddFromJSonData(jsonObjects[i], tableType);
    }

    SortFighters();
}

void FightTracker::AddFromJSonData(const JSonCategoryInfo& jsonData, TableType tableType)
{
    m_IsFeedingTableItemStack++;

    QTableWidget* table;

    if (tableType == TableType::Monster)
    {
        AddMonsterButtonClicked();
        table = ui->monstersFightTable;
    }
    else
    {
        AddPlayerButtonClicked();
        table = ui->playersFightTable;
    }

    int row = table->rowCount() - 1;

    const QString& category = jsonData.second;

    QTableWidgetItem* jsonHolderItem = table->item(row, ms_JSonObjectRow);
    jsonHolderItem->setData(Qt::UserRole, jsonData.first);
    jsonHolderItem->setData(Qt::UserRole+1, category);

    QList<FeedFunction>& scripts = m_FeedScriptFunctions[category];
    for (FeedFunction& script : scripts)
    {
        script(jsonData.first);
    }

    QList<FeedRowFunction>& feedList = m_FeedFunctions[category];
    for (FeedRowFunction& feed : feedList)
    {
        feed(table, row, jsonData.first);
    }

    QHeaderView* headers = table->horizontalHeader();
    headers->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    headers->setSectionResizeMode(headers->count() - 1, QHeaderView::ResizeMode::Stretch);

    m_IsFeedingTableItemStack--;
}

void FightTracker::ClearMonsters()
{
    ui->monstersFightTable->selectAll();
    RemoveSelectedRow(ui->monstersFightTable);

    m_IsFightInProgress = false;
}

void FightTracker::ClearPlayers()
{
    ui->playersFightTable->selectAll();
    RemoveSelectedRow(ui->playersFightTable);
    m_LoadedPlayersPath.clear();
    m_PlayerSheetType.clear();

    m_IsFightInProgress = false;
}

void FightTracker::GenerateEncounterFromJSonData(const std::vector<JSonCategoryInfo>& jsonObjects)
{
    profile();

    using Encounter = std::vector<JSonCategoryInfoPtr>;

    ClearMonsters();

    const int maxTries = 200;
    const int jsonCount = (int)jsonObjects.size();

    if (jsonCount == 0)
    {
        return;
    }

    Encounter encouters[maxTries];

    const int wantedChallengeRating = ui->wantedChallengeRating->value();
    float bestAbsolutDeltaChallengeRating = 1000;
    float bestDeltaChallengeRating = 1000;

    Encounter* bestEncounter = &encouters[0];

    for (int i = 0; i < maxTries; ++i)
    {
        Encounter* currentEncounter = &encouters[i];

        int randomMonsterIndex = QRandomGenerator::global()->bounded(jsonCount);
        if (randomMonsterIndex < 0)
        {
            randomMonsterIndex = 0;
        }

        const QJsonObject* randomMonster = &jsonObjects[randomMonsterIndex].first;
        const QString* randomMonsterCategory = &jsonObjects[randomMonsterIndex].second;

        if (bestDeltaChallengeRating < wantedChallengeRating)
        {
            currentEncounter->reserve(bestEncounter->size());
            for (int j = 0; j < bestEncounter->size(); ++j)
            {
                currentEncounter->push_back((*bestEncounter)[j]);
            }

            currentEncounter->push_back({randomMonster, randomMonsterCategory});
        }
        else if (!currentEncounter->empty())
        {
            currentEncounter->reserve(bestEncounter->size() - 1);
            for (int j = 0; j < bestEncounter->size() - 1; ++j)
            {
                currentEncounter->push_back((*bestEncounter)[j]);
            }

            int randomIndexToReplace = QRandomGenerator::global()->bounded(currentEncounter->size() - 1);
            (*currentEncounter)[randomIndexToReplace] = {randomMonster, randomMonsterCategory};
        }

        float currentChallengeRating = ComputeChallengeRating(*currentEncounter);

        if (currentChallengeRating == wantedChallengeRating)
        {
            bestEncounter = currentEncounter;
            break;
        }

        float currentDeltaCR = currentChallengeRating - wantedChallengeRating;
        float currentAbsolutDeltaCR = abs(currentDeltaCR);

        if (currentAbsolutDeltaCR <= bestAbsolutDeltaChallengeRating)
        {
            bestEncounter = currentEncounter;
            bestDeltaChallengeRating = currentDeltaCR;
            bestAbsolutDeltaChallengeRating = currentAbsolutDeltaCR;
        }
    }

    AddFromJSonData(*bestEncounter, TableType::Monster);
}

void FightTracker::FeedFromJSonData(const QJsonObject& jsonData, TableType tableType, const QString& sheetType, int row)
{
    m_IsFeedingTableItemStack++;

    QTableWidget* table = m_Table[(int)tableType];
    QTableWidgetItem* jsonHolderItem = table->item(row, ms_JSonObjectRow);
    jsonHolderItem->setData(Qt::UserRole, jsonData);

    QList<FeedFunction>& scripts = m_FeedScriptFunctions[sheetType];
    for (FeedFunction& script : scripts)
    {
        script(jsonData);
    }

    QList<FeedRowFunction>& feedList = m_FeedFunctions[sheetType];
    for (FeedRowFunction& feed : feedList)
    {
        feed(table, row, jsonData);
    }

    m_IsFeedingTableItemStack--;
}

void FightTracker::AddPlayerFromJSonData(const QJsonObject& jsonData, const QString& category)
{
    AddPlayerButtonClicked();
    int row = ui->playersFightTable->rowCount() - 1;

    QTableWidgetItem* jsonHolderItem = ui->playersFightTable->item(row, ms_JSonObjectRow);
    jsonHolderItem->setData(Qt::UserRole+1, category);

    FeedFromJSonData(jsonData, TableType::Player, category, row);
}

void FightTracker::SetRound(int round)
{
    m_CurrentRound = round;
    ui->roundLineEdit->setText(QString::number(m_CurrentRound));
}

void FightTracker::AddMonsterButtonClicked()
{
    m_IsFeedingTableItemStack++;

    int nextRow = ui->monstersFightTable->rowCount();
    ui->monstersFightTable->setRowCount(nextRow + 1);

    ui->monstersFightTable->setWordWrap(true);

    for (int i = 0; i < ui->monstersFightTable->columnCount(); ++i)
    {
        int nameColumn = -1;
        QTableWidgetItem* newItem = nullptr;

        if (ui->monstersFightTable->horizontalHeaderItem(i)->text().toLower() == "name")
        {
            newItem = new QTableWidgetItem("Mob " + QString::number(nextRow+1));

            nameColumn = i;
        }
        else if (i == ToIndex(ColumnsFromEnd::Alteration, TableType::Monster))
        {
            newItem = new QTableWidgetItem("");
            QList<AlterationInfo> alterationInfo;
            for (int i = 0; i < m_AlterationCheckBoxes.count(); ++i)
            {
                alterationInfo.append({Qt::Unchecked, 0});
            }

            newItem->setData(Qt::UserRole, QVariant::fromValue(alterationInfo));
        }
        else if (i == ToIndex(ColumnsFromEnd::Note, TableType::Monster) || i == ToIndex(ColumnsFromEnd::CountDown, TableType::Monster))
        {
            newItem = new QTableWidgetItem("");
        }
        else if (i == ToIndex(ColumnsFromEnd::CountDown, TableType::Monster))
        {
            newItem = new QTableWidgetItem();
            newItem->setData(Qt::DisplayRole, 0);
        }
        else if (i == 0)
        {
            newItem = new QTableWidgetItem();
            newItem->setData(Qt::DisplayRole, 10);
        }
        else
        {
            newItem = new QTableWidgetItem("");
        }

        newItem->setTextAlignment(Qt::AlignLeft | Qt::Alignment(Qt::TextWordWrap));
        ui->monstersFightTable->setItem(nextRow, i, newItem);

        if (i == (int)Columns::Initiative)
        {
            m_Fighters.append({newItem, TableType::Monster});
        }

        if (nameColumn != -1)
        {
            connect(ui->monstersFightTable, &QTableWidget::cellChanged, this,
                    [this, nameColumn](int row, int column)
            {
                if (!m_IsUpdatingEnabledState && column == nameColumn)
                {
                    if (QTableWidgetItem* item = ui->monstersFightTable->item(row, column))
                    {
                        MonsterNameChanged(item->text(), row);
                    }
                }
            });
        }
    }

    m_IsFeedingTableItemStack--;
}

void FightTracker::MonsterNameChanged(const QString& name, int row)
{
    if (m_IsFeedingTableItemStack == 0 && !m_IsDeletingTableItem && m_InfoDataItemList->contains(name))
    {
        for (const auto& [category, itemNames] : *m_InfoData)
        {
            for (const auto& [itemName, data] : itemNames)
            {
                if (name == itemName)
                {
                    QJsonDocument jsonData = QJsonDocument::fromJson(data.toUtf8());
                    if (!jsonData.isNull() && jsonData.isObject())
                    {
                        const QJsonObject& jsonObject = jsonData.object();
                        QTableWidgetItem* jsonHolderItem = ui->monstersFightTable->item(row, ms_JSonObjectRow);
                        jsonHolderItem->setData(Qt::UserRole, jsonObject);
                        jsonHolderItem->setData(Qt::UserRole+1, category);

                        FeedFromJSonData(jsonObject, TableType::Monster, category, row);

                        return;
                    }
                }
            }
        }
    }
}

void FightTracker::AddPlayerButtonClicked()
{
    int nextRow = ui->playersFightTable->rowCount();
    ui->playersFightTable->setRowCount(nextRow + 1);
    ui->playersFightTable->setWordWrap(true);

    for (int i = 0; i < ui->playersFightTable->columnCount(); ++i)
    {
        QTableWidgetItem* newItem = nullptr;

        if (ui->playersFightTable->horizontalHeaderItem(i)->text().toLower() == "name")
        {
            newItem = new QTableWidgetItem("Player " + QString::number(nextRow+1));
        }
        else if (i == ToIndex(ColumnsFromEnd::Alteration, TableType::Player))
        {
            newItem = new QTableWidgetItem("");
            QList<AlterationInfo> alterationInfo;
            for (int i = 0; i < m_AlterationCheckBoxes.count(); ++i)
            {
                alterationInfo.append({Qt::Unchecked, 0});
            }

            newItem->setData(Qt::UserRole, QVariant::fromValue(alterationInfo));
        }
        else if (i == ToIndex(ColumnsFromEnd::Note, TableType::Player) || i == ToIndex(ColumnsFromEnd::CountDown, TableType::Player))
        {
            newItem = new QTableWidgetItem("");
        }
        else if (i == ToIndex(ColumnsFromEnd::CountDown, TableType::Player))
        {
            newItem = new QTableWidgetItem();
            newItem->setData(Qt::DisplayRole, 0);
        }
        else if (i == 0)
        {
            newItem = new QTableWidgetItem();
            newItem->setData(Qt::DisplayRole, 10);
        }
        else
        {
            newItem = new QTableWidgetItem("");
        }

        newItem->setTextAlignment(Qt::AlignLeft | Qt::Alignment(Qt::TextWordWrap));
        ui->playersFightTable->setItem(nextRow, i, newItem);

        if (i == (int)Columns::Initiative)
        {
            m_Fighters.append({newItem, TableType::Player});
        }
    }
}

void FightTracker::RollAndDisplayDice(int faces)
{
    int value = DiceHelper::RollDice(faces);
    ui->diceResultLabel->setPlainText(QString::number(value));
}

void FightTracker::ConnectDiceButton(const QPushButton* button)
{
    connect(button, &QPushButton::pressed, this, [this, button]()
    {
        const bool isControlPressed = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
        const bool isAltPressed = QApplication::keyboardModifiers().testFlag(Qt::AltModifier);
        const bool isShiftPressed = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);

        if (isShiftPressed)
        {
            ui->customDiceLineEdit->setText(button->text());
            emit ui->customDiceThrowButton->pressed();
        }
        else if (isControlPressed)
        {
            ui->customDiceLineEdit->setText(ui->customDiceLineEdit->text() + " + " + button->text());
            emit ui->customDiceThrowButton->pressed();
        }
        else if (isAltPressed)
        {
            ui->customDiceLineEdit->setText(ui->customDiceLineEdit->text() + " | " + button->text());
            emit ui->customDiceThrowButton->pressed();
        }
        else
        {
            int total;
            QString result;
            DiceHelper::RollDice(button->text(), result, total, ui->detailedThrowCheckBox->isChecked());
            ui->diceResultLabel->setPlainText(result);
        }
    });
}


void FightTracker::ReinitFightButtonClicked()
{
    if (m_CurrentFighter != nullptr)
    {
        QBrush defaultBrush;
        SetRowBrushes(m_CurrentFighter, m_CurrentFighterTableType, defaultBrush, defaultBrush);
    }

    SetRound(1);

    SortFightButtonClicked();

    for (auto& [item, sheetType] : m_Fighters)
    {
        EnableRow(item, sheetType);
    }

    if (!m_Fighters.isEmpty())
    {
        auto& [figher, tableType] = m_Fighters.front();
        figher->tableWidget()->selectRow(m_Fighters.front().first->row());

        m_CurrentFighter = figher;
        m_CurrentFighterTableType = tableType;
        m_CurrentFighterIndex = 0;

        SetRowBrushes(figher, tableType, QBrush(QColor(160, 255, 200)), QBrush(QColorConstants::Black));

        RollDiceColumn(m_CurrentFighterTableType, m_CurrentFighterIndex);
    }

    m_IsFightInProgress = true;
}

void FightTracker::RestButtonClicked()
{
    HealAll(TableType::Player);
    HealAll(TableType::Monster);
}

void FightTracker::HealAll(TableType tableType)
{
    int hpColumnIndex = m_HPColumnIndex[(int)tableType];
    int hpMaxColumnIndex = m_HPMaxColumnIndex[(int)tableType];

    if (hpColumnIndex > -1 && hpMaxColumnIndex > -1)
    {
        QTableWidget* table = m_Table[(int)tableType];
        for (int i = 0; i < table->rowCount(); ++i)
        {
            int hpMax = table->item(i, hpMaxColumnIndex)->data(Qt::DisplayRole).toInt();
            table->item(i, hpColumnIndex)->setData(Qt::DisplayRole, hpMax);
        }
    }
}

void FightTracker::SortFightButtonClicked()
{
    ui->playersFightTable->sortItems((int)Columns::Initiative, Qt::SortOrder::DescendingOrder);
    ui->monstersFightTable->sortItems((int)Columns::Initiative, Qt::SortOrder::DescendingOrder);

    int sortedMonsterCount = 0;
    int sortedPlayerCount = 0;

    for (int i = 0; i < m_Fighters.count(); ++i)
    {
        auto& [fighter, sheetType] = m_Fighters.at(i);

        if (sheetType == TableType::Monster)
        {
            Utils::MoveRow(*ui->monstersFightTable, fighter->row(), sortedMonsterCount);
            ++sortedMonsterCount;
        }
        else
        {
            Utils::MoveRow(*ui->playersFightTable, fighter->row(), sortedPlayerCount);
            ++sortedPlayerCount;
        }
    }

    SortFighters();
}

void FightTracker::NextTurnButtonClicked()
{
    if (m_CurrentFighterIndex < 0)
    {
        SortFighters();

        if (!m_Fighters.isEmpty())
        {
            m_Fighters.front().first->tableWidget()->selectRow(m_Fighters.front().first->row());
        }
    }

    if (m_CurrentFighterIndex == m_Fighters.count() - 1)
    {
        SetRound(m_CurrentRound + 1);

        for (auto& [item, sheetType]: m_Fighters)
        {
            EnableRow(item, sheetType);
        }
    }

    SelectNextFighter();

    m_IsFightInProgress = true;
}

void FightTracker::SortFighters()
{
    ui->monstersFightTable->clearSelection();
    ui->playersFightTable->clearSelection();

    if (m_Fighters.count() > 0)
    {
        std::sort(m_Fighters.begin(), m_Fighters.end(), [](const QPair<QTableWidgetItem*, TableType>& left, QPair<QTableWidgetItem*, TableType>& right)
        {
            return left.first->text().toInt() > right.first->text().toInt();
        });

        m_CurrentFighterIndex = 0;

        if (m_CurrentFighter != nullptr)
        {
            for (int i = 0; i < m_Fighters.count(); ++i)
            {
                auto& [fighter, sheetType] = m_Fighters.at(i);
                if (fighter == m_CurrentFighter)
                {
                    m_CurrentFighterIndex = i;
                    break;
                }
            }
        }
    }
    else
    {
        if (m_CurrentFighter != nullptr)
        {
            QBrush defaultBrush;
            SetRowBrushes(m_CurrentFighter, m_CurrentFighterTableType, defaultBrush, defaultBrush);
        }

        m_CurrentFighter = nullptr;
    }

    UpdateLastMonsterBeforePlayerHint();
    UpdateChallengeRating();
}

void FightTracker::SelectNextFighter()
{
    if (m_CurrentFighterIndex < 0 || m_CurrentFighterIndex >= m_Fighters.count())
    {
        return;
    }

    if (m_CurrentFighterIndex != m_Fighters.count() - 1)
    {
        auto& [previousFighter, sheetType] = m_Fighters.at(m_CurrentFighterIndex);
        DisableRow(previousFighter, sheetType);
    }

    ui->monstersFightTable->clearSelection();
    ui->playersFightTable->clearSelection();

    m_CurrentFighterIndex = (m_CurrentFighterIndex + 1) % m_Fighters.count();

    auto& [fighter, sheetType] = m_Fighters.at(m_CurrentFighterIndex);
    m_CurrentFighter = fighter;
    m_CurrentFighterTableType = sheetType;

    QTableWidget* table = fighter->tableWidget();
    QAbstractItemModel* model = table->model();
    table->selectRow(fighter->row());
    QModelIndex countDownIndex = model->index(fighter->row(), ToIndex(ColumnsFromEnd::CountDown, sheetType));
    model->setData(countDownIndex, qMax(countDownIndex.data().toInt() - 1, 0));

    QTableWidgetItem* alterationItem = table->item(fighter->row(), ToIndex(ColumnsFromEnd::Alteration, sheetType));
    DecrementAlterationCountDown(*alterationItem);

    RollDiceColumn(sheetType, fighter->row());

    SetRowBrushes(m_CurrentFighter, sheetType, QBrush(QColor(160, 255, 200)), QBrush(QColorConstants::Black));

    UpdateLastMonsterBeforePlayerHint();
}

void FightTracker::RollDiceColumn(TableType tableType, int row)
{
    QAbstractItemModel* model = m_Table[(int)tableType]->model();
    for (auto [diceColumnIndex, resultColumnIndex] : m_DiceColumnIndexes[(int)tableType])
    {
        QModelIndex diceIndex = model->index(row, diceColumnIndex);
        const QString& diceExpression = diceIndex.data().toString();

        QString result;
        int total;
        DiceHelper::RollDice(diceExpression, result, total, ui->detailedThrowCheckBox->isChecked());

        QModelIndex resultIndex = model->index(row, resultColumnIndex);
        model->setData(resultIndex, result);
    }
}

void FightTracker::SetRowBrushes(QTableWidgetItem* item, TableType tableType, const QBrush& backgroundBrush, const QBrush& foregroundBrush)
{
    m_IsUpdatingEnabledState = true;

    int row = item->row();
    if (UnderlinedRowDelegate* delegate = qobject_cast<UnderlinedRowDelegate*>(m_Table[(int)tableType]->itemDelegate()))
    {
        delegate->SetRowBrushes(row, foregroundBrush, backgroundBrush);
    }

    m_IsUpdatingEnabledState = false;
}

void FightTracker::DisableRow(QTableWidgetItem* item, TableType tableType)
{
    SetRowBrushes(item, tableType, QBrush(QColorConstants::Black), QBrush(QColorConstants::White));
}

void FightTracker::EnableRow(QTableWidgetItem* item, TableType tableType)
{
    QBrush defaultBrush;
    SetRowBrushes(item, tableType, defaultBrush, defaultBrush);
}

void FightTracker::RemoveSelectedRow(QTableWidget* table)
{
    QList<int> rowsToDelete;

    for (QTableWidgetItem* item : table->selectedItems())
    {
        int row = item->row();
        if (!rowsToDelete.contains(row))
        {
            rowsToDelete.append(row);
        }
    }

    std::sort(rowsToDelete.begin(), rowsToDelete.end(), [](int left, int right) { return left > right; });

    m_IsDeletingTableItem = true;

    for (int row : rowsToDelete)
    {
        for (int i = 0; i <  ui->monstersFightTable->columnCount(); ++i)
        {
            QTableWidgetItem* item = table->item(row, i);

            if (item == m_CurrentFighter)
            {
                m_CurrentFighter = nullptr;
            }

            TableType tableType = (table == ui->monstersFightTable) ? TableType::Monster : TableType::Player;
            m_Fighters.removeOne({item, tableType});
            delete item;
        }

        table->removeRow(row);
    }

    m_IsDeletingTableItem = false;

    SortFighters();

    if (ui->monstersFightTable->rowCount() == 0)
    {
        m_IsFightInProgress = false;
    }
}

void FightTracker::DuplicateSelectedRow(QTableWidget* table)
{
    QList<int> rowsToProcess;

    for (QTableWidgetItem* item : table->selectedItems())
    {
        int row = item->row();
        if (!rowsToProcess.contains(row))
        {
            rowsToProcess.append(row);
        }
    }

    for (int selectedRow : rowsToProcess)
    {
        AddMonsterButtonClicked();
        int newRow = ui->monstersFightTable->rowCount() - 1;

        QAbstractItemModel* model = ui->monstersFightTable->model();

        for (int i = 0; i < ui->monstersFightTable->columnCount(); ++i)
        {
            QTableWidgetItem* item = table->item(selectedRow, i);

            model->setData(model->index(newRow, i), item->data(Qt::DisplayRole));

            if (i == ToIndex(ColumnsFromEnd::Alteration, TableType::Monster))
            {
                QTableWidgetItem* newItem = table->item(newRow, i);
                newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
            }
        }
    }

    SortFighters();
}

void FightTracker::RollInitiative(QTableWidget* table, TableType tableType, const QString& sheetType)
{
    for (int i = 0; i < table->rowCount(); ++i)
    {
        QAbstractItemModel* model = table->model();

        FeedFunction& initFunction = m_InitModifierFeedFunction[sheetType];
        if (initFunction)
        {
            QTableWidgetItem* item = m_Table[(int)tableType]->item(i, ms_JSonObjectRow);

            const QVariant& variant = item->data(Qt::UserRole);
            if (variant.canConvert((QMetaType)QMetaType::QJsonObject))
            {
                initFunction(variant.toJsonObject());
            }
        }

        int initiative = DiceHelper::RollDice(20) + m_InitModifier;

        model->setData(model->index(i, (int)Columns::Initiative), initiative);
    }

    SortFighters();
}

void FightTracker::ClickOnAlteration(QTableWidget* table, int row, int column)
{
    Q_UNUSED(row)

    m_CurrentAlterationItems.clear();

    for (QTableWidgetItem* item : table->selectedItems())
    {
        if (item->column() == column)
        {
            m_CurrentAlterationItems.append(item);
        }
    }

    const int selectedRowCount = m_CurrentAlterationItems.count();
    if (selectedRowCount > 0)
    {
        const int alterationCount = m_AlterationCheckBoxes.count();

        QPoint mousePosition = QCursor::pos();
        int screenHeight = QApplication::screenAt(mousePosition)->size().height();
        QPoint offset(20, qMin(screenHeight - (mousePosition.y() + m_AlterationWidget->height() + 50), 0));

        m_AlterationWidget->setGeometry(QRect(mousePosition + offset, QSize(150,200)));
        m_AlterationWidget->setVisible(true);

        QList<int> alterationCheckedCount;
        alterationCheckedCount.resize(alterationCount);
        for (QTableWidgetItem* item : m_CurrentAlterationItems)
        {
            QList<QVariant> alterationInfoList = item->data(Qt::UserRole).toList();
            for (int i = 0; i < alterationInfoList.count(); ++i)
            {
                const AlterationInfo& alterationInfo = alterationInfoList[i].value<AlterationInfo>();
                if (alterationInfo.m_CheckState == Qt::Checked)
                {
                    alterationCheckedCount[i]++;
                }

                m_AlterationDurationSpinBoxes[i]->setValue(alterationInfo.m_TimeLimit);
            }
        }

        for (int i = 0; i < alterationCount; ++i)
        {
            Qt::CheckState state = Qt::Unchecked;
            if (alterationCheckedCount[i] == selectedRowCount)
            {
                state = Qt::Checked;
            }
            else if (alterationCheckedCount[i] > 0)
            {
                state = Qt::PartiallyChecked;
            }

            m_AlterationCheckBoxes[i]->setCheckState(state);
        }
    }
}

void FightTracker::ValidateAlterationWidget()
{
    if (m_CurrentAlterationItems.empty())
    {
        return;
    }

    QList<AlterationInfo> rawAlterationInfo;
    for (int i = 0; i < m_AlterationCheckBoxes.count(); ++i)
    {
        const int timeLimit = m_AlterationDurationSpinBoxes[i]->value();
        const Qt::CheckState checkState = m_AlterationCheckBoxes[i]->checkState();

        rawAlterationInfo.append({checkState, timeLimit});
    }

    for (QTableWidgetItem* item : m_CurrentAlterationItems)
    {
        QList<AlterationInfo> wantedAlterationInfo;
        QList<QVariant> currentAlterationInfoList = item->data(Qt::UserRole).toList();

        for (int i = 0; i < m_AlterationCheckBoxes.count(); ++i)
        {
            if (rawAlterationInfo[i].m_CheckState == Qt::PartiallyChecked && (currentAlterationInfoList.count() > i))
            {
                wantedAlterationInfo.append(currentAlterationInfoList[i].value<AlterationInfo>());
            }
            else
            {
                wantedAlterationInfo.append(rawAlterationInfo[i]);
            }
        }

        item->setData(Qt::UserRole, QVariant::fromValue(wantedAlterationInfo));
        UpdateAlterationInfo(*item);
    }

    m_CurrentAlterationItems.clear();

    m_AlterationWidget->setVisible(false);
}

void FightTracker::UpdateAlterationInfo(QTableWidgetItem& item)
{
    QString alterations;
    QString toolTip;

    QList<QVariant> alterationInfoList = item.data(Qt::UserRole).toList();
    for (int i = 0; i < alterationInfoList.count(); ++i)
    {
        const AlterationInfo& alterationInfo = alterationInfoList[i].value<AlterationInfo>();
        QCheckBox* checkBox = m_AlterationCheckBoxes[i];
        int timeLimit = alterationInfo.m_TimeLimit;
        bool checked = alterationInfo.m_CheckState == Qt::Checked;

        if (checked)
        {
            if (!alterations.isEmpty())
            {
                alterations += ", ";
                toolTip += "\n";
            }

            alterations += checkBox->text();
            if (timeLimit > 0)
            {
                alterations += "(" + QString::number(timeLimit) + ")";
            }

            toolTip += checkBox->text() + " : " + checkBox->toolTip();
        }
    }

    item.setData(Qt::DisplayRole, alterations);
    item.setToolTip(toolTip);
}

void FightTracker::DecrementAlterationCountDown(QTableWidgetItem& item)
{
    QList<QVariant> alterationInfoList = item.data(Qt::UserRole).toList();
    for (int i = 0; i < alterationInfoList.count(); ++i)
    {
        AlterationInfo alterationInfo = alterationInfoList[i].value<AlterationInfo>();

        if (alterationInfo.m_CheckState == Qt::Checked && alterationInfo.m_TimeLimit > 0)
        {
            alterationInfo.m_TimeLimit--;
            alterationInfo.m_CheckState = (alterationInfo.m_TimeLimit > 0) ? Qt::Checked : Qt::Unchecked;
        }

        alterationInfoList[i] = QVariant::fromValue(alterationInfo);
    }

    item.setData(Qt::UserRole, alterationInfoList);

    UpdateAlterationInfo(item);
}

void FightTracker::ClearAlterationWidget()
{
    QList<AlterationInfo> alterationInfo;
    for (int i = 0; i < m_AlterationCheckBoxes.count(); ++i)
    {
        alterationInfo.append({Qt::Unchecked, 0});
        m_AlterationCheckBoxes[i]->setCheckState(Qt::CheckState::Unchecked);
        m_AlterationDurationSpinBoxes[i]->setValue(0);
    }

    for (QTableWidgetItem* item : m_CurrentAlterationItems)
    {
        item->setData(Qt::UserRole, QVariant::fromValue(alterationInfo));

        item->setData(Qt::DisplayRole, "");
    }

    m_CurrentAlterationItems.clear();

    m_AlterationWidget->setVisible(false);
}

void FightTracker::ClearAllAlterations()
{
    for (int i = 0; i < (int)TableType::Count; ++i)
    {
        int alterationColumn = ToIndex(ColumnsFromEnd::Alteration, TableType::Monster);
        QTableWidget* table = m_Table[i];
        if (alterationColumn >= 0 && alterationColumn < table->columnCount())
        {
            for (int row = 0; row < table->rowCount(); ++row)
            {
                QTableWidgetItem* item = table->item(row, alterationColumn);

                QList<AlterationInfo> alterationInfo;
                for (int j = 0; j < m_AlterationCheckBoxes.count(); ++j)
                {
                    alterationInfo.append({Qt::Unchecked, 0});
                    item->setData(Qt::DisplayRole, "");
                }

                item->setData(Qt::UserRole, QVariant::fromValue(alterationInfo));
            }
        }
    }

    m_AlterationWidget->setVisible(false);
}

void FightTracker::MonsterSelectionChanged()
{
    UpdateSelectedFigherCount();

    m_JSEngine.collectGarbage();

    if (ui->monstersFightTable->selectedItems().count() > 0)
    {
        QTableWidgetItem* selectedItem = ui->monstersFightTable->selectedItems().front();
        QTableWidgetItem* item = ui->monstersFightTable->item(selectedItem->row(), ms_JSonObjectRow);

        if (item->data(Qt::UserRole).canConvert((QMetaType)QMetaType::QJsonObject))
        {
            const QString& category = item->data(Qt::UserRole+1).toString();
            m_CharacterSheet->FeedMonsterFromJson(item->data(Qt::UserRole).toJsonObject(), category);
        }

        m_UserSelectedFighterName = "";

        if (QTableWidgetItem* nameItem = GetItem(ui->monstersFightTable, selectedItem->row(), "name"))
        {
            m_UserSelectedFighterName = nameItem->data(Qt::DisplayRole).toString();
        }
    }
}

void FightTracker::PlayerSelectionChanged()
{
    UpdateSelectedFigherCount();

    m_JSEngine.collectGarbage();

    if (ui->playersFightTable->selectedItems().count() > 0)
    {
        QTableWidgetItem* selectedItem = ui->playersFightTable->selectedItems().front();
        QTableWidgetItem* item = ui->playersFightTable->item(selectedItem->row(), ms_JSonObjectRow);

        if (item->data(Qt::UserRole).canConvert((QMetaType)QMetaType::QJsonObject))
        {
            const QString& category = item->data(Qt::UserRole+1).toString();

            m_CharacterSheet->FeedMonsterFromJson(item->data(Qt::UserRole).toJsonObject(), category);

            m_UserSelectedFighterName = "";

            if (QTableWidgetItem* nameItem = GetItem(ui->playersFightTable, selectedItem->row(), "name"))
            {
                m_UserSelectedFighterName = nameItem->data(Qt::DisplayRole).toString();
            }
        }
    }
}

void FightTracker::UpdateSelectedFigherCount()
{
    int selectedFighterCount = Utils::GetSelectedRowCount(*ui->playersFightTable) + Utils::GetSelectedRowCount(*ui->monstersFightTable);
    ui->selectedFighterCountLabel->setText(QString::number(selectedFighterCount));
}

void FightTracker::SaveMonsterButtonClicked()
{
    const QString defaultName = "CR_" + QString::number(ComputeCurrentChallengeRating()) + "_.json";

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save monsters"), "encounters/" + defaultName, tr("monster list (*.json)"));
    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }

        QTextStream out(&file);

        QJsonArray dataArray;
        for (int i = 0; i < ui->monstersFightTable->rowCount(); ++i)
        {
            QTableWidgetItem* item = ui->monstersFightTable->item(i, ms_JSonObjectRow);
            QVariant data = item->data(Qt::UserRole);
            if (data.canConvert((QMetaType)QMetaType::QJsonObject))
            {
                dataArray.append(data.toJsonObject());
            }
        }

        QJsonDocument saveDocument(dataArray);

        out << saveDocument.toJson();
    }
}

void FightTracker::LoadMonsterButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load monsters"), "encounters", tr("monster list (*.json)"));
    if (!fileName.isEmpty())
    {
        profileName("Load monsters");

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }

        ClearMonsters();

        QJsonDocument document(QJsonDocument::fromJson(file.readAll()));
        QJsonArray jsonArray = document.array();

        for (const QJsonValue& value : jsonArray)
        {
            QJsonObject jsonObject = value.toObject();
            QString category = jsonObject.find("MorfalDataSourceCategory")->toString();

            AddFromJSonData({value.toObject(), category}, TableType::Monster);
        }

        if (ui->monstersFightTable->rowCount() > 0)
        {
            for (QTableWidgetItem* item : ui->playersFightTable->selectedItems())
            {
                item->setSelected(false);
            }

            ui->monstersFightTable->selectRow(0);
        }
    }

    SortFighters();

    m_IsFightInProgress = false;
}

void FightTracker::LoadPlayersButtonClicked()
{
    profileName("Load players");

    ClearPlayers();

    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Load players"), "players", tr("players (*.json)"));

    for (const QString& fileName : fileNames)
    {
        if (!fileName.isEmpty())
        {
            LoadPlayers(fileName);
        }
    }
}

void FightTracker::LoadPlayers(const QString& filePath)
{
    profile();

    QString playersPath = filePath;
    if (!playersPath.isEmpty() || Utils::TryGetFirstFilePathIn("players", ".json", playersPath))
    {
        QFile file(playersPath);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this, "Unable to open file " + playersPath, file.errorString());
            return;
        }

        const QString& fileName =  Utils::CleanFileName(file.fileName());
        QJsonDocument document(QJsonDocument::fromJson(file.readAll()));

        if (document.isArray())
        {
            QJsonArray jsonArray = document.array();

            for (const QJsonValue& value : jsonArray)
            {
                AddPlayerFromJSonData(value.toObject(), fileName);
            }
        }
        else
        {
            AddPlayerFromJSonData(document.object(), fileName);
        }

        m_LoadedPlayersPath.append(playersPath);
        m_PlayerSheetType.append(fileName);

        for (QTableWidgetItem* item : ui->playersFightTable->selectedItems())
        {
            item->setSelected(false);
        }
    }

    SortFighters();
}

void FightTracker::ClickOnHealth(QTableWidget* table, int row, int column)
{
    QPoint mousePosition = QCursor::pos();
    int screenHeight = QApplication::screenAt(mousePosition)->size().height();

    QPoint offset(20, qMin(screenHeight - (mousePosition.y() + m_AlterationWidget->height() + 50), 0));

    m_HealthWidget->setGeometry(QRect(mousePosition + offset, QSize(150, 200)));
    m_HealthWidget->setVisible(true);

    m_CurrentHealthItem = table->item(row, column);
}


void FightTracker::DamageButtonClicked()
{
    if (m_CurrentHealthItem != nullptr)
    {
        const int currentHealth = m_CurrentHealthItem->data(Qt::DisplayRole).toInt();
        const int newHealth = currentHealth - m_HealthSpinBox->value();

        m_CurrentHealthItem->setData(Qt::DisplayRole, newHealth);
        m_CurrentHealthItem = nullptr;

        m_HealthWidget->setVisible(false);
    }
}

void FightTracker::HealButtonClicked()
{
    if (m_CurrentHealthItem != nullptr)
    {
        const int currentHealth = m_CurrentHealthItem->data(Qt::DisplayRole).toInt();
        const int newHealth = currentHealth + m_HealthSpinBox->value();

        m_CurrentHealthItem->setData(Qt::DisplayRole, newHealth);
        m_CurrentHealthItem = nullptr;

        m_HealthWidget->setVisible(false);
    }
}

void FightTracker::SetHealhButtonClicked()
{
    if (m_CurrentHealthItem != nullptr)
    {
        const int newHealth = m_HealthSpinBox->value();

        m_CurrentHealthItem->setData(Qt::DisplayRole, newHealth);
        m_CurrentHealthItem = nullptr;

        m_HealthWidget->setVisible(false);
    }
}

void FightTracker::CloseHealhButtonClicked()
{
    if (m_CurrentHealthItem != nullptr)
    {
        m_CurrentHealthItem = nullptr;

        m_HealthWidget->setVisible(false);
    }
}

void FightTracker::GenerateEncounterButtonClicked()
{
    const QString dataName = ui->encounterDataFilterComboBox->currentData(Qt::DisplayRole).toString();
    const QString zoneName = ui->encounterZoneFilterComboBox->currentData(Qt::DisplayRole).toString();

    std::vector<JSonCategoryInfo> jsonObjects;

    if (zoneName != "None")
    {
        m_InteractiveMap->GetZoneJsonObjects(zoneName, dataName, jsonObjects);
    }
    else
    {
        ui->jsonDataList->GetFileJsonObjects(dataName, jsonObjects);
    }

    GenerateEncounterFromJSonData(jsonObjects);

    m_IsFightInProgress = false;
}

void FightTracker::GetCurrentMonstersJson(std::vector<JSonCategoryInfo>& monstersJson)
{
    int rowCount = ui->monstersFightTable->rowCount();

    monstersJson.clear();
    monstersJson.reserve(rowCount);

    for (int i = 0; i < rowCount; ++i)
    {
        QTableWidgetItem* jsonHolderItem = ui->monstersFightTable->item(i, ms_JSonObjectRow);

        const QVariant& variant = jsonHolderItem->data(Qt::UserRole);
        if (variant.canConvert((QMetaType)QMetaType::QJsonObject))
        {
            const QString& category = jsonHolderItem->data(Qt::UserRole+1).toString();
            monstersJson.push_back({variant.toJsonObject(), category});
        }
    }
}

void FightTracker::ExecuteCumulatedScriptFunctions(std::vector<JSonCategoryInfoPtr>& monstersJson)
{
    const int monsterCount = (int)monstersJson.size();

    for (auto& [cumulatedVarName, cumulatedFeedScriptFunctions] : m_CumulatedFeedScriptFunctionsMap.toStdMap())
    {
        for (FeedFunction& script : cumulatedFeedScriptFunctions)
        {
            float result = 0.0f;

            for (int i = 0; i < monsterCount; ++i)
            {
                script(*monstersJson[i].first);

                result += m_ScriptResult[cumulatedVarName].toNumber();
            }

            m_ScriptResult[cumulatedVarName] = result;
        }
    }

}

float FightTracker::ComputeChallengeRating(std::vector<JSonCategoryInfoPtr>& monstersJson)
{
    profile();

    float challengeRating = 0;

    m_ScriptResult["MonsterCount"] = (int)monstersJson.size();

    ExecuteCumulatedScriptFunctions(monstersJson);

    QJsonObject emptyJsonObject;

    for (FeedFunction& script : m_GlobalFeedScriptFunctions)
    {
        script(emptyJsonObject);
    }

    if (m_ScriptResult.constFind(m_ChallengeRatingVarName) != m_ScriptResult.cend())
    {
        challengeRating = (float)(m_ScriptResult[m_ChallengeRatingVarName].toNumber());
    }

    return challengeRating;
}

float FightTracker::ComputeCurrentChallengeRating()
{
    profile();

    std::vector<JSonCategoryInfo> monstersJson;
    GetCurrentMonstersJson(monstersJson);

    std::vector<JSonCategoryInfoPtr> monstersJsonPointers;
    monstersJsonPointers.reserve(monstersJson.size());

    for (JSonCategoryInfo& jsonObject : monstersJson)
    {
        monstersJsonPointers.push_back({&jsonObject.first, &jsonObject.second});
    }

    return ComputeChallengeRating(monstersJsonPointers);
}

void FightTracker::UpdateChallengeRating()
{
    m_ChallengeRating = ComputeCurrentChallengeRating();
    UpdateTabName();
}

void FightTracker::UpdateTabName()
{
    const QString currentChallengeRating = QString::number(m_ChallengeRating);
    const QString wantedChallengeRating = QString::number(ui->wantedChallengeRating->value());

    ui->tabs->setTabText(1, "Fight Tracker (CR " + currentChallengeRating + "/" + wantedChallengeRating + ")");
}

void FightTracker::UpdateLastMonsterBeforePlayerHint()
{
    UpdateLastMonsterBeforePlayerHint(m_MonsterDelegate, TableType::Monster);
    UpdateLastMonsterBeforePlayerHint(m_PlayerDelegate, TableType::Player);
}

void FightTracker::UpdateLastMonsterBeforePlayerHint(UnderlinedRowDelegate* delegate, TableType type)
{
    if (delegate == nullptr)
    {
        return;
    }

    bool matchingTypeFound = false;
    int lastMatchingRow = -1;

    for (int i = m_CurrentFighterIndex; i < m_Fighters.count(); ++i)
    {
        auto& [item, tableType] = m_Fighters[i];
        bool isTypeMatching = (tableType == type);
        matchingTypeFound |= isTypeMatching;

        if (matchingTypeFound)
        {
            if (!isTypeMatching)
            {
                delegate->SetUnderlinedRow(lastMatchingRow);
                return;
            }
            else if (i == m_Fighters.count() - 1)
            {
                delegate->SetUnderlinedRow(item->row());
                return;
            }

            lastMatchingRow = item->row();
        }
    }
}

QTableWidgetItem* FightTracker::GetItem(QTableWidget* table, int row, const QString& columnName)
{
    QTableWidgetItem* item = nullptr;

    QHeaderView* headers = table->horizontalHeader();
    for (int i = 0; i < headers->count(); ++i)
    {
        if (table->horizontalHeaderItem(i)->text().toLower() == columnName.toLower())
        {
            item = table->item(row, i);
            break;
        }
    }

    return item;
}

void FightTracker::SetSelectedDiceColumnByFighterName(TableType tableType)
{
    QTableWidget* table = m_Table[(int)tableType];

    for (QTableWidgetItem* item : table->selectedItems())
    {
        const int column = item->column();
        auto columnNameIterator = m_DiceColumnNameMap[(int)tableType].find(column);
        const bool isDiceItem = (columnNameIterator != m_DiceColumnNameMap[(int)tableType].end());
        if (isDiceItem)
        {
            const int row = item->row();
            if (QTableWidgetItem* nameItem = GetItem(table, row, "name"))
            {
                const QVariant& dice = item->data(Qt::DisplayRole);
                const QString& fighterName = nameItem->data(Qt::DisplayRole).toString();

                const QString& columnName = *columnNameIterator;

                SetDiceColumnByFighterName(table, fighterName, columnName, dice, DiceOperation::Set);
            }
        }
    }
}

void FightTracker::SetDiceColumnByFighterName(QTableWidget* table, const QString& figherName, const QString& columnName, const QVariant& content, DiceOperation diceOperation)
{
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (QTableWidgetItem* nameItem = GetItem(table, i, "name"))
        {
            if (nameItem->data(Qt::DisplayRole) == figherName)
            {
                if (QTableWidgetItem* diceItem = GetItem(table, i, columnName))
                {
                    switch (diceOperation)
                    {
                        case DiceOperation::Set:
                        {
                            diceItem->setData(Qt::DisplayRole, content);
                        }
                        break;
                        case DiceOperation::Add:
                        {
                            const QString& data = diceItem->data(Qt::DisplayRole).toString();
                            diceItem->setData(Qt::DisplayRole, data + " + " + content.toString());
                        }
                        break;
                        case DiceOperation::Append:
                        {
                            const QString& data = diceItem->data(Qt::DisplayRole).toString();
                            diceItem->setData(Qt::DisplayRole, data + " | " + content.toString());
                        }
                        break;
                    }
                }
            }
        }
    }
}

bool FightTracker::IsAnyDiceColumnSelect(TableType tableType) const
{
    QTableWidget* table = m_Table[(int)tableType];

    for (const QTableWidgetItem* item : table->selectedItems())
    {
        const bool isDiceItem = (m_DiceColumnNameMap[(int)tableType].find(item->column()) != m_DiceColumnNameMap[(int)tableType].cend());
        if (isDiceItem)
        {
                return true;
        }
    }

    return false;
}
