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

#include "interactivemap.h"

#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QFileDialog>
#include <QCompleter>
#include <QXmlStreamReader>

#include "minimalscopeprofiler.h"
#include "mapzonegraphicsobject.h"
#include "jsonfilters/jsonfilter.h"

void ExtractDataListFromFile(Ui::MainWindow* mainWindow, QFile& file, std::vector<std::pair<QString, QString>>& dataList)
{
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QJsonParseError jsonError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll(), &jsonError);
        if (jsonError.error == QJsonParseError::NoError)
        {
            const QString& category = Utils::CleanFileName(file.fileName());

            const QJsonArray& jsonArray = jsonDocument.array();
            for(const auto& element : jsonArray)
            {
                QJsonObject asObject = element.toObject();
                asObject.insert("MorfalDataSourceCategory", category);

                if (asObject.contains("name"))
                {
                    QString elementName = asObject["name"].toString();
                    QJsonDocument doc(asObject);
                    QString data = doc.toJson(QJsonDocument::Compact);

                    FightTracker::JSonCategoryInfoPtr categoryInfoPtr = {&asObject, &elementName };
                    std::vector toCompute { categoryInfoPtr };
                    float challengeRating = mainWindow->fightTrackerWidget->ComputeChallengeRating(toCompute);

                    asObject.insert("MorfalDataChallengeRating", challengeRating);

                    elementName += " (cr " + QString::number(challengeRating) + ")";

                    dataList.push_back(std::make_pair(std::move(elementName), std::move(data)));
                }
            }
        }
    }
}

void InteractiveMap::LoadFilters()
{
    QFileInfo settingsDir;
    if (Utils::TryFindDir("settings", settingsDir))
    {
        QDirIterator rulesDir = Utils::GetFileInfoFromPath(settingsDir.filePath() + "/rules").dir();
        QString ruleName = rulesDir.fileName();

        std::vector<QFileInfo> rulesSubDirectories;
        Utils::GetDirectSubDirList(rulesDir.path(), rulesSubDirectories);

        for (const QFileInfo& ruleDir : rulesSubDirectories)
        {
            QString ruleName = Utils::CleanFileName(ruleDir.fileName());
            QString xmlPath = ruleDir.filePath() + "/filters.xml";

            QFile file(xmlPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QXmlStreamReader reader(&file);
                while (reader.readNext() && !reader.isEndDocument())
                {
                    if (!reader.isEndElement())
                    {
                        const QString& elementName = reader.name().toString();
                        if (elementName == "filter")
                        {
                            const QString& filterName = reader.attributes().value("Name").toString();
                            m_DataFilters[ruleName][filterName] = new JSonFilter(reader);
                        }
                    }
                }
            }
        }

        ui->jsonDataList->SetFilters(m_DataFilters, m_MonsterCategoryToSetting);
    }
}

InteractiveMap::InteractiveMap(Ui::MainWindow* mainWindowUi, CharacterSheet* characterSheet, const std::map<QString, QString>& monsterCategoryToSetting, QObject *parent)
    : super(parent)
    , ui(mainWindowUi)
    , m_CharacterSheet(characterSheet)
    , m_MonsterCategoryToSetting(&monsterCategoryToSetting)
{
    m_UpdateTextToSendTimer = new QTimer(this);
    m_ReactToZoneListChangedTimer = new QTimer(this);
    m_ReactToZoneInfoListChangedTimer = new QTimer(this);

    ui->jsonDataList->SetMainWindow(mainWindowUi);
}

