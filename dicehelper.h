#pragma once

#include <QString>
#include <QRegularExpression>

enum class DiceOperation : short
{
    Set,
    Add,
    Append
};

namespace DiceHelper
{
    extern QRegularExpression DiceRegExp;

    bool TryGetDiceExpression(const QString& sourceText, QString& result, int offset = 0);

    int RollDice(int faces);
    void RollDice(const QString& diceExpression, QString& result, int& total, bool isDetailedThrow, bool showTotal = true);
}
