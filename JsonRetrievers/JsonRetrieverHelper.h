// Morfal is a Virtual Pen and Paper (VPP): a Game Master's toolbox designed for those who play Role Playing Games with pen and paper only,
// as opposed to Virtual Table Tops (VTT) which are made to handle tactical gameplay (tokens, square/hexagons, area of effect, etc...).
// Copyright (C) 2022  akujiki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <QApplication>
#include <QObject>
#include <QList>
#include <QXmlStreamReader>
#include <QJSEngine>
#include <QJSEngine>
#include <QMessageBox>
#include <QFile>

#include "utils.h"
#include "xmlscript.h"

class JSonIntRetriever;
class JSonFloatRetriever;
class JSonStringRetriever;
class JSonMapRetriever;
class JSonArrayRetriever;
class JSonConditionRetriever;
class JSonRetriever;

class JsonRetrieverHelper : public QObject
{
    Q_OBJECT

public:
    JsonRetrieverHelper() = default;

    using XmlScriptBank = QMap<QString, QMap<QString, XmlScript>>;

    enum class XmlTypes : short
    {
        jsonInt,
        jsonFloat,
        jsonString,
        map,
        array,
        condition,
        MathScript
    };

    Q_ENUM(XmlTypes)

    template<typename QEnum>
    static bool TryReadElementAsType(QXmlStreamReader& reader, QEnum& qEnum, bool& foundError)
    {
        if (reader.isEndElement())
        {
            return false;
        }

        if (reader.hasError())
        {
            QString errorMessage(reader.errorString() + "\nat line " + QString::number(reader.lineNumber()) + ", column "+ QString::number(reader.columnNumber()));

            QFile* file = qobject_cast<QFile*>(reader.device());
            QString fileInformation = (file != nullptr) ? "In file " + file->fileName() + "\n" : "";
            QMessageBox::information(QApplication::activeWindow(), "Invalid xml", fileInformation + errorMessage);
            foundError = true;

            return false;
        }

        bool matchesEnum;
        const QString& elementName = reader.name().toString();
        qEnum = Utils::StringToQtEnum<QEnum>(elementName, matchesEnum);
        return matchesEnum;
    }

    static JSonIntRetriever* BuildJsonInt(QXmlStreamReader& reader);
    static JSonFloatRetriever* BuildJsonFloat(QXmlStreamReader& reader);
    static JSonStringRetriever* BuildJsonString(QXmlStreamReader& reader);
    static JSonMapRetriever* BuildJsonMap(QXmlStreamReader& reader);
    static JSonArrayRetriever* BuildJsonArray(QXmlStreamReader &reader);
    static JSonConditionRetriever* BuildCondition(QXmlStreamReader &reader);

    static void BuildRetrieverList(QXmlStreamReader &reader, QList<JSonRetriever*>& list);

    using FeedFunction = std::function<void(const QJsonObject& jsonData)>;

    static void RegisterMathScript(QXmlStreamReader& reader, QJSEngine& jsEngine, QMap<QString, QJSValue>& scriptResult, QList<FeedFunction>& feedFunctions);
    static void RegisterXmlScript(QXmlStreamReader& reader, const std::map<QString, QString>& categoryToScriptSetting, XmlScriptBank& xmlScriptBank, QMap<QString, QJSValue>& scriptResult, QList<FeedFunction>& feedFunctions);
};
