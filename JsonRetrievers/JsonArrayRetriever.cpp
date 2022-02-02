#include "JsonRetrievers/JsonArrayRetriever.h"

#include <QXmlStreamReader>
#include <QJsonArray>
#include <QJsonObject>

#include "JsonRetrievers/JsonConditionRetriever.h"
#include "JsonRetrievers/JsonNumberRetriever.h"
#include "JsonRetrievers/JsonStringRetriever.h"

#include "JsonRetrievers/JsonRetrieverHelper.h"

JSonArrayRetriever::JSonArrayRetriever(QXmlStreamReader& reader, const QString& path, const QString& separator, const QString& prefix, const QString& suffix, const QString& varName)
    : super(prefix, path, suffix, varName) , m_Separator(separator)
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

void JSonArrayRetriever::ToObjectList(const QJsonValue& jsonValue, QList<QJsonObject>& objectList)
{
    const QJsonArray& array = GetJsonValue(jsonValue, GetPath()).toArray();
    for (const QJsonValue& value : array)
    {
        bool isValid = true;

        for (JSonRetriever* retriever : m_RetrieverList)
        {
            if (!retriever->IsValid(value))
            {
                isValid = false;
                break;
            }
        }

        if (isValid)
        {
            objectList.append(value.toObject());
        }
    }
}

void JSonArrayRetriever::SetConcatAsNumber(bool concatAsNumber)
{
    m_ConcatAsNumber = concatAsNumber;
}

QString JSonArrayRetriever::ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const
{
    QString result;

    const QJsonArray& array = GetJsonValue(jsonValue, GetPath(), varMap).toArray();
    for (const QJsonValue& value : array)
    {
        QString localResult;

        if (m_ConcatAsNumber)
        {
            localResult = "0";
        }

        bool isValid = true;

        for (JSonRetriever* retriever : m_RetrieverList)
        {
            localResult += retriever->ToString(value, varMap);

            if (!retriever->IsValid(value))
            {
                isValid = false;
                break;
            }
        }

        if (!isValid)
        {
            continue;
        }

        if (!result.isEmpty() && !localResult.isEmpty())
        {
            if (m_ConcatAsNumber)
            {
                localResult = QString::number(localResult.toDouble() + localResult.toDouble());
            }
            else
            {
                localResult = m_Separator + localResult;
            }
        }

        if (m_ConcatAsNumber)
        {
            result = QString::number(result.toDouble() + localResult.toDouble());
        }
        else
        {
            result += localResult;
        }
    }

    return result;
}
