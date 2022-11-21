#pragma once

#include <QWidget>
#include <QList>
#include <QJSEngine>
#include <QBrush>
#include "charactersheet.h"
#include "searchablemultilistdatawiget.h"
#include "xmlscript.h"
#include <QMetaType>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QJsonObject;
class QPushButton;
class QXmlStreamReader;
class QCheckBox;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;
class InteractiveMap;
class MonsterDelegate;
class UnderlinedRowDelegate;

struct AlterationInfo
{
    Qt::CheckState m_CheckState { Qt::Unchecked };
    int m_TimeLimit { 0 };
};
Q_DECLARE_METATYPE(AlterationInfo);

class FightTracker : public QWidget
{
private:
    Q_OBJECT

    using super = QWidget;

    using FeedRowFunction = std::function<void(QTableWidget* table, int row, const QJsonObject& jsonData)>;
    using FeedFunction = std::function<void(const QJsonObject& jsonData)>;

public:
    using JSonCategoryInfoPtr = std::pair<const QJsonObject*, const QString*>;
    using JSonCategoryInfo = std::pair<const QJsonObject, const QString>;

    enum class TableType : short
    {
        Player = 0,
        Monster,
        Count
    };

public:
    explicit FightTracker(QWidget *parent = nullptr);

    void SetUI(Ui::MainWindow* mainWindowUi) { ui = mainWindowUi; }
    void Initialize();
    void InitializeFilters();
    void AddFromJSonData(const std::vector<JSonCategoryInfoPtr>& jsonData, TableType tableType);
    void AddFromJSonData(const std::vector<JSonCategoryInfo>& jsonData, TableType tableType);
    void AddFromJSonData(const JSonCategoryInfo& jsonData, TableType tableType);
    void AddPlayerFromJSonData(const QJsonObject& jsonData, const QString& category);

    void GenerateEncounterFromJSonData(const std::vector<JSonCategoryInfo>& jsonObjects);
    void GenerateEncounterFromJSonData(const std::vector<JSonCategoryInfoPtr>& jsonObjects);

    float ComputeChallengeRating(std::vector<JSonCategoryInfoPtr>& monstersJson);

    inline void SetInfoData(const SearchableMultiListDataWiget::Data* data)
    {
        m_InfoData = data;
    }

    inline void SetInfoDataItemList(const QStringList* list)
    {
        m_InfoDataItemList = list;
    }

    inline void SetCharacterSheet(CharacterSheet* characterSheet)
    {
        m_CharacterSheet = characterSheet;
    }

    void SetInteractiveMap(InteractiveMap* interactiveMap);

    inline void SetRuleSettingsPaths(const std::map<QString, QString>& playerSettingPath,
                                     const std::map<QString, QString>& playerCategoryToSetting,
                                     const std::map<QString, QString>& monsterSettingPath,
                                     const std::map<QString, QString>& monsterCategoryToSetting,
                                     const QString& fightTrackerSettingPath)
    {
        m_PlayerSettingPath = &playerSettingPath;
        m_PlayerCategoryToSetting = &playerCategoryToSetting;
        m_MonsterSettingPath = &monsterSettingPath;
        m_MonsterCategoryToSetting = &monsterCategoryToSetting;
        m_FightTrackerSettingPath = fightTrackerSettingPath;
    }

    void SetFiltersCurrentText(const QString& dataFilterText, const QString& zoneFilterText);

    void SortFighters();

    const QStringList& GetLoadedPlayersPath() { return m_LoadedPlayersPath; }
    void SetLastLoadedPlayersPath(const QStringList& paths) { m_LastLoadedPlayersPath = paths; }

private:
    enum class Columns : short
    {
        Initiative = 0,
        Count
    };

    enum class ColumnsFromEnd : short
    {
        Note = 0,
        Alteration,
        CountDown,
        Count
    };

private:
    void FeedFromJSonData(const QJsonObject& jsonData, TableType tableType, const QString& sheetType, int row);

    void AddMonsterButtonClicked();
    void AddPlayerButtonClicked();
    void MonsterNameChanged(const QString& name, int row);

    void ClearMonsters();
    void ClearPlayers();

    void ReinitFightButtonClicked();
    void RestButtonClicked();
    void HealAll(TableType tableType);
    void SortFightButtonClicked();
    void NextTurnButtonClicked();

    void ConnectDiceButton(const QPushButton* button);

    void RollAndDisplayDice(int faces);
    void SetRound(int round);

    void RollDiceColumn(TableType tableType, int row);

    void RollInitiative(QTableWidget* table, TableType tableType, const QString& sheetType);

    void SelectNextFighter();

    void SetRowBrushes(QTableWidgetItem* item, TableType tableType, const QBrush& backgroundBrush, const QBrush& foregroundBrush);
    void DisableRow(QTableWidgetItem* item, TableType tableType);
    void EnableRow(QTableWidgetItem* item, TableType tableType);

    void RemoveSelectedRow(QTableWidget* table);
    void DuplicateSelectedRow(QTableWidget* table);
    void ClickOnHealth(QTableWidget* table, int row, int column);
    void ClickOnAlteration(QTableWidget* table, int row, int column);
    void ValidateAlterationWidget();
    void UpdateAlterationInfo(QTableWidgetItem& item);
    void DecrementAlterationCountDown(QTableWidgetItem& item);
    void ClearAlterationWidget();
    void ClearAllAlterations();
    void MonsterSelectionChanged();
    void PlayerSelectionChanged();

