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

#include "searchablemultilistdatawiget.h"

#include <QAbstractListModel>
#include <QListWidgetItem>
#include <QCompleter>
#include <QComboBox>
#include <QSizePolicy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QTimer>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>

#include "utils.h"
#include <QStyleOption>
#include <QPainter>
#include "jsonfilters/jsonfilter.h"

SearchableMultiListDataWiget::SearchableMultiListDataWiget(QWidget *parent)
    : super(parent)
{
    m_ComputeMergedJSonTimer = new QTimer(this);

    connect(m_ComputeMergedJSonTimer, &QTimer::timeout, this, &SearchableMultiListDataWiget::ComputeMergedJSon);

    setMinimumWidth(270);

    QVBoxLayout* outerLayout = new QVBoxLayout();
    outerLayout->setObjectName("categoriesOuterLayout");
    ui.outerLayout = outerLayout;
    this->setLayout(outerLayout);
    {
        QFrame* searchFrame = new QFrame(this);
        searchFrame->setObjectName("searchFrame");
        searchFrame->setMaximumHeight(45);
        searchFrame->setStyleSheet("QFrame {border: none;}");
        searchFrame->setContentsMargins(0, 0, 0, 0);

        outerLayout->addWidget(searchFrame);
        {
            QHBoxLayout* searchLayout = new QHBoxLayout();
            searchFrame->setLayout(searchLayout);
            {
                QLabel* searchLabel = new QLabel(this);
                searchLabel->setText(tr("Search"));

                ui.searchText = new QLineEdit(this);
                ui.searchText->setObjectName("searchText");
                connect(ui.searchText, &QLineEdit::textChanged, this, &SearchableMultiListDataWiget::OnSearchTextChanged);

                QPushButton* clearSearchButton = new QPushButton(this);
                clearSearchButton->setMaximumWidth(25);
                clearSearchButton->setText("x");
                clearSearchButton->setObjectName("clearSearchButton");
                connect(clearSearchButton, &QPushButton::clicked, this, &SearchableMultiListDataWiget::OnClearSearchButtonClicked);

                searchLayout->addWidget(searchLabel);
                searchLayout->addWidget(ui.searchText);
                searchLayout->addWidget(clearSearchButton);
            }
        }

        QHBoxLayout* controlsLayout = new QHBoxLayout();
        outerLayout->addItem(controlsLayout);
        {
            QPushButton* expandCategoryButton = new QPushButton(this);
            expandCategoryButton->setText("+");
            expandCategoryButton->setMaximumWidth(25);
            expandCategoryButton->setObjectName("expandCategoryButton");
            expandCategoryButton->setToolTip(tr("Expand all"));
            connect(expandCategoryButton, &QPushButton::clicked, this, &SearchableMultiListDataWiget::OnExpandCategoryButtonClicked);

            QPushButton* collapseCategoryButton = new QPushButton(this);
            collapseCategoryButton->setText("-");
            collapseCategoryButton->setMaximumWidth(25);
            collapseCategoryButton->setObjectName("collapseCategoryButton");
            collapseCategoryButton->setToolTip(tr("Collapse all"));
            connect(collapseCategoryButton, &QPushButton::clicked, this, &SearchableMultiListDataWiget::OnCollapseCategoryButtonClicked);

            QSpacerItem* spacer = new QSpacerItem(100, 20);

            QPushButton* clearSelectionButton = new QPushButton(this);
            clearSelectionButton->setText(tr("Clear selection"));
            clearSelectionButton->setObjectName("clearSelectionButton");
            connect(clearSelectionButton, &QPushButton::clicked, this, &SearchableMultiListDataWiget::OnClearSelectionButtonClicked);

            controlsLayout->addWidget(expandCategoryButton);
            controlsLayout->addWidget(collapseCategoryButton);
            controlsLayout->addItem(spacer);
            controlsLayout->addWidget(clearSelectionButton);
        }

        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea {border: none;}");

        outerLayout->addWidget(scrollArea);
        {
            ui.scrollContent = new QWidget(this);
            scrollArea->setWidget(ui.scrollContent);
            {
                QVBoxLayout* v = new QVBoxLayout();
                ui.categoriesLayout = new QVBoxLayout();
                ui.scrollContent->setLayout(v);
                v->addItem(ui.categoriesLayout);
                ui.scrollContent->setGeometry(0, 0, 248, 393);
                ui.scrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                ui.categoriesLayout->addStretch();

                ui.categoriesLayout->setSizeConstraint(QLayout::SetFixedSize);
            }
        }
    }

    QMetaObject::connectSlotsByName(this);
}

