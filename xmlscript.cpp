#include "xmlscript.h"

#include "JsonRetrievers/JsonRetrieverHelper.h"

XmlScript::XmlScript()
{
}

void XmlScript::Initialize(const QDirIterator& xmlFile, QJSEngine* jsEngine)
{
    m_JSEngine = jsEngine;

    QFile file(xmlFile.filePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QXmlStreamReader reader(&file);
        while (reader.readNext() && !reader.isEndDocument())
        {
            if (!reader.isEndElement())
            {
                const QString& elementName = reader.name().toString();
                if (elementName == "MathScript")
                {
                    JsonRetrieverHelper::RegisterMathScript(reader, *m_JSEngine, m_ScriptResult, m_FeedScriptFunctions);
                }
                else if (elementName == "Out")
                {
                    m_OutVarName = reader.attributes().value("VarName").toString();
                }
            }
        }
    }
}

QJSValue XmlScript::Execute(const QJsonObject& jsonData)
{
    for (FeedFunction& script : m_FeedScriptFunctions)
    {
        script(jsonData);
    }

    return m_ScriptResult[m_OutVarName];
}
