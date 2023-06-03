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

#include <QString>

class QJsonValue;
class QJsonObject;
class QJSValue;

class JSonRetriever
{
public:
    JSonRetriever(const QString& prefix, const QString& path, const QString& suffix, const QString& varName);

    QJsonValue GetJsonValue(const QJsonValue& jsonValue, const QString& fullPath, QMap<QString, QJSValue>* varMap = nullptr) const;

    void SetItemNamePath(const QString& path);
    const QString& GetItemNamePath();

    QString ToString(const QJsonObject& jsonObject) const;
    QString ToString(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap = nullptr) const;

    bool IsValid(const QJsonObject& jsonObject, QMap<QString, QJSValue>* varMap = nullptr) const;
    bool IsValid(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap = nullptr) const;

    virtual void ToObjectList(const QJsonValue& /*jsonValue*/, QList<QJsonObject>& /*objectList*/);

protected:
    const QString& GetPath() const;
    virtual QString ToStringInternal(const QJsonValue& jsonValue, QMap<QString, QJSValue>* varMap) const = 0;
    virtual bool IsValidInternal(const QJsonValue& /*jsonValue*/, QMap<QString, QJSValue>* varMap = nullptr) const;

private:
    QString m_Prefix;
    QString m_Path;
    QString m_Suffix;
    QString m_VarName;
    QString m_ItemNamePath;
};
