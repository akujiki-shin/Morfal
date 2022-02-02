#pragma once

#include "JsonRetrievers/JsonRetriever.h"

#include <QJsonValue>
#include <QVariant>

template<typename ValueType>
class JsonNumberRetriever : public JSonRetriever
{
private:
    using super = JSonRetriever;

public:
    JsonNumberRetriever(const QString& prefix, const QString& path, const QString& suffix, bool showSign, const QString& varName);

protected:
    QString ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const override;

private:
    bool m_ShowSign { false };
};

template<typename ValueType>
JsonNumberRetriever<ValueType>::JsonNumberRetriever(const QString& prefix, const QString& path, const QString& suffix, bool showSign, const QString& varName)
    : super(prefix, path, suffix, varName), m_ShowSign(showSign)
{
}

template<typename ValueType>
QString JsonNumberRetriever<ValueType>::ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const
{
    QVariant variant = GetJsonValue(jsonValue, GetPath(), varMap).toVariant();
    if (!variant.isNull() && variant.canConvert((QMetaType)QMetaType::Double))
    {
        const QString& asString = variant.toString();
        if (!asString.isEmpty())
        {
            ValueType value = (ValueType)asString.toDouble();
            QString sign = (m_ShowSign && value > 0) ? "+" : "";

            return sign + QString::number(value);
        }
    }

    return "";
}

class JSonIntRetriever : public JsonNumberRetriever<int>
{
public:
    JSonIntRetriever(const QString& prefix, const QString& path, const QString& suffix, bool showSign, const QString& varName)
        : JsonNumberRetriever(prefix, path, suffix, showSign, varName) {}
};

class JSonFloatRetriever : public JsonNumberRetriever<float>
{
public:
    JSonFloatRetriever(const QString& prefix, const QString& path, const QString& suffix, bool showSign, const QString& varName)
        : JsonNumberRetriever(prefix, path, suffix, showSign, varName) {}
};
