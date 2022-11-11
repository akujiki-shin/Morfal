#pragma once

#include <QObject>

#include <QListWidgetItem>
#include <QString>

#include "ui_mainwindow.h"

#include "utils.h"
#include "saveutils.h"
#include "mapzonegraphicsobject.h"

#include <QMenu>

class CharacterSheet;

class InteractiveMap : public QObject
{
private:
    Q_OBJECT

    using super = QObject;

public:
    explicit InteractiveMap(Ui::MainWindow* mainWindowUi, CharacterSheet* characterSheet, const std::map<QString, QString>& monsterCategoryToSetting, QObject *parent = nullptr);
    ~InteractiveMap();

    using InfoData = std::map<QString, // category (mob, city, ...)
                        std::map<QString, // item name
                                 QString>>; // data

    inline InfoData* GetInfoDataPtr()
    {
        return &m_UsedData;
    }

    void Initialize();
    void SetupZoneListSearchableView();
    void OnUiLoaded(const QString& lastLoadedMapPath);

    bool ShouldItemBeAcceptedByFilter(const QString& itemName) const;

    void GetZoneNames(std::vector<QString>& names) const;
    void GetDataListNames(std::vector<QString>& names) const;
    void GetZoneJsonObjects(const QString& zoneName, const QString& dataName, std::vector<FightTracker::JSonCategoryInfo>& jsonObjects);

private slots:
    void DataSelectionBeingModified();
    void DataSelectionChanged();
    void DataSelectionCleared();

    void OnMapZoomSliderValueChanged(int value);
    void ZoneCreated(int id);
    void ZoneSelectionChanged(const QList<int>& selection);
    void ZoneListSelectionChanged();
    void ZoneInfoListSelectionChanged();
    void MeasureUpdated(float distance);
    void OnRightClickOnZone(QPoint globalMousePosition, QList<int> zoneIds);

    void OnRenameZoneButtonClicked();

    void DeleteSelectedZones();

    void OnZoneAlphaSliderValueChanged(int value);

    void GetJsonDataListSelectedJsonObjects(std::vector<FightTracker::JSonCategoryInfo>& jsonObjects);
    void GetZoneInfoListSelectedJsonObjects(std::vector<FightTracker::JSonCategoryInfo>& jsonObjects);

    void AddToSelectedZoneRequested();
    void SetAsZoneDetailsRequested();
    void AddToFightTrackerRequested();
    void AddToFightTrackerAsAllyRequested();

    void GenerateEncounterRequested();
    void GenerateEncounterFromZoneInfoListSelectedItems();

    void OnSaveButtonClicked();
    void OnLoadMapButtonClicked();
    void OnAddMapButtonClicked();

    void OnEditZoneButtonClicked();

    void OnZoneInfoCategoryComboBoxCurrentIndexChanged(int index);
    void OnCheckBoxStateChanged(int state);

private:
    void ComputeMergedJSon();

    void UpdateTextToSend();
    void LoadFilters();

    void SetZoom(int zoom);
    void MapZoomChanged(int zoom);
    void DrawableMapEditChanged(bool editStarted);
    void DrawableMapEditModeChanged(MapZoneGraphicsObject::EditMode editMode);

    void RefreshZoneInfoList();
    void RemoveZoneInfoListSelectedItems();
    void AddZoneInfoListSelectedItemsToFightTracker();

    void OnMapDirty();
    void ClearDirtiness();

    bool LoadMapFromBinary(const QString& mapPath);
    void SaveMapToBinary();

    void UnloadCurrentMap();
    bool LoadMap(const QString& mapPath);
    bool LoadMapFromJSon(const QString& mapPath);
    void SaveMapToJSon();

    template<typename SaveStreamType>
    void SaveMapToStream(SaveStreamType& saveStream);

    template<typename LoadStreamType>
    void LoadMapFromStream(LoadStreamType& loadStream);

public:
    struct DataId
    {
        QString m_CategoryName;
        QString m_ItemName;
    };

private:
    using ZoneInfoData =
        std::map<int, // zone id
            QList<DataId>>;

private:
    const int m_Version = 1;

