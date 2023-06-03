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

#include "JsonRetrievers/JsonRetriever.h"

#include <QString>
#include <QJsonValue>
#include <QJsonArray>
#include <QJSValue>
#include <QRegExp>

#include <sstream>

JSonRetriever::JSonRetriever(const QString& prefix, const QString& path, const QString& suffix, const QString& varName)
        : m_Prefix(prefix), m_Path(path), m_Suffix(suffix), m_VarName(varName)
{
}

QJsonValue JSonRetriever::GetJsonValue(const QJsonValue& jsonValue, const QString& fullPath, QMap<QString, QJSValue>* varMap /* = nullptr */) const
{
    QJsonValue value = jsonValue;

    if (varMap != nullptr && !m_VarName.isEmpty())
    {
        if (varMap->contains(m_VarName))
        {
            value = (*varMap)[m_VarName].toString();
        }
    }
    else if (!fullPath.isEmpty())
    {
        static QRegExp isArrayIndex("\\[\\d*\\]");

        std::istringstream stringStream(fullPath.toStdString());

        std::string localPath;
        while (std::getline(stringStream, localPath, '.'))
        {
            const char* path = localPath.c_str();

            if (!value.isNull())
            {
                if (isArrayIndex.exactMatch(path))
                {
                    int index = std::atoi(localPath.substr(1, localPath.size() - 1).c_str());
                    const QJsonArray& array = value.toArray();
                    if (array.size() > index)
                    {
                        value = array[index];
                    }
                }
                else
                {
                    value = value[path];
                }
            }
        }
    }

    return value;
}

void JSonRetriever::SetItemNamePath(const QString& path)
{
    m_ItemNamePath = path;
}

const QString& JSonRetriever::GetItemNamePath()
{
    return m_ItemNamePath;
}

QString JSonRetriever::ToString(const QJsonObject& jsonObject) const
{
    return ToString(QJsonValue(jsonObject));
}

QString JSonRetriever::ToString(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap /* = nullptr */) const
{
    const QString& result = ToStringInternal(jsonValue, varMap);
    return result.isEmpty() ? "" : m_Prefix + result + m_Suffix;
}

bool JSonRetriever::IsValid(const QJsonObject& jsonObject, QMap<QString, QJSValue>* varMap /* = nullptr */) const
{
    return IsValid(QJsonValue(jsonObject), varMap);
}

bool JSonRetriever::IsValid(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap /* = nullptr */) const
{
    return IsValidInternal(jsonValue, varMap);
}

void JSonRetriever::ToObjectList(const QJsonValue& /*jsonValue*/, QList<QJsonObject>& /*objectList*/)
{
}

const QString& JSonRetriever::GetPath() const
{
    return m_Path;
}

bool JSonRetriever::IsValidInternal(const QJsonValue& /*jsonValue*/, QMap<QString, QJSValue>* /* varMap  = nullptr */) const
{
    return true;
}