void InteractiveMap::Initialize()
{
    m_DefaultSaveFontSize = ui->saveButton->font().pointSize();

    connect(m_UpdateTextToSendTimer, &QTimer::timeout, this, &InteractiveMap::UpdateTextToSend);
    connect(m_ReactToZoneListChangedTimer, &QTimer::timeout, this, &InteractiveMap::ZoneListSelectionChanged);
    connect(m_ReactToZoneInfoListChangedTimer, &QTimer::timeout, this, &InteractiveMap::ZoneInfoListSelectionChanged);

    SearchableMultiListDataWiget::FileDataExtractor dataExtractor = ExtractDataListFromFile;

    ui->jsonDataList->ListData("data", ".json", dataExtractor, true);
    LoadFilters();

    connect(ui->jsonDataList, &SearchableMultiListDataWiget::SelectionBeingModified, this, &InteractiveMap::DataSelectionBeingModified);
    connect(ui->jsonDataList, &SearchableMultiListDataWiget::SelectionChanged, this, &InteractiveMap::DataSelectionChanged);
    connect(ui->jsonDataList, &SearchableMultiListDataWiget::SelectionCleared, this, &InteractiveMap::DataSelectionCleared);

    connect(ui->mapZoomSlider, &QSlider::valueChanged, this, &InteractiveMap::OnMapZoomSliderValueChanged);
    connect(ui->zoneAlphaSlider, &QSlider::valueChanged, this, &InteractiveMap::OnZoneAlphaSliderValueChanged);
    connect(ui->renameZoneButton, &QPushButton::clicked, this, &InteractiveMap::OnRenameZoneButtonClicked);
    connect(ui->showServerDataCheckBox, &QCheckBox::stateChanged, this, &InteractiveMap::OnCheckBoxStateChanged);
    connect(ui->saveButton, &QPushButton::clicked, this, &InteractiveMap::OnSaveButtonClicked);
    connect(ui->loadMapButton, &QPushButton::clicked, this, &InteractiveMap::OnLoadMapButtonClicked);
    connect(ui->addMapButton, &QPushButton::clicked, this, &InteractiveMap::OnAddMapButtonClicked);
    connect(ui->editZoneButton, &QPushButton::clicked, this, &InteractiveMap::OnEditZoneButtonClicked);
    connect(ui->zoneInfoCategoryComboBox, &QComboBox::currentIndexChanged, this, &InteractiveMap::OnZoneInfoCategoryComboBoxCurrentIndexChanged);

    connect(ui->mapLabel, &DrawableMap::ZoomChanged, this, &InteractiveMap::MapZoomChanged);

    ui->zoomLabel->setText(QString::number(ui->mapZoomSlider->value()) + " %");

    MapZoneGraphicsObject* mapZone = ui->mapLabel->GetMapZoneGraphicsObject();
    connect(mapZone, &MapZoneGraphicsObject::OnEditChanged, this, &InteractiveMap::DrawableMapEditChanged);
    connect(mapZone, &MapZoneGraphicsObject::OnEditModeChanged, this, &InteractiveMap::DrawableMapEditModeChanged);
    connect(mapZone, &MapZoneGraphicsObject::ZoneCreated, this, &InteractiveMap::ZoneCreated);
    connect(mapZone, &MapZoneGraphicsObject::SelectionChanged, this, &InteractiveMap::ZoneSelectionChanged);
    connect(mapZone, &MapZoneGraphicsObject::MeasureUpdated, this, &InteractiveMap::MeasureUpdated);
    connect(mapZone, &MapZoneGraphicsObject::OnRightClickOnZone, this, &InteractiveMap::OnRightClickOnZone);

    connect(ui->mapZoneList->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, [this](const QItemSelection& /*current*/, const QItemSelection& /*deselected*/)
                {
                    m_ReactToZoneListChangedTimer->start(50);
                });

    connect(ui->mapZoneInfoList->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, [this](const QItemSelection& /*current*/, const QItemSelection& /*deselected*/)
                {
                    m_ReactToZoneInfoListChangedTimer->start(50);
                });

    connect(ui->jsonDataList, &SearchableMultiListDataWiget::AddToSelectedZoneRequested, this, &InteractiveMap::AddToSelectedZoneRequested);
    connect(ui->jsonDataList, &SearchableMultiListDataWiget::SetAsZoneDetailsRequested, this, &InteractiveMap::SetAsZoneDetailsRequested);
    connect(ui->jsonDataList, &SearchableMultiListDataWiget::AddToFightTrackerRequested, this, &InteractiveMap::AddToFightTrackerRequested);
    connect(ui->jsonDataList, &SearchableMultiListDataWiget::AddToFightTrackerAsAllyRequested, this, &InteractiveMap::AddToFightTrackerAsAllyRequested);
    connect(ui->jsonDataList, &SearchableMultiListDataWiget::GenerateEncounterRequested, this, &InteractiveMap::GenerateEncounterRequested);

    connect(ui->kmPerPixel, &QDoubleSpinBox::valueChanged, this, [this](double)
    {
        OnMapDirty();
    });

    ui->mapZoneList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapZoneList, &QListWidget::customContextMenuRequested,
            this, [this](const QPoint &pos)
    {
        QPoint globalPosition = ui->mapZoneList->mapToGlobal(pos);

        QMenu menu;
        menu.addAction(tr("Delete"), this, [this]()
        {
            DeleteSelectedZones();
        });

        menu.exec(globalPosition);
    });

    ui->mapZoneInfoList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapZoneInfoList, &QListWidget::customContextMenuRequested,
            this, [this](const QPoint &pos)
    {
        QPoint globalPosition = ui->mapZoneInfoList->mapToGlobal(pos);

        QMenu menu;
        menu.addAction(tr("Remove"), this, [this]()
        {
            RemoveZoneInfoListSelectedItems();
        });

        menu.addAction(tr("Add to fight tracker"), this, [this]()
        {
            AddZoneInfoListSelectedItemsToFightTracker();
        });

        menu.addAction(tr("Generate encounter"), this, [this]()
        {
            GenerateEncounterFromZoneInfoListSelectedItems();
        });

        menu.exec(globalPosition);
    });

    SetupZoneListSearchableView();

    ui->mapLabel->Initialize(ui);
}

