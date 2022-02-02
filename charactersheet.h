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
    explicit CharacterSheet(Ui::MainWindow* mainWindowUi, const QString& playerSettingPath, const std::map<QString, QString>& monsterSettingPaths, QObject *parent = nullptr);

    void FeedMonsterFromJson(const QJsonObject& jsonData, const QString& category);
    void FeedPlayerFromJson(const QJsonObject& jsonData);

    void FeedMonsterFromJson(const QString& jsonString, const QString& category);
    void FeedPlayerFromJson(const QString& jsonString);

    void ClearMonster();
    void ClearPlayer();

    void RegisterDiceExpressionReceiver(const QString& receiverName);

signals:
    void DiceExpressionSelected(const QString& diceExpression, bool replaceCustomDice);
    void DiceExpressionSentToReceiver(const QString& receiverName, const QString& diceExpression, DiceOperation diceOperation);

private:
    void HideAllSheets();
    void HideSheet(const QString& type = "player");
    void ShowSheet(const QString& type = "player");
    void HideSheetPart(QWidget* widget);
    void ShowSheetPart(QWidget* widget);
    void ConnectDice(JsonToQtXmBuilder* builder);

private:
    std::map<QString, QList<QWidget*>> m_SheetPartsToShow;
    std::map<QString, QList<QWidget*>> m_SheetPartsToHide;

    Ui::MainWindow* ui { nullptr };

    QString m_PlayerSettingPath;
    QStringList m_DiceExpressionReceivers;

    struct MonsterSheet
    {
        JsonToQtXmBuilder* m_MainInformation;
        JsonToQtXmBuilder* m_Spells;
        JsonToQtXmBuilder* m_ItemAndActions;
    };

    std::map<QString, MonsterSheet> m_MonsterSheets;
    QString m_CurrentMonsterCategory{};
};
