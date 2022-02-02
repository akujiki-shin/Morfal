#include "JsonRetrievers/JsonStringRetriever.h"

#include <QJsonValue>

JSonStringRetriever::JSonStringRetriever(const QString& prefix, const QString& path, const QString& suffix, const QString& varName /* = "" */)
        : super(prefix, path, suffix, varName)
{
}

QString JSonStringRetriever::ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const
{
    return GetJsonValue(jsonValue, GetPath(), varMap).toString();
}