void InteractiveMap::SetupZoneListSearchableView()
{
    ui->mapZoneListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapZoneListView, &QListView::customContextMenuRequested,
            this, [this](const QPoint &pos)
    {
        QPoint globalPosition = ui->mapZoneListView->mapToGlobal(pos);

        QMenu menu;
        menu.addAction(tr("Delete"), this, [this]()
        {
            DeleteSelectedZones();
        });

        menu.exec(globalPosition);
    });

    connect(ui->clearZoneSearchBarButton, &QPushButton::clicked, this, [this]()
    {
        ui->zoneSearchBar->clear();
    });

    QCompleter* completer = new QCompleter(ui->mapZoneList->model(), this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(3);
    ui->zoneSearchBar->setCompleter(completer);

    using DataSortFilterProxyModel = FreezableSortFilterProxyModel<InteractiveMap>;
    DataSortFilterProxyModel* searchFilter = new DataSortFilterProxyModel(this);

    searchFilter->setSourceModel(ui->mapZoneList->model());
    ui->mapZoneListView->setModel(searchFilter);
    searchFilter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

    QItemSelectionModel* selectionModel = new QItemSelectionModel(searchFilter, this);
    ui->mapZoneListView->setSelectionModel(selectionModel);

    searchFilter->SetIgnoreFilterPredicate(this, &InteractiveMap::ShouldItemBeAcceptedByFilter);

    ui->mapZoneListView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    connect(ui->zoneSearchBar, &QLineEdit::textChanged, this, [this, searchFilter](const QString& text)
    {
        using Mode = QAbstractItemView::SelectionMode;

        const bool isTextEmpty = text.isEmpty();
        Mode mode = isTextEmpty ? Mode::ExtendedSelection : Mode::MultiSelection;
        ui->mapZoneListView->setSelectionMode(mode);

        searchFilter->setFilterFixedString(text);

        ui->mapZoneList->setVisible(isTextEmpty);
        ui->mapZoneListView->setVisible(!isTextEmpty);
    });

    ui->mapZoneListView->setVisible(false);

    connect(ui->mapZoneList->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (m_IsLoadingMap)
        {
            return;
        }

        const QModelIndexList& selectedIndexes = selected.indexes();

        for (int i = 0; i < selectedIndexes.count(); ++i)
        {
            QString itemName = selectedIndexes[i].data().toString();
            Utils::SelectViewItemByName(*ui->mapZoneListView, itemName, true);
        }

        const QModelIndexList& deselectedIndexes = deselected.indexes();

        for (int i = 0; i < deselectedIndexes.count(); ++i)
        {
            QString itemName = deselectedIndexes[i].data().toString();
            Utils::SelectViewItemByName(*ui->mapZoneListView, itemName, false);
        }
    });

    connect(ui->mapZoneListView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection& selected, const QItemSelection& deselected)
    {
        Q_UNUSED(selected);

        if (m_IsLoadingMap)
        {
            return;
        }

        const QModelIndexList& indexList = ui->mapZoneListView->selectionModel()->selectedIndexes();

        for (int i = 0; i < indexList.count(); ++i)
        {
            QString itemName = indexList[i].data().toString();
            const QList<QListWidgetItem*>& matchingItems = ui->mapZoneList->findItems(itemName, Qt::MatchExactly);
            for (QListWidgetItem* item : matchingItems)
            {
                if (!item->isSelected())
                {
                    item->setSelected(true);
                }
            }
        }

        for (const QItemSelectionRange& range : deselected)
        {
            const QModelIndexList& deselectedIndexList = range.indexes();

            for (int i = 0; i < deselectedIndexList.count(); ++i)
            {
                QString itemName = deselectedIndexList[i].data().toString();
                const QList<QListWidgetItem*>& matchingItems = ui->mapZoneList->findItems(itemName, Qt::MatchExactly);
                for (QListWidgetItem* item : matchingItems)
                {
                    if (item->isSelected())
                    {
                        item->setSelected(false);
                    }
                }
            }
        }
    });
}

bool InteractiveMap::ShouldItemBeAcceptedByFilter(const QString& itemName) const
{
    if (!itemName.isEmpty())
    {
        const QList<QListWidgetItem*>& selection = ui->mapZoneList->selectedItems();
        for (QListWidgetItem* selectedItem : selection)
        {
             if (itemName == selectedItem->data(Qt::DisplayRole))
             {
                 return true;
             }
        }
    }

    return false;
}

void InteractiveMap::OnUiLoaded(const QString& lastLoadedMapPath)
{
    if (lastLoadedMapPath.isEmpty() || LoadMap(lastLoadedMapPath) == false)
    {
        QString mapPath;
        if (Utils::TryGetFirstFilePathIn("map", ".jpg", mapPath))
        {
            LoadMap(mapPath);
        }
    }
}

InteractiveMap::~InteractiveMap()
{
    delete m_UpdateTextToSendTimer;
    delete m_ReactToZoneListChangedTimer;
    delete m_ReactToZoneInfoListChangedTimer;
}

void InteractiveMap::ComputeMergedJSon()
{
    m_MergedJSonElements.clear();
    m_MergedJSonElements.reserve(ui->jsonDataList->GetSelection().size());

    const auto& categorySelectionItemNames = ui->jsonDataList->GetCategorySelectionItemNames();
    for (const auto& [category, itemNames] : categorySelectionItemNames)
    {
        auto& data = ui->jsonDataList->GetData();
        const std::map<QString, QString>& categoryMap = data.at(category);
        for (const QString& itemName : itemNames)
        {
            const QString& data = categoryMap.at(itemName);
            m_MergedJSonElements.push_back(data);
        }
    }

    ui->textToSend->setUpdatesEnabled(false);

    UpdateTextToSend();

    ui->textToSend->setUpdatesEnabled(true);
}

