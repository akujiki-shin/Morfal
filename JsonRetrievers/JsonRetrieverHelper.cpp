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

#include "JsonRetrievers/JsonRetrieverHelper.h"

#include "JsonRetrievers/JsonArrayRetriever.h"
#include "JsonRetrievers/JsonConditionRetriever.h"
#include "JsonRetrievers/JsonMapRetriever.h"
#include "JsonRetrievers/JsonNumberRetriever.h"
#include "JsonRetrievers/JsonStringRetriever.h"
#include "JsonRetrievers/JsonRetriever.h"
#include <QJsonObject>
#include <QJsonValue>

void JsonRetrieverHelper::BuildRetrieverList(QXmlStreamReader& reader, QList<JSonRetriever*>& list)
{
    bool errorFound = false;
    while (reader.readNextStartElement() && !errorFound)
    {
        JsonRetrieverHelper::XmlTypes xmlType;
        if (JsonRetrieverHelper::TryReadElementAsType(reader, xmlType, errorFound))
        {
            if (xmlType == XmlTypes::jsonInt)
            {
                list.append(BuildJsonInt(reader));
            }
            else if (xmlType == XmlTypes::jsonFloat)
            {
                list.append(BuildJsonFloat(reader));
            }
            else if (xmlType == XmlTypes::jsonString)
            {
                list.append(BuildJsonString(reader));
            }
            else if (xmlType == XmlTypes::map)
            {
                list.append(BuildJsonMap(reader));
            }
            else if (xmlType == XmlTypes::array)
            {
                list.append(BuildJsonArray(reader));
            }
            else if (xmlType == XmlTypes::condition)
            {
                list.append(BuildCondition(reader));
            }
        }
    }
}

JSonIntRetriever* JsonRetrieverHelper::BuildJsonInt(QXmlStreamReader& reader)
{
    const QString& prefix = reader.attributes().value("Prefix").toString();
    const QString& path = reader.attributes().value("Path").toString();
    const QString& suffix = reader.attributes().value("Suffix").toString();
    const QString& varName = reader.attributes().value("VarName").toString();
    const bool showSign = (reader.attributes().value("ShowSign").toString() == "true");

    reader.skipCurrentElement();

    return new JSonIntRetriever(prefix, path, suffix, showSign, varName);
}

JSonFloatRetriever* JsonRetrieverHelper::BuildJsonFloat(QXmlStreamReader& reader)
{
    const QString& prefix = reader.attributes().value("Prefix").toString();
    const QString& path = reader.attributes().value("Path").toString();
    const QString& suffix = reader.attributes().value("Suffix").toString();
    const QString& varName = reader.attributes().value("VarName").toString();
    const bool showSign = (reader.attributes().value("ShowSign").toString() == "true");

    reader.skipCurrentElement();

    return new JSonFloatRetriever(prefix, path, suffix, showSign, varName);
}

JSonStringRetriever* JsonRetrieverHelper::BuildJsonString(QXmlStreamReader& reader)
{
    const QString& prefix = reader.attributes().value("Prefix").toString();
    const QString& path = reader.attributes().value("Path").toString();
    const QString& suffix = reader.attributes().value("Suffix").toString();
    const QString& varName = reader.attributes().value("VarName").toString();

    reader.skipCurrentElement();

    return new JSonStringRetriever(prefix, path, suffix, varName);
}

JSonMapRetriever* JsonRetrieverHelper::BuildJsonMap(QXmlStreamReader& reader)
{
    const QString& prefix = reader.attributes().value("Prefix").toString();
    const QString& path = reader.attributes().value("Path").toString();
    const QString& suffix = reader.attributes().value("Suffix").toString();
    const QString& separator = reader.attributes().value("Separator").toString();
    const bool displayKey = (reader.attributes().value("DisplayKey").toString() == "true");

    JSonMapRetriever* retriever = new JSonMapRetriever(reader, path, separator, displayKey, prefix, suffix);

    return retriever;
}

JSonArrayRetriever* JsonRetrieverHelper::BuildJsonArray(QXmlStreamReader& reader)
{
    const QString& prefix = reader.attributes().value("Prefix").toString();
    const QString& path = reader.attributes().value("Path").toString();
    const QString& suffix = reader.attributes().value("Suffix").toString();
    const QString& separator = reader.attributes().value("Separator").toString();
    const QString& itemsNamePath = reader.attributes().value("itemsNamePath").toString();
    bool concatAsNumber = (reader.attributes().value("concatAsNumber").toString() == "true");

    JSonArrayRetriever* retriever = new JSonArrayRetriever(reader, path, separator, prefix, suffix);

    retriever->SetItemNamePath(itemsNamePath);
    retriever->SetConcatAsNumber(concatAsNumber);

    return retriever;
}

JSonConditionRetriever* JsonRetrieverHelper::BuildCondition(QXmlStreamReader& reader)
{
    const QString& conditionOperator = reader.attributes().value("Operator").toString();
    const QString& operand = reader.attributes().value("Operand").toString();
    const QString& path = reader.attributes().value("Path").toString();
    const QString& varName = reader.attributes().value("VarName").toString();

    reader.skipCurrentElement();

    return new JSonConditionRetriever(conditionOperator, operand, path, "", "", varName);
}

void JsonRetrieverHelper::RegisterMathScript(QXmlStreamReader& reader, QJSEngine& jsEngine, QMap<QString, QJSValue>& scriptResult, QList<FeedFunction>& feedFunctions)
{
    const QString& expression = reader.attributes().value("Expression").toString();
    const QString& varName = reader.attributes().value("VarName").toString();

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [expression, varName, retrieverList, &scriptResult, &jsEngine](const QJsonObject& jsonData)
    {
        QString fullExpression = expression;

        bool isValid = true;

        for (int i = 0; i < retrieverList.count() && isValid; ++i)
        {
            QString identifier = "{" + QString::number(i) +"}";
            const QString& value = retrieverList[i]->ToString(jsonData, &scriptResult);

            if (!value.isEmpty())
            {
                fullExpression.replace(identifier, value);
            }

            isValid &= retrieverList[i]->IsValid(jsonData, &scriptResult);
        }

        bool allIdentifiersReplaced = (fullExpression.contains('{') == false);

        if (allIdentifiersReplaced && isValid)
        {
            scriptResult[varName] = jsEngine.evaluate(fullExpression);
        }
    };

    feedFunctions.append(feedFunction);
}

void JsonRetrieverHelper::RegisterXmlScript(QXmlStreamReader& reader, const std::map<QString, QString>& categoryToScriptSetting, XmlScriptBank& xmlScriptBank, QMap<QString, QJSValue>& scriptResult, QList<FeedFunction>& feedFunctions)
{
    const QString& scriptName = reader.attributes().value("ScriptName").toString();
    const QString& varName = reader.attributes().value("VarName").toString();

    QList<JSonRetriever*> retrieverList;
    JsonRetrieverHelper::BuildRetrieverList(reader, retrieverList);

    FeedFunction feedFunction = [scriptName, varName, retrieverList, &scriptResult, &xmlScriptBank, &categoryToScriptSetting](const QJsonObject& jsonData)
    {
        const QString& category = jsonData.find("MorfalDataSourceCategory")->toString();
        if (!category.isEmpty())
        {
            auto scriptSettingIterator = categoryToScriptSetting.find(category);
            if (scriptSettingIterator != categoryToScriptSetting.end())
            {
                const QString& scriptSetting = scriptSettingIterator->second;
                scriptResult[varName] = xmlScriptBank[scriptName][scriptSetting].Execute(jsonData);
            }
        }
    };

    feedFunctions.append(feedFunction);
}
