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

#include "jsonfilters/jsonfilter.h"

#include <QString>
#include <QJsonValue>
#include <QJsonArray>
#include <QJSValue>
#include <QRegExp>
#include <JsonRetrievers/JsonConditionRetriever.h>
#include <JsonRetrievers/JsonRetrieverHelper.h>

#include <sstream>

JSonFilter::JSonFilter(QXmlStreamReader& reader)
{
    while (reader.readNext() && !reader.isEndDocument())
    {
        const QString& elementName = reader.name().toString();
        if (elementName == "condition" && !reader.isEndElement())
        {
            m_ConditionRetrievers.append(JsonRetrieverHelper::BuildCondition(reader));
        }
        else if (elementName == "filter")
        {
            break;
        }
    }
}

bool JSonFilter::IsValid(const QJsonObject& jsonObject) const
{
    return IsValid(QJsonValue(jsonObject));
}

bool JSonFilter::IsValid(const QJsonValue& jsonValue) const
{
    for (JSonConditionRetriever* condition : m_ConditionRetrievers)
    {
        if (condition->IsValid(jsonValue) == false)
        {
            return false;
        }
    }

    return true;
}
