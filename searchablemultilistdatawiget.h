#pragma once

#include <QWidget>

#include <QDirIterator>
#include <QListView>
#include <QString>
#include <QVBoxLayout>
#include <QApplication>
#include <QMenu>
#include <QListWidget>

#include "freezablesortfilterproxymodel.h"
#include "Section.h"
#include "mainwindow.h"

namespace Utils
{
    QString RemoveJsonExtension(const QString& name);
}

class JSonFilter;

class SearchableMultiListDataWiget : public QWidget
{
private:
    Q_OBJECT

    using super = QWidget;

public:
    explicit SearchableMultiListDataWiget(QWidget* parent = nullptr);

    using Data = std::map<QString, std::map<QString, QString>>;
    using Filters = std::map<QString, std::map<QString, JSonFilter*>>;
    using JSonCategoryInfoPtr = std::pair<const QJsonObject*, const QString*>;
    using JSonCategoryInfo = std::pair<const QJsonObject, const QString>;

signals:
    void SelectionBeingModified();
    void SelectionChanged();
    void SelectionCleared();
    void SectionItemClicked(const QString& categoryName, const QString& itemName);
    void SectionItemDoubleClicked(const QString& categoryName, const QString& itemName);
    void AddToSelectedZoneRequested();
    void SetAsZoneDetailsRequested();
    void AddToFightTrackerRequested();
    void AddToFightTrackerAsAllyRequested();
    void GenerateEncounterRequested();

private slots:
    void OnSearchTextChanged();
    void OnClearSearchButtonClicked();
    void OnClearSelectionButtonClicked();

    void OnFilterTextChanged(const QString& filterName);

    void OnExpandCategoryButtonClicked();

    void OnCollapseCategoryButtonClicked();

    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;

    void ItemClicked(const QListView& listView, const QModelIndex &index, const QString& categoryName);
    void ItemDoubleClicked(const QListView& listView, const QModelIndex &index, const QString& categoryName);

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

public:
    using FileDataExtractor = std::function<void(Ui::MainWindow* ui, QFile&, std::vector<std::pair<QString, QString>>&)>;
    using DirDataExtractor = std::function<void(QDirIterator&, std::vector<std::pair<QString, QString>>&, const QString&)>;

    void ListData(const QString& dataPath, const QString& extension, FileDataExtractor& extractor, bool canBeSentToZone);
    void ListData(const QString& dataPath, const QString& extension, DirDataExtractor& extractor, bool canBeSentToZone);

    const std::vector<QString>& GetSelection() const { return m_MergedSelectionItemNames; }
    const std::map<QString, std::vector<QString>>& GetCategorySelectionItemNames() const { return m_CategorySelectionItemNames; }
    const Data& GetData() const { return m_JSonData; }
    const QStringList& GetMergedCategoriesItemName() const { return m_MergedCategoriesItemName; }

    void GetFileJsonObjects(const QString& fileName, std::vector<JSonCategoryInfo>& jsonObjects) const;

    void paintEvent(QPaintEvent * ) override;

    inline void SetMainWindow(Ui::MainWindow* mainWindow) { m_MainWindow = mainWindow; }
    void SetFilters(Filters& filters, const std::map<QString, QString>* monsterCategoryToSetting);
    bool TryGetSettingNameFromCategory(QString category, QString& setting) const;

private:
    void LoadDataList(const std::vector<std::pair<QString, QString>>& dataList, std::map<QString, QString>& categoryMap, QListWidget* sectionInternalList);

    template<class T>
    void ParseFile(QDirIterator& file, const QString& localPath, T& extractor, const QString& extensions = "");

    void OnSelectedCategoryItemChange(const QListView& listView, const QItemSelection& deselected, const QString& categoryName);
    int GetListSizedClamped(const QListView& list, int windowHeight) const;
    void RefreshCategoryList();

    void UpdateMergedSelection();
    void ComputeMergedJSon();

    bool ItemMatchesSearch(const QString& itemName) const;
    bool ShouldItemBeAcceptedByFilter(const QString& itemName) const;
    bool ShouldItemBeRejectedByFilter(const QString& fileName, const QString& itemName) const;

private:
    Data m_JSonData;
    std::map<QString, std::vector<QString>> m_CategorySelectionItemNames;
    std::vector<QString> m_MergedSelectionItemNames;
    std::map<QString, std::vector<JSonFilter*>> m_ActiveFiltersPerRule;
    Filters m_LoadedFilters;
    std::vector<std::pair<QListView*, Section*>> m_CategoryLists;
    QStringList m_MergedCategoriesItemName;
    QTimer* m_ComputeMergedJSonTimer { nullptr };
    QString m_TextToSend;
    bool m_CanBeSentToZone{ false };
    const std::map<QString, QString>* m_MonsterCategoryToSetting{ nullptr };