void InteractiveMap::OnCheckBoxStateChanged(int /*state*/)
{
    if (m_MergedJSonElements.size() > 0 || m_MergedZoneInfoElements.size() > 0 || !ui->textToSend->toPlainText().isEmpty())
    {
        ui->textToSend->setPlainText(tr("Loading..."));
        m_UpdateTextToSendTimer->start(50);
    }
}

void InteractiveMap::UpdateTextToSend()
{
    m_UpdateTextToSendTimer->stop();

    std::vector<std::vector<QString>> mergedData;
    mergedData.reserve(2);

    mergedData.push_back(m_MergedJSonElements);
    mergedData.push_back(m_MergedZoneInfoElements);

    Utils::ComputeMergedJSon(mergedData, m_TextToSend);

    if (ui->showServerDataCheckBox->isChecked())
    {
        profileName("set merged json string plain text");
        ui->textToSend->setPlainText(m_TextToSend);
    }
    else
    {
        ui->textToSend->setPlainText(m_TextToSend.isEmpty() ? "" : tr("data ready"));
    }
}

void InteractiveMap::DataSelectionBeingModified()
{
    ui->textToSend->setPlainText("Loading...");
}

void InteractiveMap::DataSelectionChanged()
{
    profile();

    if (ui->jsonDataList->GetSelection().size() > 0)
    {
        const auto& categorySelectionItemNames = ui->jsonDataList->GetCategorySelectionItemNames();
        for (const auto& [category, itemNames] : categorySelectionItemNames)
        {
            auto& data = ui->jsonDataList->GetData();
            const std::map<QString, QString>& categoryMap = data.at(category);
            for (const QString& itemName : itemNames)
            {
                const QString& data = categoryMap.at(itemName);
                m_CharacterSheet->FeedMonsterFromJson(data, category);

                break;
            }
        }
    }

    ComputeMergedJSon();
}

void InteractiveMap::DataSelectionCleared()
{
    m_UpdateTextToSendTimer->start(50);
}

void InteractiveMap::OnMapZoomSliderValueChanged(int value)
{
    SetZoom(value);
}

void InteractiveMap::SetZoom(int zoom)
{
    ui->mapLabel->SetZoom(zoom);

    ui->zoomLabel->setText(QString::number(zoom) + " %");
}

void InteractiveMap::MapZoomChanged(int zoom)
{
    ui->mapZoomSlider->setValue(zoom);
}

void InteractiveMap::DrawableMapEditChanged(bool editStarted)
{
    ui->mapLabel->SetDragEnabled(!editStarted);
}

void InteractiveMap::DrawableMapEditModeChanged(MapZoneGraphicsObject::EditMode editMode)
{
    ui->editZoneButton->setEnabled(editMode == MapZoneGraphicsObject::EditMode::None);
}

void InteractiveMap::ZoneCreated(int id)
{
    QListWidgetItem* item = new QListWidgetItem();
    ui->mapZoneList->addItem(item);

    QString stringId = QString::number(id);
    item->setData(Qt::UserRole, std::move(stringId));
    item->setData(Qt::DisplayRole, "Zone " + QString::number(id));

    if (!m_IsLoadingMap)
    {
        ui->mapLabel->SetSelected(id);
        OnRenameZoneButtonClicked();
    }

    OnMapDirty();
}

void InteractiveMap::GetZoneNames(std::vector<QString>& names) const
{
    for (int i = 0; i < ui->mapZoneList->count(); ++i)
    {
        if (const QListWidgetItem* item = ui->mapZoneList->item(i))
        {
            names.push_back(item->data(Qt::DisplayRole).toString());
        }
    }
}

void InteractiveMap::GetDataListNames(std::vector<QString>& names) const
{
    for (int i = 0; i < ui->mapZoneList->count(); ++i)
    {
        if (const QListWidgetItem* item = ui->mapZoneList->item(i))
        {
            names.push_back(item->data(Qt::DisplayRole).toString());
        }
    }
}

void InteractiveMap::OnRenameZoneButtonClicked()
{
    if (ui->mapZoneList->selectedItems().count() > 0)
    {
        ui->zoneSearchBar->clear();

        ui->mapZoneList->setVisible(true);
        ui->mapZoneListView->setVisible(false);

        QListWidgetItem* item = ui->mapZoneList->selectedItems().first();
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->mapZoneList->editItem(item);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        OnMapDirty();
    }
}

void InteractiveMap::OnEditZoneButtonClicked()
{
    if (ui->mapZoneList->selectedItems().count() > 0)
    {
        QListWidgetItem* item = ui->mapZoneList->selectedItems().first();
        int zoneId = item->data(Qt::UserRole).toInt();
        ui->mapLabel->EditZone(zoneId);

        OnMapDirty();
    }
}

