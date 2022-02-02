#pragma once

#include "JsonRetrievers/JsonRetriever.h"

class JSonConditionRetriever : public JSonRetriever
{
private:
    using super = JSonRetriever;

public:
    JSonConditionRetriever(const QString& conditionOperator, const QString& conditionOperand, const QString& path = "", const QString& prefix = "", const QString& suffix = "", const QString& varName = "");

protected:
    QString ToStringInternal(const QJsonValue& /*jsonValue*/, QMap<QString, QJSValue>* /*varMap*/) const override;

    virtual bool IsValidInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap = nullptr) const override;

private:
    QString m_ConditionOperator;
    QString m_ConditionOperand;
};
