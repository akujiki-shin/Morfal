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

#include "JsonRetrievers/JsonMapRetriever.h"

#include <QXmlStreamReader>
#include <QJsonArray>
#include <QJsonObject>

#include "JsonRetrievers/JsonConditionRetriever.h"
#include "JsonRetrievers/JsonNumberRetriever.h"
#include "JsonRetrievers/JsonStringRetriever.h"

#include "JsonRetrievers/JsonRetrieverHelper.h"

JSonMapRetriever::JSonMapRetriever(QXmlStreamReader& reader, const QString& path, const QString& separator, bool displayKey, const QString& prefix, const QString& suffix, const QString& varName)
    : super(prefix, path, suffix, varName) , m_Separator(separator), m_DisplayKey(displayKey)
{
    bool errorFound = false;
    while (reader.readNextStartElement() && !errorFound)
    {
        JsonRetrieverHelper::XmlTypes xmlType;
        if (JsonRetrieverHelper::TryReadElementAsType(reader, xmlType, errorFound))
        {
            switch (xmlType)
            {
                case JsonRetrieverHelper::XmlTypes::jsonInt:
                {
                    m_RetrieverList.append(JsonRetrieverHelper::BuildJsonInt(reader));
                }
                break;

                case JsonRetrieverHelper::XmlTypes::jsonFloat:
                {
                    m_RetrieverList.append(JsonRetrieverHelper::BuildJsonFloat(reader));
                }
                break;

                case JsonRetrieverHelper::XmlTypes::jsonString:
                {
                    m_RetrieverList.append(JsonRetrieverHelper::BuildJsonString(reader));
                }
                break;

                case JsonRetrieverHelper::XmlTypes::condition:
                {
                    m_RetrieverList.append(JsonRetrieverHelper::BuildCondition(reader));
                }
                break;

                default: break;
            }
        }
    }
}

QString JSonMapRetriever::ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const
{
    QString result;

    const QJsonObject& localObject = GetJsonValue(jsonValue, GetPath(), varMap).toObject();

    const auto& map = localObject.toVariantMap().toStdMap();
    for (auto& [key, value] : map)
    {
        QString localResult;

        if (!result.isEmpty())
        {
            localResult += m_Separator;
        }

        if (m_DisplayKey)
        {
            localResult += key;
        }

        bool isValid = true;

        const QJsonValue& localValue = value.toJsonValue();
        for (JSonRetriever* retriever : m_RetrieverList)
        {
            localResult += retriever->ToString(localValue);

            if (!retriever->IsValid(localValue))
            {
                isValid = false;
                break;
            }
        }

        if (!isValid)
        {
            continue;
        }

        result += localResult;
    }

    return result;
}