    std::vector<QItemSelectionModel*> m_SelectionModels;

    using DataSortFilterProxyModel = FreezableSortFilterProxyModel<SearchableMultiListDataWiget>;
    std::vector<DataSortFilterProxyModel*> m_SearchFilters;

    struct UI
    {
        QVBoxLayout* categoriesLayout { nullptr };
        QLineEdit* searchText { nullptr };
        QWidget* scrollContent { nullptr };
        QVBoxLayout* outerLayout { nullptr };
    };

    UI ui {};

    Ui::MainWindow* m_MainWindow { nullptr };
};

template<class T>
void SearchableMultiListDataWiget::ParseFile(QDirIterator& fileIterator, const QString& localPath, T& extractor, const QString& extensions)
{
    QString fileName = Utils::RemoveJsonExtension(fileIterator.fileName());
    QString localName = localPath;

    if (fileIterator.fileInfo().isFile())
    {
        localName += fileName;
    }

    std::map<QString, QString>& categoryMap = m_JSonData[fileName];

    Section* section = new Section(ui.scrollContent, localName, 50);

    QListWidget* sectionInternalList = new QListWidget();
    sectionInternalList->setUniformItemSizes(false);
    sectionInternalList->setStyleSheet("QListWidget {border: none;}");
    sectionInternalList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    QListView* listView = new QListView(this);
    DataSortFilterProxyModel* searchFilter = new DataSortFilterProxyModel(this);

    searchFilter->setSourceModel(sectionInternalList->model());
    listView->setModel(searchFilter);
    searchFilter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    m_SearchFilters.push_back(searchFilter);

    QItemSelectionModel* selectionModel = new QItemSelectionModel(searchFilter, this);
    m_SelectionModels.push_back(selectionModel);
    listView->setSelectionModel(selectionModel);

    searchFilter->SetIgnoreFilterPredicate(this, &SearchableMultiListDataWiget::ShouldItemBeAcceptedByFilter);
    searchFilter->SetForceFilterOutPredicate(this, &SearchableMultiListDataWiget::ShouldItemBeRejectedByFilter);
    searchFilter->SetDataFileName(fileName);

    listView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, [this, listView, fileName](const QItemSelection& /*current*/, const QItemSelection& deselected)
    {
        OnSelectedCategoryItemChange(*listView, deselected, fileName);
    });

    connect(listView, &QListView::pressed,
          this, [this, listView, fileName](const QModelIndex &index)
    {
        if (QApplication::mouseButtons() & Qt::RightButton)
        {
            ItemClicked(*listView, index, fileName);
        }
    });

    connect(listView, &QListView::doubleClicked,
          this, [this, listView, fileName](const QModelIndex &index)
    {
        ItemDoubleClicked(*listView, index, fileName);
    });

    if (m_CanBeSentToZone)
    {
        listView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(listView, &QListView::customContextMenuRequested,
                this, [this, listView, fileName](const QPoint &pos)
        {
            QPoint globalPosition = listView->mapToGlobal(pos);

            QMenu menu;
            menu.addAction(tr("Add to selected zone"), this, [this]()
            {
                emit AddToSelectedZoneRequested();
            });

            menu.addAction(tr("Set as selected zone details"), this, [this]()
            {
                emit SetAsZoneDetailsRequested();
            });

            menu.addAction(tr("Generate encounter"), this, [this]()
            {
                emit GenerateEncounterRequested();
            });

            menu.addAction(tr("Add to fight tracker"), this, [this]()
            {
                emit AddToFightTrackerRequested();
            });

            menu.addSeparator();

            menu.addAction(tr("Add to fight tracker as ally"), this, [this]()
            {
                emit AddToFightTrackerAsAllyRequested();
            });

            menu.exec(globalPosition);
        });
    }

    std::vector<std::pair<QString, QString>> dataList;

    if constexpr(std::is_same<T, FileDataExtractor>())
    {
        QFile file(fileIterator.filePath());
        extractor(m_MainWindow, file, dataList);

        LoadDataList(dataList, categoryMap, sectionInternalList);

        if (file.isOpen())
        {
            file.close();
        }
    }
    else if constexpr(std::is_same<T, DirDataExtractor>())
    {
        extractor(fileIterator, dataList, extensions);

        LoadDataList(dataList, categoryMap, sectionInternalList);
    }

    QVBoxLayout* sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(listView);
    listView->setMinimumHeight(0);

    section->setContentLayout(*sectionLayout);
    section->setMinimumHeight(0);

    m_CategoryLists.push_back(std::make_pair(listView, section));

    ui.categoriesLayout->insertWidget(ui.categoriesLayout->count() - 1, section);
    ui.categoriesLayout->setAlignment(section, Qt::Alignment(Qt::AlignmentFlag::AlignTop));
}
