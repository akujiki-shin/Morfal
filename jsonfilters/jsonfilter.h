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