    ZoneInfoData m_ZoneInfoData;
    std::map<int, QList<QString>> m_ZoneInfoCategories;
    InfoData m_UsedData;

    SearchableMultiListDataWiget::Filters m_DataFilters;

    bool m_IsUpdatingCategoryComboBox { false };

    Ui::MainWindow *ui { nullptr };

    QTimer* m_UpdateTextToSendTimer { nullptr };
    QTimer* m_ReactToZoneListChangedTimer { nullptr };
    QTimer* m_ReactToZoneInfoListChangedTimer { nullptr };
    QString m_TextToSend;
    std::vector<QString> m_MergedJSonElements;
    std::vector<QString> m_MergedZoneInfoElements;

    const std::map<QString, QString>* m_MonsterCategoryToSetting { nullptr };

    CharacterSheet* m_CharacterSheet { nullptr };

    bool m_IsLoadingMap { false };
    bool m_IsDirty { false };
    int m_DefaultSaveFontSize { 10 };
};


template<typename SaveStreamType>
void InteractiveMap::SaveMapToStream(SaveStreamType& saveStream)
{
    using namespace SaveUtils;

    using ParentScope = typename ParentSaveScope<SaveStreamType>::type;

    SaveUtils::Save(saveStream, "Version", m_Version);
    SaveUtils::Save(saveStream, "kmPerPixel", ui->kmPerPixel->value());

    ParentScope categoryMapParent;
    SaveUtils::Save(saveStream, "categoryMapSize", (int)m_UsedData.size());
    for (const auto& [categoryName, itemMap] : m_UsedData)
    {
        SaveStreamType* categoryMapStream = OpenSaveScope(saveStream);

        SaveUtils::Save(*categoryMapStream, "categoryName", categoryName);

        ParentScope itemMapParent;
        SaveUtils::Save(*categoryMapStream, "itemMapSize", (int)itemMap.size());
        for (const auto& [itemName, itemData] : itemMap)
        {
            SaveStreamType* itemMapStream = OpenSaveScope(*categoryMapStream);

            SaveUtils::Save(*itemMapStream, "itemName", itemName);
            SaveUtils::Save(*itemMapStream, "itemData", itemData);

            SaveScope(itemMapParent, *itemMapStream);
            CloseSaveScope(itemMapStream);
        }

        SaveParent(*categoryMapStream, "itemMap", itemMapParent);

        SaveScope(categoryMapParent, *categoryMapStream);
        CloseSaveScope(categoryMapStream);
    }

    SaveParent(saveStream, "categoryMap", categoryMapParent);

    ParentScope zoneInfoDataParent;
    SaveUtils::Save(saveStream, "ZoneInfoDataSize", (int)m_ZoneInfoData.size());
    for (const auto& [id, dataIdList] : m_ZoneInfoData)
    {
        SaveStreamType* zoneInfoDataStream = OpenSaveScope(saveStream);

        SaveUtils::Save(*zoneInfoDataStream, "id", id);
        SaveUtils::SaveAsByte(*zoneInfoDataStream, "dataIdList", dataIdList);

        SaveScope(zoneInfoDataParent, *zoneInfoDataStream);
        CloseSaveScope(zoneInfoDataStream);
    }

    SaveParent(saveStream, "zoneInfoData", zoneInfoDataParent);

    ParentScope mapZoneListParent;
    SaveUtils::Save(saveStream, "mapZoneListSize", ui->mapZoneList->count());
    for (int i = 0; i < ui->mapZoneList->count(); ++i)
    {
        SaveStreamType* mapZoneLisStream = OpenSaveScope(saveStream);

        QListWidgetItem* item = ui->mapZoneList->item(i);
        SaveUtils::Save(*mapZoneLisStream, "zoneName", item->data(Qt::DisplayRole).toString());

        SaveScope(mapZoneListParent, *mapZoneLisStream);
        CloseSaveScope(mapZoneLisStream);
    }

    SaveParent(saveStream, "mapZoneList", mapZoneListParent);

    SaveUtils::Save(saveStream, "zoom", ui->mapZoomSlider->value());

    ui->mapLabel->Save(saveStream, m_Version);
}