void InteractiveMap::DeleteSelectedZones()
{
    for (QListWidgetItem* item : ui->mapZoneList->selectedItems())
    {
        int zoneId = item->data(Qt::UserRole).toInt();
        ui->mapLabel->DeleteZone(zoneId);
        m_ZoneInfoData.erase(zoneId);
    }

    ui->mapLabel->update();

    qDeleteAll(ui->mapZoneList->selectedItems());

    if (m_IsLoadingMap)
    {
        ui->mapZoneList->clear();
    }

    OnMapDirty();
}

void InteractiveMap::ZoneSelectionChanged(const QList<int>& selection)
{
    profile();

    for (int i = 0; i < ui->mapZoneList->count(); ++i)
    {
        QListWidgetItem* item = ui->mapZoneList->item(i);
        int zoneId = item->data(Qt::UserRole).toInt();
        item->setSelected(selection.contains(zoneId));
    }
}

void InteractiveMap::OnZoneAlphaSliderValueChanged(int value)
{
    ui->mapLabel->SetAlpha(value);

    OnMapDirty();
}

void InteractiveMap::ZoneListSelectionChanged()
{
    profile();

    m_ReactToZoneListChangedTimer->stop();

    QList<int> selection;

    if (!m_IsLoadingMap)
    {
        bool zoneDetailsShown = false;
        for (QListWidgetItem* item : ui->mapZoneList->selectedItems())
        {
            int zoneId = item->data(Qt::UserRole).toInt();
            selection.append(zoneId);

            if (!zoneDetailsShown)
            {
                for (const DataId& selectedInfo : m_ZoneInfoData[zoneId])
                {
                    if (selectedInfo.m_CategoryName.startsWith("[System][Details]"))
                    {
                        QString detailsCategory = selectedInfo.m_CategoryName.split("]").last();
                        const QString& data = m_UsedData[detailsCategory][selectedInfo.m_ItemName];
                        zoneDetailsShown = true;

                        m_CharacterSheet->FeedMonsterFromJson(data, detailsCategory);

                        break;
                    }
                }
            }
        }
    }

    ui->mapLabel->SetSelected(selection);

    RefreshZoneInfoList();
}

void InteractiveMap::AddToSelectedZoneRequested()
{
    profile();

    const SearchableMultiListDataWiget::Data& data = ui->jsonDataList->GetData();
    const auto& categorySelectionItemNames = ui->jsonDataList->GetCategorySelectionItemNames();

    for (QListWidgetItem* item : ui->mapZoneList->selectedItems())
    {
        int zoneId = item->data(Qt::UserRole).toInt();

        for (const auto& [category, itemNames] : categorySelectionItemNames)
        {
            const std::map<QString, QString>& categoryMap = data.at(category);
            for (const QString& itemName : itemNames)
            {
                const QString& itemData = categoryMap.at(itemName);

                m_UsedData[category][itemName] = itemData;
                m_ZoneInfoData[zoneId].append(DataId{.m_CategoryName = category, .m_ItemName = itemName});
            }
        }
    }

    RefreshZoneInfoList();

    OnMapDirty();
}

void InteractiveMap::SetAsZoneDetailsRequested()
{
    profile();

    const SearchableMultiListDataWiget::Data& data = ui->jsonDataList->GetData();
    const auto& categorySelectionItemNames = ui->jsonDataList->GetCategorySelectionItemNames();

    for (QListWidgetItem* item : ui->mapZoneList->selectedItems())
    {
        int zoneId = item->data(Qt::UserRole).toInt();

        if (!categorySelectionItemNames.empty())
        {
            const auto& [category, itemNames] = *categorySelectionItemNames.begin();
            const std::map<QString, QString>& categoryMap = data.at(category);
            if (!itemNames.empty())
            {
                const QString& itemName = itemNames[0];
                const QString& itemData = categoryMap.at(itemName);

                m_UsedData[category][itemName] = itemData;
                m_ZoneInfoData[zoneId].append(DataId{.m_CategoryName = "[System][Details]" + category, .m_ItemName = itemName});
            }
        }
    }

    RefreshZoneInfoList();

    OnMapDirty();
}

void InteractiveMap::GetJsonDataListSelectedJsonObjects(std::vector<FightTracker::JSonCategoryInfo>& jsonObjects)
{
    profile();

    const SearchableMultiListDataWiget::Data& data = ui->jsonDataList->GetData();
    const auto& categorySelectionItemNames = ui->jsonDataList->GetCategorySelectionItemNames();

    for (const auto& [category, itemNames] : categorySelectionItemNames)
    {
        const std::map<QString, QString>& categoryMap = data.at(category);
        for (const QString& itemName : itemNames)
        {
            const QString& itemData = categoryMap.at(itemName);
            QJsonDocument jsonData = QJsonDocument::fromJson(itemData.toUtf8());
            if (!jsonData.isNull() && jsonData.isObject())
            {
                jsonObjects.push_back({jsonData.object(), category});
            }
        }
    }
}

