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

#include "saveutils.h"

namespace SaveUtils
{
    ParentSaveScope<QDataStream>::type LoadParent(QDataStream& /*stream*/, const QString& /*name*/)
    {
        static char dummy;
        return dummy;
    }

    QJsonArray LoadParent(QJsonObject& stream, const QString& name)
    {
        return stream[name].toArray();
    }

    QDataStream& AsStream(QDataStream& stream, LoadScopeObject<QDataStream>::type& /*dummy*/, ParentSaveScope<QDataStream>::type& /*parent*/, int /*index*/)
    {
        return stream;
    }

    QJsonObject& AsStream(QJsonObject& /*stream*/, QJsonObject& scope, QJsonArray& parent, int index)
    {
        scope = parent[index].toObject();
        return scope;
    }

    QDataStream* OpenSaveScope(QDataStream& stream)
    {
        return &stream;
    }

    QJsonObject* OpenSaveScope(QJsonObject& /*jsonObject*/)
    {
        return new QJsonObject();
    }

    void CloseSaveScope(QDataStream* /*stream*/)
    {
    }

    void CloseSaveScope(QJsonObject* jsonObject)
    {
        delete jsonObject;
    }

    void SaveParent(QDataStream& /*parentScope*/, const QString& /*dataName*/, ParentSaveScope<QDataStream>::type& /*scopeTsSave*/)
    {
    }

    void SaveParent(QJsonObject& jsonObject, const QString& dataName, QJsonArray& data)
    {
        jsonObject[dataName] = data;
    }

    void SaveScope(ParentSaveScope<QDataStream>::type& /*parentScope*/, QDataStream& /*scopeToSave*/)
    {
    }

    void SaveScope(QJsonArray& parentScope, QJsonObject& scopeToSave)
    {
        parentScope.append(scopeToSave);
    }

    template<>
    void Save(QJsonObject& jsonObject, const QString& dataName, const int& data)
    {
        jsonObject[dataName] = (int)data;
    }

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, bool& data)
    {
        data = jsonObject[dataName].toBool();
    }

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, int& data)
    {
        data = jsonObject[dataName].toInt();
    }

    template<>
    void Load(QJsonObject& jsonObject, const QString& dataName, double& data)
    {
        data = jsonObject[dataName].toDouble();
    }
}
