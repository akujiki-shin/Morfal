#include "mainwindow.h"

#include <QSettings>
#include <QString>
#include <QPropertyAnimation>

#include "serverdatasender.h"
#include "charactersheet.h"
#include "fighttracker.h"
#include "musicplayer.h"
#include "interactivemap.h"
#include "IgnoreHoverEventFilter.h"
#include "minimalscopeprofiler.h"

//#include <QtLocation>
//#include <QtLocation/RouteQuery>

MainWindow::MainWindow(QWidget *parent)
    : super(parent)
    , ui(new Ui::MainWindow)
{
    profileName("Create/Init main window");

    ui->setupUi(this);
    //ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/map.qml")));

    SetupUpExpandableFrames();

    RetrieveRuleSettingsPaths();
    ReadMapSettings();
    ReadFightTrackerSettings();

    m_CharacterSheet = new CharacterSheet(ui, m_PlayerSettingPath, m_MonsterSettingPaths, this);

    m_MusicPlayer = new MusicPlayer(ui, this);
    m_ServerDataSender = new ServerDataSender(ui, this);
    m_InteractiveMap = new InteractiveMap(ui, m_CharacterSheet, this);

    ui->fightTrackerWidget->SetUI(ui);
    ui->fightTrackerWidget->SetInfoData(&ui->jsonDataList->GetData());
    ui->fightTrackerWidget->SetInfoDataItemList(&ui->jsonDataList->GetMergedCategoriesItemName());
    ui->fightTrackerWidget->SetCharacterSheet(m_CharacterSheet);
    ui->fightTrackerWidget->SetInteractiveMap(m_InteractiveMap);
    ui->fightTrackerWidget->SetRuleSettingsPaths(m_PlayerSettingPath, m_MonsterSettingPaths, m_MonsterCategoryToSetting, m_FightTrackerSettingPath);
    ui->fightTrackerWidget->Initialize();

    ui->fightTrackerWidget->SetFiltersCurrentText(m_LastEncounterDataFilterText, m_LastEncounterZoneFilterText);

    m_InteractiveMap->Initialize(m_LastLoadedMapPath);
    ui->fightTrackerWidget->InitializeFilters();

    m_ExpandedServerText = "<<\n\n" + Utils::Verticalize(tr("server")) + "\n<<";
    m_CollapsedServerText = ">>\n\n" + Utils::Verticalize(tr("server")) + "\n>>";

    ui->expandServerButton->setText(m_CollapsedServerText);

    LoadZoomSettings();
    ReadSettings();
}

MainWindow::~MainWindow()
{
    WriteSettings();

    delete m_CharacterSheet;
    delete m_MusicPlayer;
    delete m_ServerDataSender;
    delete m_InteractiveMap;

    delete ui;
}

