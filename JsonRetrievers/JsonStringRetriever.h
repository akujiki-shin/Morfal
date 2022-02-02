#pragma once

#include "JsonRetrievers/JsonRetriever.h"

class JSonStringRetriever : public JSonRetriever
{
private:
    using super = JSonRetriever;

public:
    JSonStringRetriever(const QString& prefix, const QString& path, const QString& suffix, const QString& varName = "");

protected:
    QString ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const override;
};
