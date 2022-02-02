#pragma once

#include <functional>
#include <QString>
#include <QMap>
#include <QJSValue>

class QDirIterator;
class QJsonObject;

class XmlScript
{
public:
    XmlScript();
    void Initialize(const QDirIterator& xmlFile, QJSEngine* jsEngine);

    QJSValue Execute(const QJsonObject& jsonData);

private:
    using FeedFunction = std::function<void(const QJsonObject& jsonData)>;

    QList<FeedFunction> m_FeedScriptFunctions;
    QMap<QString, QJSValue> m_ScriptResult;
    QJSEngine* m_JSEngine { nullptr };

    QString m_OutVarName;
};