void InteractiveMap::GetZoneJsonObjects(const QString& zoneName, const QString& dataName, std::vector<FightTracker::JSonCategoryInfo>& jsonObjects)
{
    const bool parseAllCategories = (dataName == "None");

    for (int i = 0; i < ui->mapZoneList->count(); ++i)
    {
        if (const QListWidgetItem* item = ui->mapZoneList->item(i))
        {
            if (item->data(Qt::DisplayRole).toString() == zoneName)
            {
                int zoneId = item->data(Qt::UserRole).toInt();
                for (const DataId& selectedInfo : m_ZoneInfoData[zoneId])
                {
                    if (dataName == selectedInfo.m_CategoryName || parseAllCategories)
                    {
                        const QString& data = m_UsedData[selectedInfo.m_CategoryName][selectedInfo.m_ItemName];

                        QJsonDocument jsonData = QJsonDocument::fromJson(data.toUtf8());
                        if (!jsonData.isNull() && jsonData.isObject())
                        {
                            jsonObjects.push_back({jsonData.object(), selectedInfo.m_CategoryName});
                        }
                    }
                }

                break;
            }
        }
    }
}

void InteractiveMap::GetZoneInfoListSelectedJsonObjects(std::vector<FightTracker::JSonCategoryInfo>& jsonObjects)
{
    profile();

    for (QListWidgetItem* item : ui->mapZoneInfoList->selectedItems())
    {
        const QString& categoryName = item->data(Qt::UserRole).toString();
        const QString& itemName = item->data(Qt::UserRole+1).toString();
        const QString& data = m_UsedData[categoryName][itemName];

        QJsonDocument jsonData = QJsonDocument::fromJson(data.toUtf8());
        if (!jsonData.isNull() && jsonData.isObject())
        {
            jsonObjects.push_back({jsonData.object(), categoryName});
        }
    }
}

void InteractiveMap::AddToFightTrackerRequested()
{
    std::vector<FightTracker::JSonCategoryInfo> jsonObjects;
    GetJsonDataListSelectedJsonObjects(jsonObjects);

    ui->fightTrackerWidget->AddFromJSonData(jsonObjects, FightTracker::TableType::Monster);
}

void InteractiveMap::AddToFightTrackerAsAllyRequested()
{
    std::vector<FightTracker::JSonCategoryInfo> jsonObjects;
    GetJsonDataListSelectedJsonObjects(jsonObjects);

    ui->fightTrackerWidget->AddFromJSonData(jsonObjects, FightTracker::TableType::Player);
}

void InteractiveMap::AddZoneInfoListSelectedItemsToFightTracker()
{
    profile();

    std::vector<FightTracker::JSonCategoryInfo> jsonObjects;
    GetZoneInfoListSelectedJsonObjects(jsonObjects);

    ui->fightTrackerWidget->AddFromJSonData(jsonObjects, FightTracker::TableType::Monster);
}

void InteractiveMap::GenerateEncounterRequested()
{
    std::vector<FightTracker::JSonCategoryInfo> jsonObjects;
    GetJsonDataListSelectedJsonObjects(jsonObjects);

    ui->fightTrackerWidget->GenerateEncounterFromJSonData(jsonObjects);
}

void InteractiveMap::GenerateEncounterFromZoneInfoListSelectedItems()
{
    std::vector<FightTracker::JSonCategoryInfo> jsonObjects;
    GetZoneInfoListSelectedJsonObjects(jsonObjects);

    ui->fightTrackerWidget->GenerateEncounterFromJSonData(jsonObjects);
}

void InteractiveMap::RefreshZoneInfoList()
{
    profile();

    Utils::DeleteAll(*ui->mapZoneInfoList);

    const QString& categoryFilter = ui->zoneInfoCategoryComboBox->currentText();
    const bool filterAllowAll = categoryFilter == "All";

    m_ZoneInfoCategories.clear();

    QList<QString> categories;

    for (QListWidgetItem* zoneItem : ui->mapZoneList->selectedItems())
    {
        const int zoneId = zoneItem->data(Qt::UserRole).toInt();

        for (const DataId& selectedInfo : m_ZoneInfoData[zoneId])
        {
            if (!Utils::Contains(*ui->mapZoneInfoList, selectedInfo.m_ItemName)
                && ((filterAllowAll && !selectedInfo.m_CategoryName.startsWith("[System]"))
                    || categoryFilter == selectedInfo.m_CategoryName))
            {
                QListWidgetItem* item = new QListWidgetItem(selectedInfo.m_ItemName);
                item->setData(Qt::UserRole, selectedInfo.m_CategoryName);
                item->setData(Qt::UserRole+1, selectedInfo.m_ItemName);

                ui->mapZoneInfoList->addItem(item);
            }

            if (!categories.contains(selectedInfo.m_CategoryName))
            {
                categories.append(selectedInfo.m_CategoryName);
            }
        }
    }

    m_IsUpdatingCategoryComboBox = true;

    ui->zoneInfoCategoryComboBox->clear();
    ui->zoneInfoCategoryComboBox->addItem("All");
    ui->zoneInfoCategoryComboBox->addItems(categories);

    if (categories.contains(categoryFilter))
    {
        ui->zoneInfoCategoryComboBox->setCurrentText(categoryFilter);
    }

    m_IsUpdatingCategoryComboBox = false;
}

