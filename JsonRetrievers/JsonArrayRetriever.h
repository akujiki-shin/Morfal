#pragma once

#include "JsonRetrievers/JsonRetriever.h"

#include <QList>

class QXmlStreamReader;

class JSonArrayRetriever : public JSonRetriever
{
private:
    using super = JSonRetriever;

public:
    JSonArrayRetriever(QXmlStreamReader& reader, const QString& path, const QString& separator, const QString& prefix = "", const QString& suffix = "", const QString& varName = "");

    void ToObjectList(const QJsonValue& jsonValue, QList<QJsonObject>& objectList) override;

    void SetConcatAsNumber(bool concatAsNumber);

protected:
    QString ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const override;

private:
    QString m_Separator;
    QList<JSonRetriever*> m_RetrieverList;
    bool m_ConcatAsNumber { false };
};