void SearchableMultiListDataWiget::resizeEvent(QResizeEvent *event)
{
    super::resizeEvent(event);

    RefreshCategoryList();
}

void SearchableMultiListDataWiget::RefreshCategoryList()
{
    for (auto& [widget, section]: m_CategoryLists)
    {
        const int wantedHeight = GetListSizedClamped(*widget, height());
        widget->setMinimumHeight(wantedHeight);
        section->OnLayoutChange();
    }
}

int SearchableMultiListDataWiget::GetListSizedClamped(const QListView& list, int windowHeight) const
{
    const int heightPerElement = 14;
    QAbstractListModel* model = (QAbstractListModel*)list.model();
    const int fullListHeight = model->rowCount() * heightPerElement;

    return fmin(fullListHeight, windowHeight - 150);
}

void SearchableMultiListDataWiget::LoadDataList(const std::vector<std::pair<QString, QString>>& dataList, std::map<QString, QString>& categoryMap, QListWidget* sectionInternalList)
{
    for (auto& [itemName, data] : dataList)
    {
        categoryMap[itemName] = data;

        QListWidgetItem* widgetItem = new QListWidgetItem();
        widgetItem->setText(itemName);
        sectionInternalList->addItem(widgetItem);

        m_MergedCategoriesItemName.append(itemName);
    }
}

