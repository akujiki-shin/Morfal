#include "JsonRetrievers/JsonConditionRetriever.h"

#include <QJsonValue>

JSonConditionRetriever::JSonConditionRetriever(const QString& conditionOperator, const QString& conditionOperand, const QString& path, const QString& prefix, const QString& suffix, const QString& varName)
    : super(prefix, path, suffix, varName), m_ConditionOperator(conditionOperator), m_ConditionOperand(conditionOperand)
{
}

QString JSonConditionRetriever::ToStringInternal(const QJsonValue& /*jsonValue*/, QMap<QString, QJSValue>* /*varMap*/) const
{
    return "";
}

bool JSonConditionRetriever::IsValidInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap /* = nullptr */) const
{
    const QJsonValue& value = GetJsonValue(jsonValue, GetPath(), varMap);

    if (value.isNull())
    {
        return false;
    }

    const QString asString = value.toString();
    const double asDouble = (asString != "") ? asString.toDouble() : value.toDouble();

    if (m_ConditionOperator == "IsA")
    {
        if (m_ConditionOperand == "int")
        {
            return value.isDouble();
        }
    }
    else if (m_ConditionOperator == "IsNotNull")
    {
        return asDouble != 0;
    }
    else if (m_ConditionOperator == "StringNotEqual")
    {
        return asString != m_ConditionOperand;
    }
    else if (m_ConditionOperator == "StringEqual")
    {
        return asString == m_ConditionOperand;
    }
    else if (m_ConditionOperator == "StringContains")
    {
        return asString.contains(m_ConditionOperand, Qt::CaseInsensitive);
    }
    else if (m_ConditionOperator == "BoolEqual")
    {
        bool operand = (m_ConditionOperand == "true");
        return value.toBool() == operand;
    }
    else if (m_ConditionOperator == "Lesser")
    {
        return asDouble < m_ConditionOperand.toDouble();
    }
    else if (m_ConditionOperator == "LesserOrEqual")
    {
        return asDouble <= m_ConditionOperand.toDouble();
    }
    else if (m_ConditionOperator == "Higher")
    {
        return asDouble > m_ConditionOperand.toDouble();
    }
    else if (m_ConditionOperator == "HigherOrEqual")
    {
        return asDouble >= m_ConditionOperand.toDouble();
    }
    else if (m_ConditionOperator == "Equal")
    {
        return asDouble == m_ConditionOperand.toDouble();
    }

    return true;
}