void InteractiveMap::RemoveZoneInfoListSelectedItems()
{
    QStringList namesToRemove;
    for (QListWidgetItem* item : ui->mapZoneInfoList->selectedItems())
    {
        namesToRemove.append(item->data(Qt::DisplayRole).toString());
    }

    for (QListWidgetItem* item : ui->mapZoneList->selectedItems())
    {
        int zoneId = item->data(Qt::UserRole).toInt();

        QList<std::pair<QString, QString>> pairToRemove;
        for (const DataId& dataId : m_ZoneInfoData[zoneId])
        {
            if (namesToRemove.contains(dataId.m_ItemName))
            {
                pairToRemove.append(std::make_pair(dataId.m_CategoryName, dataId.m_ItemName));
            }
        }

        for (const auto& [category, itemName] : pairToRemove)
        {
            const QString& localName = itemName;
            m_ZoneInfoData[zoneId].removeIf([&localName](const DataId& dataId)
                {
                    return localName == dataId.m_ItemName;
                });
        }
    }

    RefreshZoneInfoList();
}

void InteractiveMap::ZoneInfoListSelectionChanged()
{
    profile();

    m_ReactToZoneInfoListChangedTimer->stop();

    m_MergedZoneInfoElements.clear();
    m_MergedZoneInfoElements.reserve(ui->mapZoneInfoList->selectedItems().count());

    QString categoryName;

    for (QListWidgetItem* item : ui->mapZoneInfoList->selectedItems())
    {
        categoryName = item->data(Qt::UserRole).toString();
        const QString& itemName = item->data(Qt::UserRole+1).toString();
        const QString& data = m_UsedData[categoryName][itemName];
        m_MergedZoneInfoElements.push_back(data);
    }

    if (m_MergedZoneInfoElements.size() > 0)
    {
        m_CharacterSheet->FeedMonsterFromJson(m_MergedZoneInfoElements[0], categoryName);
    }

    ui->textToSend->setPlainText(tr("Loading..."));
    m_UpdateTextToSendTimer->start(50);
}

void InteractiveMap::MeasureUpdated(float distance)
{
    float kmDistance = distance / 1000.0f * (float)ui->kmPerPixel->value();
    ui->lastDistanceLabel->setText(QString::number(kmDistance));
}

void InteractiveMap::SaveMapToBinary()
{
    profile();

    using namespace Utils;

    QString mapSaveFilePath = ReplaceExtensionBy(ui->mapLabel->GetMapPath(), ".bsav");

    QFile mapSave(mapSaveFilePath);
    mapSave.open(QIODevice::WriteOnly);

    QByteArray data;
    QDataStream saveStream(&data, QIODevice::WriteOnly);
    saveStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    SaveMapToStream(saveStream);

    /*
                  Load Time     Size     Save Time

        JSon        5,141s | 251 329 Ko | 2,4s
        Bin lvl 0   0,84s  | 429 441 Ko | 0,964s
        Bin lvl 1   1,14s  |  62 901 Ko | 3,022s
        Bin lvl 2   1,09s  |  55 216 Ko | 3,131s
        Bin lvl 3   1,055s |  50 016 Ko | 3,447s
        Bin lvl 4   1,043s |  45 657 Ko | 4,801s
        Bin lvl 5   1,022s |  40 735 Ko | 6,449s

        Bin lvl 2 wins with (compared to JSon)
        - 30% slower to save
        - 22% of the size
        - 22% of the loading time
     */


    const int compressionLevel = 2;
    QByteArray compressedData = qCompress(data, compressionLevel);

    mapSave.write(compressedData);
    mapSave.close();
}

void InteractiveMap::OnMapDirty()
{
    if (!m_IsDirty)
    {
        QFont font = ui->saveButton->font();
        font.setWeight(QFont::ExtraBold);
        font.setPointSize(m_DefaultSaveFontSize + 2);
        font.setItalic(true);
        ui->saveButton->setFont(font);

        m_IsDirty = true;
    }
}

void InteractiveMap::ClearDirtiness()
{
    if (m_IsDirty)
    {
        QFont font = ui->saveButton->font();
        font.setWeight(QFont::Medium);
        font.setPointSize(m_DefaultSaveFontSize);
        font.setItalic(false);
        ui->saveButton->setFont(font);

        m_IsDirty = false;
    }
}

bool InteractiveMap::LoadMapFromBinary(const QString& mapPath)
{
    profile();

    try
    {
        using namespace Utils;

        bool saveFileFound = false;

        QString mapSaveFilePath = ReplaceExtensionBy(mapPath, ".bsav");

        QFile mapSave(mapSaveFilePath);
        if (mapSave.open(QIODevice::ReadOnly))
        {
            saveFileFound = true;

            QByteArray compressedData = mapSave.readAll();
            mapSave.close();

            QByteArray data = qUncompress(compressedData);
            QDataStream loadStream(&data, QIODevice::ReadOnly);
            loadStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            LoadMapFromStream(loadStream);
        }

        return saveFileFound;
    }
    catch (...)
    {
        UnloadCurrentMap();
        return false;
    }
}

