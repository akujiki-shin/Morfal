#pragma once

#include "JsonRetrievers/JsonRetriever.h"

#include <QList>

class QXmlStreamReader;

class JSonMapRetriever : public JSonRetriever
{
private:
    using super = JSonRetriever;

public:
    JSonMapRetriever(QXmlStreamReader& reader, const QString& path, const QString& separator, bool displayKey, const QString& prefix = "", const QString& suffix = "", const QString& varName = "");

protected:
    QString ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const override;

private:
    QString m_Separator;
    QList<JSonRetriever*> m_RetrieverList;
    bool m_DisplayKey { false };
};
