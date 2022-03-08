#pragma once

#include <QMainWindow>
#include <QProcess>

#include "utils.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Section;

class ServerDataSender;
class CharacterSheet;
class MusicPlayer;
class InteractiveMap;
class QPropertyAnimation;

class MainWindow : public QMainWindow
{
private:
    Q_OBJECT

    using super = QMainWindow;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
     void showEvent(QShowEvent *event) override;

private slots:
    void OnExpandServerButtonClicked();
    void OnExpandSheetButtonClicked();
    void OnOpenMusicTabButtonClicked();
    void OnCloseRightTabButtonClicked();

private:
    void WriteSettings();
    void ReadSettings();
    void ReadMapSettings();
    void ReadFightTrackerSettings();
    void RetrieveRuleSettingsPaths();
    void SetupUpExpandableFrames();

    void LoadZoomSettings();

    void OpenRightTab();
    void CloseRightTab();
    void LoadNetworkSettings();

private:
    QString m_ExpandedServerText;
    QString m_CollapsedServerText;

    QString m_LastLoadedMapPath;
    QString m_LastEncounterZoneFilterText;
    QString m_LastEncounterDataFilterText;
    QStringList m_LastLoadedPlayersPath;

    std::map<QString, QString> m_MonsterSettingPaths;
    std::map<QString, QString> m_MonsterCategoryToSetting;
    std::map<QString, QString> m_PlayerSettingPaths;
    std::map<QString, QString> m_PlayerCategoryToSetting;
    QString m_FightTrackerSettingPath;

    Ui::MainWindow* ui { nullptr };

    QPropertyAnimation* m_ServerFrameAnimation { nullptr };
    QPropertyAnimation* m_SheetExpandAnimationMaximum { nullptr };
    QPropertyAnimation* m_SheetExpandAnimationMinimum { nullptr };

    ServerDataSender* m_ServerDataSender { nullptr };
    CharacterSheet* m_CharacterSheet { nullptr };
    MusicPlayer* m_MusicPlayer { nullptr };
    InteractiveMap* m_InteractiveMap { nullptr };
    bool m_IsUiLoaded { false };
    int m_ServerPort { 0 };
};