void SearchableMultiListDataWiget::ListData(const QString& dataPath, const QString& extension, FileDataExtractor& extractor, bool canBeSentToZone)
{
    m_CanBeSentToZone = canBeSentToZone;

    Utils::ParseFilesWithExtractor<SearchableMultiListDataWiget, FileDataExtractor>(dataPath, extension, *this, &SearchableMultiListDataWiget::ParseFile, extractor);

    QCompleter* completer = new QCompleter(m_MergedCategoriesItemName, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(3);
    ui.searchText->setCompleter(completer);
}

void SearchableMultiListDataWiget::ListData(const QString& dataPath, const QString& extension, DirDataExtractor& extractor, bool canBeSentToZone)
{
    m_CanBeSentToZone = canBeSentToZone;

    Utils::ParseDirectoriesWithExtractor<SearchableMultiListDataWiget, DirDataExtractor>(dataPath, extension, *this, &SearchableMultiListDataWiget::ParseFile, extractor);

    QCompleter* completer = new QCompleter(m_MergedCategoriesItemName, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(3);
    ui.searchText->setCompleter(completer);
}

bool SearchableMultiListDataWiget::ItemMatchesSearch(const QString& itemName) const
{
    const QString& searchText = ui.searchText->text();
    const bool matchesSearch = (itemName.contains(searchText, Qt::CaseInsensitive));

    return (searchText.isEmpty() || matchesSearch);
}

bool SearchableMultiListDataWiget::ShouldItemBeAcceptedByFilter(const QString& itemName) const
{
    return (!itemName.isEmpty() && Utils::Contains(m_MergedSelectionItemNames, itemName));
}

bool SearchableMultiListDataWiget::TryGetSettingNameFromCategory(QString category, QString& setting) const
{
    if (m_MonsterCategoryToSetting != nullptr && m_MonsterCategoryToSetting->contains(category))
    {
        setting = m_MonsterCategoryToSetting->at(category);
        return true;
    }

    return false;
}

bool SearchableMultiListDataWiget::ShouldItemBeRejectedByFilter(const QString& fileName, const QString& itemName) const
{
    const QString& data = m_JSonData.at(fileName).at(itemName);
    QJsonDocument jsonData = QJsonDocument::fromJson(data.toUtf8());
    QString setting;
    if (!jsonData.isNull() && jsonData.isObject()
      && TryGetSettingNameFromCategory(fileName, setting)
      && m_ActiveFiltersPerRule.contains(setting))
    {
        QJsonObject jsonObject = jsonData.object();
        const std::vector<JSonFilter*>& filters = m_ActiveFiltersPerRule.at(setting);
        for(JSonFilter* filter : filters)
        {
            if (!filter->IsValid(jsonObject))
            {
                return true;
            }
        }
    }

    return false;
}

void SearchableMultiListDataWiget::OnSelectedCategoryItemChange(const QListView& listView, const QItemSelection& /*deselected*/, const QString& categoryName)
{
    std::vector<QString>& selectionItemNames = m_CategorySelectionItemNames[categoryName];

    QModelIndexList indexList = listView.selectionModel()->selectedIndexes();

    selectionItemNames.clear();

    for (int i = 0; i < indexList.count(); ++i)
    {
        QString itemName = indexList[i].data().toString();
        selectionItemNames.push_back(itemName);
    }

    UpdateMergedSelection();
}

void SearchableMultiListDataWiget::UpdateMergedSelection()
{
    m_MergedSelectionItemNames.clear();

    for (const auto& [category, itemNames] : m_CategorySelectionItemNames)
    {
        m_MergedSelectionItemNames.insert(m_MergedSelectionItemNames.cend(), itemNames.begin(), itemNames.end());
    }

    emit SelectionBeingModified();
    m_ComputeMergedJSonTimer->start(50);
}

void SearchableMultiListDataWiget::ComputeMergedJSon()
{
    m_ComputeMergedJSonTimer->stop();

    const std::size_t totalItemCount = m_MergedSelectionItemNames.size();

    std::size_t totalStringSize = 1;

    std::vector<const QString*> mergedData;
    mergedData.reserve(totalItemCount);

    for (const auto& [category, itemNames] : m_CategorySelectionItemNames)
    {
        const std::map<QString, QString>& categoryMap = m_JSonData[category];
        for (const QString& itemName : itemNames)
        {
            const QString* data = &(categoryMap.at(itemName));
            mergedData.push_back(data);
            totalStringSize += data->size() + 1;
        }
    }

    m_TextToSend.clear();
    m_TextToSend.reserve(totalStringSize);

    const bool hasMultipleItems = (totalItemCount > 1);
    if (hasMultipleItems)
    {
        m_TextToSend += "[";
    }

    std::size_t processedItemCount = 0;

    for (const QString* data : mergedData)
    {
        m_TextToSend += *data;

        if (processedItemCount < totalItemCount - 1)
        {
            m_TextToSend += ",";
        }

        processedItemCount++;
    }

    if (hasMultipleItems)
    {
        m_TextToSend += "]";
    }

    emit SelectionChanged();
}

void SearchableMultiListDataWiget::GetFileJsonObjects(const QString& fileName, std::vector<JSonCategoryInfo>& jsonObjects) const
{
    const bool parseAll = (fileName == "None");

    for (auto& [categoryName, items] : m_JSonData)
    {
        if (categoryName == fileName || parseAll)
        {
            for (const auto& [itemName, data] : items)
            {
                QJsonDocument jsonData = QJsonDocument::fromJson(data.toUtf8());
                if (!jsonData.isNull() && jsonData.isObject())
                {
                    jsonObjects.push_back({jsonData.object(), categoryName});
                }
            }

            break;
        }
    }
}

void SearchableMultiListDataWiget::OnSearchTextChanged()
{
    for (DataSortFilterProxyModel* filter : m_SearchFilters)
    {
        filter->setFilterFixedString(ui.searchText->text());
    }

    const bool isTextEmpty = ui.searchText->text().isEmpty();

    using Mode = QAbstractItemView::SelectionMode;
    Mode mode = isTextEmpty ? Mode::ExtendedSelection : Mode::MultiSelection;

    for (auto& [listView, section] : m_CategoryLists)
    {
        listView->setSelectionMode(mode);
    }

    if (!isTextEmpty)
    {
        ui.searchText->completer()->complete();
    }

    RefreshCategoryList();
}

void SearchableMultiListDataWiget::OnClearSelectionButtonClicked()
{
    m_MergedSelectionItemNames.clear();

    for (auto& [category, itemNames] : m_CategorySelectionItemNames)
    {
        itemNames.clear();
    }

    for (auto& [listView, section] : m_CategoryLists)
    {
        listView->selectionModel()->clear();
    }

    UpdateMergedSelection();

    emit SelectionCleared();
}


void SearchableMultiListDataWiget::OnClearSearchButtonClicked()
{
    ui.searchText->clear();
}

void SearchableMultiListDataWiget::OnFilterTextChanged(const QString& filterName)
{
    m_ActiveFiltersPerRule.clear();

    for (auto& [ruleName, filterMap] : m_LoadedFilters)
    {
        if (JSonFilter* filter = filterMap[filterName])
        {
            m_ActiveFiltersPerRule[ruleName].push_back(filter);
        }
    }

    for (DataSortFilterProxyModel* filter : m_SearchFilters)
    {
        filter->setFilterFixedString(ui.searchText->text());
    }

    RefreshCategoryList();
}

void SearchableMultiListDataWiget::OnExpandCategoryButtonClicked()
{
    for (auto& [listView, section] : m_CategoryLists)
    {
        if (!section->IsExpanded())
        {
            section->toggle(true);
        }
    }
}

void SearchableMultiListDataWiget::OnCollapseCategoryButtonClicked()
{
    for (auto& [listView, section] : m_CategoryLists)
    {
        if (section->IsExpanded())
        {
            section->toggle(false);
        }
    }
}

void SearchableMultiListDataWiget::mouseDoubleClickEvent(QMouseEvent* event)
{
    super::mouseDoubleClickEvent(event);
}

void SearchableMultiListDataWiget::mousePressEvent(QMouseEvent* event)
{
    super::mousePressEvent(event);
}

void SearchableMultiListDataWiget::ItemClicked(const QListView& /*listView*/, const QModelIndex& index, const QString& categoryName)
{
    const QString& itemName = index.data().toString();
    emit SectionItemClicked(categoryName, itemName);
}

void SearchableMultiListDataWiget::ItemDoubleClicked(const QListView& /*listView*/, const QModelIndex &index, const QString& categoryName)
{
    const QString& itemName = index.data().toString();
    emit SectionItemDoubleClicked(categoryName, itemName);
}

void SearchableMultiListDataWiget::paintEvent(QPaintEvent * )
{
    QStyleOption styleOption;
    styleOption.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);
}

void SearchableMultiListDataWiget::SetFilters(Filters& filters, const std::map<QString, QString>* monsterCategoryToSetting)
{
    m_MonsterCategoryToSetting = monsterCategoryToSetting;

    if (filters.size() > 0)
    {
        m_LoadedFilters = filters;

        QComboBox* comboBox = new QComboBox(this);
        comboBox->addItem("None");

        for (auto& [ruleName, filterMap] : filters)
        {
            for (auto& [name, filter] : filterMap)
            {
                comboBox->addItem(name);
            }
        }

        ui.outerLayout->insertWidget(0, comboBox);

        QFrame* frame = new QFrame(this);
        frame->setMaximumHeight(45);
        frame->setStyleSheet("QFrame {border: none;}");
        frame->setContentsMargins(0, 0, 0, 0);

        ui.outerLayout->insertWidget(0, frame);
        {
            QHBoxLayout* searchLayout = new QHBoxLayout();
            frame->setLayout(searchLayout);
            {
                QLabel* label = new QLabel(this);
                label->setText(tr("Filter"));

                connect(comboBox, &QComboBox::currentTextChanged, this, &SearchableMultiListDataWiget::OnFilterTextChanged);

                searchLayout->addWidget(label);
                searchLayout->addWidget(comboBox);
            }
        }
    }
}