template<typename LoadStreamType>
void InteractiveMap::LoadMapFromStream(LoadStreamType& loadStream)
{
    using namespace SaveUtils;

    using ParentScope = typename ParentSaveScope<LoadStreamType>::type;
    using LoadScopeObject = typename LoadScopeObject<LoadStreamType>::type;

    m_ZoneInfoData.clear();

    int version;
    Load(loadStream, "Version", version);

    double kmPerPixel;
    Load(loadStream, "kmPerPixel", kmPerPixel);
    ui->kmPerPixel->setValue(kmPerPixel);

    ParentScope categoryMapParent = LoadParent(loadStream, "categoryMap");
    int categoryMapSize;
    Load(loadStream, "categoryMapSize", categoryMapSize);
    for (int categoryIndex = 0; categoryIndex < categoryMapSize; ++categoryIndex)
    {
        LoadScopeObject categoryMapScope;
        LoadStreamType& categoryMapStream = AsStream(loadStream, categoryMapScope, categoryMapParent, categoryIndex);

        QString categoryName;
        Load(categoryMapStream, "categoryName", categoryName);

        ParentScope itemMapParent = LoadParent(categoryMapStream, "itemMap");
        int itemMapSize;
        Load(categoryMapStream, "itemMapSize", itemMapSize);
        for (int itemIndex = 0; itemIndex < itemMapSize; ++itemIndex)
        {
            LoadScopeObject itemMapScope;
            LoadStreamType& itemMapStream = AsStream(categoryMapStream, itemMapScope, itemMapParent, itemIndex);

            QString itemName;
            QString itemData;

            Load(itemMapStream, "itemName", itemName);
            Load(itemMapStream, "itemData", itemData);

            m_UsedData[categoryName][itemName] = itemData;
        }
    }

    ParentScope zoneInfoDataParent = LoadParent(loadStream, "zoneInfoData");
    int zoneInfoDataSize;
    Load(loadStream, "ZoneInfoDataSize", zoneInfoDataSize);
    for (int idIndex = 0; idIndex < zoneInfoDataSize; ++idIndex)
    {
        LoadScopeObject zoneInfoDataScope;
        LoadStreamType& zoneInfoDataStream = AsStream(loadStream, zoneInfoDataScope, zoneInfoDataParent, idIndex);

        int id;
        Load(zoneInfoDataStream, "id", id);

        QList<DataId> dataIdList;
        LoadFromByte(zoneInfoDataStream, "dataIdList", dataIdList);

        m_ZoneInfoData[id] = dataIdList;

        ZoneCreated(id);
    }

    ParentScope mapZoneListParent = LoadParent(loadStream, "mapZoneList");
    int mapZoneListSize;
    Load(loadStream, "mapZoneListSize", mapZoneListSize);
    for (int i = 0; i < mapZoneListSize; ++i)
    {
        LoadScopeObject mapZoneListScope;
        LoadStreamType& mapZoneListStream = AsStream(loadStream, mapZoneListScope, mapZoneListParent, i);

        QString zoneName;
        Load(mapZoneListStream, "zoneName", zoneName);

        if (QListWidgetItem* item = ui->mapZoneList->item(i))
        {
            item->setData(Qt::DisplayRole, zoneName);
        }
        else
        {
            qErrnoWarning("Load: Invalid mapZoneList item.");
        }
    }

    int zoom;
    Load(loadStream, "zoom", zoom);

    SetZoom(zoom);

    ui->mapLabel->Load<LoadStreamType>(loadStream);

    ui->zoneAlphaSlider->setValue(ui->mapLabel->GetAlpha());
}
