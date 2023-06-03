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

#include "dicehelper.h"

#include "utils.h"

#include <QRandomGenerator>

namespace DiceHelper
{
    QRegularExpression DiceRegExp("([0-9]+d(4|6|8|10|12|20|100))((\\+([0-9]+d(4|6|8|10|12|20|100)))+|(\\+([0-9]+))+)*");

    bool TryGetDiceExpression(const QString& sourceText, QString& result, int offset)
    {
        result = "";

        QString text = Utils::GetSubStringBetweenSeparators(sourceText, '(',  ')', offset);
        if (!text.isEmpty())
        {
            text = Utils::RemoveWhiteSpaces(text);
            if (DiceRegExp.match(text).hasMatch())
            {
                result = text;
            }
        }

        return !result.isEmpty();
    }

    int RollDice(int faces)
    {
        return QRandomGenerator::global()->generate() % faces + 1;
    }

    void RollDiceInternal(const QString& diceExpression, QString& result, int& total, bool isDetailedThrow)
    {
        result = "";
        total = 0;

        const QString& trimmedExpression = Utils::RemoveWhiteSpaces(diceExpression);

        if (DiceRegExp.match(trimmedExpression).hasMatch())
        {
            std::istringstream stringStream(trimmedExpression.toStdString());

            int value = 0;

            QString detail;
            std::string diceExpression;
            bool isFirst = true;
            while (std::getline(stringStream, diceExpression, '+'))
            {
                std::size_t separator = (int)diceExpression.find('d');
                if (separator < diceExpression.size())
                {
                    int diceCount = std::atoi(diceExpression.substr(0, separator).c_str());
                    int dice = std::atoi(diceExpression.substr(separator + 1, diceExpression.size() - 1).c_str());

                    if (!isFirst)
                    {
                        detail += "+ ";
                    }

                    detail += (diceExpression + " (").c_str();

                    for (int i = 0; i < diceCount; ++i)
                    {
                        int diceRoll = RollDice(dice);
                        value += diceRoll;

                        if (i > 0)
                        {
                            detail += " + ";
                        }

                        detail += QString::number(diceRoll);
                    }

                    detail += ") ";
                }
                else
                {
                    value += std::atoi(diceExpression.c_str());
                    detail += (" + " + diceExpression + " ").c_str();
                }

                isFirst = false;
            }

            result = QString::number(value);
            total += value;

            if (isDetailedThrow)
            {
                result += " : " + detail;
            }
        }
    }

    void RollDice(const QString& diceExpression, QString& result, int& total, bool isDetailedThrow, bool showTotal)
    {
        result = "";
        total = 0;

        const QStringList dices = diceExpression.split("|");

        for (const QString& dice : dices)
        {
            if (!result.isEmpty())
            {
                result += " | ";
            }

            QString subResult = "";
            int subTotal = 0;
            RollDiceInternal(dice, subResult, subTotal, isDetailedThrow);

            result += subResult;
            total += subTotal;
        }

        if (showTotal)
        {
            result = "[ " + QString::number(total) + " ]  " + result;
        }
    }
}
