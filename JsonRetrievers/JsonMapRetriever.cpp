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