void MainWindow::SetupUpExpandableFrames()
{
    m_ServerFrameAnimation = new QPropertyAnimation(ui->serverFrame, "maximumWidth", this);

    m_ServerFrameAnimation->setDuration(200);
    m_ServerFrameAnimation->setStartValue(0);
    m_ServerFrameAnimation->setEndValue(300);

    ui->serverFrame->setMaximumWidth(0);

    ui->expandServerButton->installEventFilter(new IgnoreHoverEventFilter());

    m_SheetExpandAnimationMaximum = new QPropertyAnimation(ui->rightSideTabWidget, "maximumSize", this);
    m_SheetExpandAnimationMinimum = new QPropertyAnimation(ui->rightSideTabWidget, "minimumSize", this);

    QSize sizeZero(0, 0);

    m_SheetExpandAnimationMaximum->setDuration(200);
    m_SheetExpandAnimationMaximum->setStartValue(sizeZero);
    m_SheetExpandAnimationMaximum->setEndValue(QSize(1000, 30000));

    m_SheetExpandAnimationMinimum->setDuration(200);
    m_SheetExpandAnimationMinimum->setStartValue(sizeZero);
    m_SheetExpandAnimationMinimum->setEndValue(QSize(475, 0));


    QSettings settings("Morfal", "VPP");
    settings.beginGroup("CharacterSheet");
    bool characterSheetOpened = settings.value("CharacterSheetOpened").toBool();
    settings.endGroup();

    ui->expandSheetButton->setChecked(characterSheetOpened);

    if (characterSheetOpened)
    {
        ui->rightSideTabWidget->setMinimumWidth(475);
        ui->rightSideTabWidget->setMaximumWidth(1000);

        ui->expandSheetButton->setMaximumHeight(0);
        ui->openMusicTabButton->setMaximumHeight(0);

        ui->closeRightTabButton->setMaximumHeight(30000);
    }
    else
    {
        ui->rightSideTabWidget->setMinimumWidth(0);
        ui->rightSideTabWidget->setMaximumWidth(0);

        ui->expandSheetButton->setMaximumHeight(30000);
        ui->openMusicTabButton->setMaximumHeight(30000);

        ui->closeRightTabButton->setMaximumHeight(0);
    }

    ui->expandSheetButton->installEventFilter(new IgnoreHoverEventFilter());
    ui->openMusicTabButton->installEventFilter(new IgnoreHoverEventFilter());
    ui->closeRightTabButton->installEventFilter(new IgnoreHoverEventFilter());

    connect(ui->expandServerButton, &QPushButton::clicked, this, &MainWindow::OnExpandServerButtonClicked);
    connect(ui->expandSheetButton, &QPushButton::clicked, this, &MainWindow::OnExpandSheetButtonClicked);
    connect(ui->openMusicTabButton, &QPushButton::clicked, this, &MainWindow::OnOpenMusicTabButtonClicked);
    connect(ui->closeRightTabButton, &QPushButton::clicked, this, &MainWindow::OnCloseRightTabButtonClicked);
}

void MainWindow::WriteSettings()
{
    QSettings settings("Morfal", "VPP");

    settings.beginGroup("MainWindow");

    if (isMaximized())
    {
        settings.setValue("isMaximized", true);
    }
    else
    {
        settings.setValue("size", size());
        settings.setValue("isMaximized", false);
        settings.setValue("pos", pos());
    }

    settings.endGroup();

    settings.beginGroup("Map");

    settings.setValue("MapPath", ui->mapLabel->GetMapPath());

    settings.endGroup();

    settings.beginGroup("FightTracker");

    settings.setValue("EncounterZoneFilterText", ui->encounterZoneFilterComboBox->currentText());
    settings.setValue("EncounterDataFilterText", ui->encounterDataFilterComboBox->currentText());
    settings.setValue("WantedChallengeRating", ui->wantedChallengeRating->value());

    settings.endGroup();

    settings.beginGroup("CharacterSheet");

    settings.setValue("CharacterSheetOpened", ui->closeRightTabButton->maximumHeight() > 0);

    settings.endGroup();
}

