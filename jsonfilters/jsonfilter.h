#pragma once

#include <QString>
#include <QXmlStreamReader>
#include <QList>

class QJsonValue;
class QJsonObject;
class QJSValue;
class JSonConditionRetriever;

class JSonFilter
{
public:
    JSonFilter(QXmlStreamReader& reader);
    bool IsValid(const QJsonObject& jsonObject) const;
    bool IsValid(const QJsonValue& jsonValue) const;

private:
    QList<JSonConditionRetriever*> m_ConditionRetrievers;
};