void InteractiveMap::OnSaveButtonClicked()
{
    SaveMapToBinary();
    SaveMapToJSon();

    ClearDirtiness();
}

void InteractiveMap::OnLoadMapButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(ui->mapLabel, tr("Load map"), "map", tr("map list (*.jpg)"));
    if (!filePath.isEmpty())
    {
        LoadMap(filePath);
    }
}

void InteractiveMap::OnAddMapButtonClicked()
{
    if (ui->mapZoneList->selectedItems().isEmpty())
    {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(ui->mapLabel, tr("Add map"), "map", tr("map list (*.jpg)"));
    if (!filePath.isEmpty())
    {
        for (QListWidgetItem* item : ui->mapZoneList->selectedItems())
        {
            int zoneId = item->data(Qt::UserRole).toInt();
            QString mapName = filePath.split("/").last().split(".").first();

            m_ZoneInfoData[zoneId].append(DataId{.m_CategoryName = "[System] Map", .m_ItemName = mapName});
        }

        RefreshZoneInfoList();
    }

    OnMapDirty();
}

void InteractiveMap::OnRightClickOnZone(QPoint globalMousePosition, QList<int> zoneIds)
{
    QMenu menu;
    bool showMenu = false;

    QStringList mapsInMenu;

    for (int zoneId : zoneIds)
    {
        const QList<DataId>& zoneInfo = m_ZoneInfoData[zoneId];
        for (auto& [category, itemName] : zoneInfo)
        {
            if (category == "[System] Map")
            {
                const QString& mapName = itemName;

                if (!mapsInMenu.contains(mapName))
                {
                    mapsInMenu.append(mapName);

                    menu.addAction("Go to " + mapName, this, [this, mapName]()
                    {
                        QString mapDirectoryPath;
                        if (Utils::TryFindDirPath("map", mapDirectoryPath))
                        {
                            QFileInfo fileInfo;
                            if (Utils::TryFindDirFromPath(mapDirectoryPath, mapName + ".jpg", fileInfo))
                            {
                                LoadMap(fileInfo.filePath());
                            }
                        }
                    });

                    showMenu = true;
                }
            }
        }
    }

    if (showMenu)
    {
        menu.exec(globalMousePosition);
    }
}

void InteractiveMap::SaveMapToJSon()
{
    profile();

    QString mapSaveFilePath = Utils::ReplaceExtensionBy(ui->mapLabel->GetMapPath(), ".jsav");

    QFile mapSave(mapSaveFilePath);
    mapSave.open(QIODevice::WriteOnly);

    QJsonObject saveStream;

    SaveMapToStream(saveStream);

    QJsonDocument saveDocument(saveStream);
    mapSave.write(saveDocument.toJson(QJsonDocument::JsonFormat::Indented));
}

void InteractiveMap::UnloadCurrentMap()
{
    ui->zoneSearchBar->clear();
    ui->mapZoneList->selectAll();
    DeleteSelectedZones();

    m_ZoneInfoCategories.clear();
    m_ZoneInfoData.clear();
    m_UsedData.clear();
}

bool InteractiveMap::LoadMap(const QString& mapPath)
{
    if (!QFile::exists(mapPath))
    {
        return false;
    }

    m_IsLoadingMap = true;

    UnloadCurrentMap();

    ui->mapLabel->SetImage(mapPath);

    bool loadingSucceded = true;

    if (!LoadMapFromBinary(mapPath))
    {
        loadingSucceded = LoadMapFromJSon(mapPath);
    }

    if (loadingSucceded)
    {
        ClearDirtiness();
    }

    m_IsLoadingMap = false;

    return loadingSucceded;
}

bool InteractiveMap::LoadMapFromJSon(const QString& mapPath)
{
    profile();

    bool saveFileFound = false;

    QString mapSaveFilePath = Utils::ReplaceExtensionBy(mapPath, ".jsav");

    QFile mapSave(mapSaveFilePath);
    if (mapSave.open(QIODevice::ReadOnly))
    {
        saveFileFound = true;

        QByteArray saveData = mapSave.readAll();
        QJsonDocument saveDocument(QJsonDocument::fromJson(saveData));
        QJsonObject loadStream = saveDocument.object();

        LoadMapFromStream(loadStream);
    }

    return saveFileFound;
}

QDataStream& operator<<(QDataStream& out, const InteractiveMap::DataId& dataId)
{
    out << dataId.m_CategoryName << dataId.m_ItemName;
    return out;
}

QDataStream& operator>>(QDataStream& in, InteractiveMap::DataId& dataId)
{
    in >> dataId.m_CategoryName >> dataId.m_ItemName;
    return in;
}

void InteractiveMap::OnZoneInfoCategoryComboBoxCurrentIndexChanged(int /*index*/)
{
    if (!m_IsUpdatingCategoryComboBox)
    {
        RefreshZoneInfoList();
    }
}
