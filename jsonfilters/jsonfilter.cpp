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