    void UpdateSelectedFigherCount();

    void SaveMonsterButtonClicked();
    void LoadMonsterButtonClicked();
    void LoadPlayersButtonClicked();

    void DamageButtonClicked();
    void HealButtonClicked();
    void SetHealhButtonClicked();
    void CloseHealhButtonClicked();
    void GenerateEncounterButtonClicked();
    void UpdateEncounterComboBox();

    void LoadPlayers(const QString& filePath = "");

    int ToIndex(ColumnsFromEnd column, TableType tableType);

    void ReadXmlSettings();
    void ReadXmlScipts();
    void AddColumnFromXml(TableType tableType, const std::map<QString, QString>& settingPaths);
    void AddColumnFromXml(QTableWidget* table, TableType tableType, const QString& sheetType, const QString& xml);
    void ReadXml(QXmlStreamReader& reader, QTableWidget* table, TableType tableType, const QString& sheetType);

    void ExecuteCumulatedScriptFunctions(std::vector<JSonCategoryInfoPtr>& monstersJson);
    void GetCurrentMonstersJson(std::vector<JSonCategoryInfo>& monstersJson);
    float ComputeCurrentChallengeRating();
    void UpdateChallengeRating();
    void UpdateTabName();
    void UpdateLastMonsterBeforePlayerHint();
    void UpdateLastMonsterBeforePlayerHint(UnderlinedRowDelegate* delegate, TableType type);

    QTableWidgetItem* GetItem(QTableWidget* table, int row, const QString& columnName);
    void SetSelectedDiceColumnByFighterName(TableType tableType);
    void SetDiceColumnByFighterName(QTableWidget* table, const QString& fighterName, const QString& columnName, const QVariant& content, DiceOperation diceOperation);

    bool IsAnyDiceColumnSelect(TableType tableType) const;

private:
    using DiceColumnIndexes = QList<QPair<int, int>>;

    static const constexpr int ms_JSonObjectRow = 0;

    std::map<QString, QList<FeedRowFunction>> m_FeedFunctions;
    std::map<QString, FeedFunction> m_InitModifierFeedFunction;

    std::map<QString, QList<FeedFunction>> m_FeedScriptFunctions;
    QMap<QString, QJSValue> m_ScriptResult;
    QJSEngine m_JSEngine;

    QList<FeedFunction> m_GlobalFeedScriptFunctions;
    QMap<QString, QMap<QString, XmlScript>> m_XmlScripts;

    DiceColumnIndexes m_DiceColumnIndexes[(int)TableType::Count];
    QMap<int, QString> m_DiceColumnNameMap[(int)TableType::Count];

    int m_InitModifier { 0 };

    QTableWidget* m_Table[(int)TableType::Count];

    MonsterDelegate* m_MonsterDelegate { nullptr };
    UnderlinedRowDelegate* m_PlayerDelegate { nullptr };

    Ui::MainWindow* ui { nullptr };
    int m_CurrentRound { 1 };
    int m_CurrentFighterIndex { -1 };
    QTableWidgetItem* m_CurrentFighter { nullptr };
    TableType m_CurrentFighterTableType;
    QList<QPair<QTableWidgetItem*, TableType>> m_Fighters;
    QList<QCheckBox*> m_AlterationCheckBoxes;
    QList<QSpinBox*> m_AlterationDurationSpinBoxes;
    QWidget* m_AlterationWidget { nullptr };
    QList<QTableWidgetItem*> m_CurrentAlterationItems;

    QWidget* m_HealthWidget { nullptr };
    QSpinBox* m_HealthSpinBox { nullptr };
    QTableWidgetItem* m_CurrentHealthItem { nullptr };

    const SearchableMultiListDataWiget::Data* m_InfoData { nullptr };
    const QStringList* m_InfoDataItemList { nullptr };

    bool m_IsUpdatingEnabledState { false };
    int m_IsFeedingTableItemStack { 0 };
    bool m_IsDeletingTableItem { false };
    CharacterSheet* m_CharacterSheet { nullptr };
    InteractiveMap* m_InteractiveMap { nullptr };

    QString m_ChallengeRatingVarName{ "" };
    QMap<QString, QList<FeedFunction>> m_CumulatedFeedScriptFunctionsMap;

    const std::map<QString, QString>* m_PlayerSettingPath;
    const std::map<QString, QString>* m_PlayerCategoryToSetting;
    const std::map<QString, QString>* m_MonsterSettingPath;
    const std::map<QString, QString>* m_MonsterCategoryToSetting;
    QString m_FightTrackerSettingPath;

    bool m_IsFightInProgress { false };

    float m_ChallengeRating { 0.0f };

    int m_HPColumnIndex[(int)TableType::Count] { -1 };
    int m_HPMaxColumnIndex[(int)TableType::Count] { -1 };

    QBrush m_AlreadyFoughtBrush;
    QBrush m_CurrentFighterBrush;

    QString m_UserSelectedFighterName;
    QStringList m_LoadedPlayersPath;
    QStringList m_LastLoadedPlayersPath;
    QStringList m_PlayerSheetType;
};