void MainWindow::ReadSettings()
{
    QSettings settings("Morfal", "VPP");

    settings.beginGroup("MainWindow");

    resize(settings.value("size", QSize(400, 400)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());

    if (settings.value("isMaximized").toBool())
    {
        showMaximized();
    }

    settings.endGroup();
}

void MainWindow::ReadMapSettings()
{
    QSettings settings("Morfal", "VPP");

    settings.beginGroup("Map");

    m_LastLoadedMapPath = settings.value("MapPath").toString();

    settings.endGroup();
}

void MainWindow::ReadFightTrackerSettings()
{
    QSettings settings("Morfal", "VPP");

    settings.beginGroup("FightTracker");

    m_LastEncounterZoneFilterText = settings.value("EncounterZoneFilterText").toString();
    m_LastEncounterDataFilterText = settings.value("EncounterDataFilterText").toString();

    ui->wantedChallengeRating->setValue(settings.value("WantedChallengeRating").toInt());

    settings.endGroup();
}

void MainWindow::OnExpandServerButtonClicked()
{
    if (ui->expandServerButton->isChecked())
    {
        ui->expandServerButton->setText(m_ExpandedServerText);
        m_ServerFrameAnimation->setDirection(QAbstractAnimation::Forward);
    }
    else
    {
        ui->expandServerButton->setText(m_CollapsedServerText);
        m_ServerFrameAnimation->setDirection(QAbstractAnimation::Backward);
    }

    m_ServerFrameAnimation->start();
}

void MainWindow::OnExpandSheetButtonClicked()
{
    ui->rightSideTabWidget->setCurrentIndex(0);
    OpenRightTab();
}

void MainWindow::OnOpenMusicTabButtonClicked()
{
    ui->rightSideTabWidget->setCurrentIndex(1);
    OpenRightTab();
}

void MainWindow::OnCloseRightTabButtonClicked()
{
    CloseRightTab();
}

void MainWindow::OpenRightTab()
{
    ui->tabSheetSplitter->setCollapsible(1, false);

    m_SheetExpandAnimationMaximum->setDuration(200);
    m_SheetExpandAnimationMaximum->setDirection(QAbstractAnimation::Forward);
    m_SheetExpandAnimationMinimum->setDirection(QAbstractAnimation::Forward);

    m_SheetExpandAnimationMinimum->start();
    m_SheetExpandAnimationMaximum->start();

    ui->expandSheetButton->setMaximumHeight(0);
    ui->openMusicTabButton->setMaximumHeight(0);

    ui->closeRightTabButton->setMaximumHeight(30000);
}

void MainWindow::CloseRightTab()
{
    m_SheetExpandAnimationMaximum->setDirection(QAbstractAnimation::Backward);
    m_SheetExpandAnimationMinimum->setDirection(QAbstractAnimation::Backward);

    m_SheetExpandAnimationMinimum->start();
    m_SheetExpandAnimationMaximum->start();

    ui->expandSheetButton->setMaximumHeight(30000);
    ui->openMusicTabButton->setMaximumHeight(30000);

    ui->closeRightTabButton->setMaximumHeight(0);
}

void MainWindow::RetrieveRuleSettingsPaths()
{
    QString settingsPath;
    if (Utils::TryFindDirPath("settings", settingsPath))
    {
        const QString prefix = settingsPath + "/rules/";

        const QString xmlPath = settingsPath + "/Application.xml";

        QFile file(xmlPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QXmlStreamReader reader(&file);
            while (reader.readNext() && !reader.isEndDocument())
            {
                if (!reader.isEndElement())
                {
                    const QString& elementName = reader.name().toString();
                    if (elementName == "PlayerSetting")
                    {
                        m_PlayerSettingPath = prefix + reader.attributes().value("SettingFolder").toString() + "/jsonToQtXml";
                    }
                    else if (elementName == "MonsterSetting")
                    {
                        const QString dataFile = reader.attributes().value("DataFileName").toString();
                        const QString settingFolder = reader.attributes().value("SettingFolder").toString();

                        m_MonsterSettingPaths[dataFile] = prefix + settingFolder + "/jsonToQtXml";
                        m_MonsterCategoryToSetting[dataFile] = settingFolder;
                    }
                    else if (elementName == "FightTrackerSetting")
                    {
                        m_FightTrackerSettingPath = prefix + reader.attributes().value("SettingFolder").toString();
                    }
                }
            }
        }
    }
}

void MainWindow::LoadZoomSettings()
{
    QString xmlPath;
    if (Utils::TryFindDirPath("settings", xmlPath))
    {
        xmlPath += "/Application.xml";

        QFile file(xmlPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QXmlStreamReader reader(&file);
            while (reader.readNext() && !reader.isEndDocument())
            {
                if (!reader.isEndElement())
                {
                    const QString& elementName = reader.name().toString();
                    if (elementName == "Zoom")
                    {
                        int min = reader.attributes().value("Min").toInt();
                        int max = reader.attributes().value("Max").toInt();
                        int increment = reader.attributes().value("Increment").toInt();

                        max = qBound(100, max, 2000);
                        min = qBound(1, min, max - 1);
                        increment = qBound(1, increment, max);

                        ui->mapLabel->SetZoomRange(min, max);
                        ui->mapLabel->SetZoomIncrement(increment);

                        ui->mapZoomSlider->setMinimum(min);
                        ui->mapZoomSlider->setMaximum(max);

                        return;
                    }
                }
            }
        }
    }
}
