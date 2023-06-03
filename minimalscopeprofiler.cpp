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

#include "minimalscopeprofiler.h"

#include <QDebug>

MinimalScopeProfiler::MinimalScopeProfiler(const std::string& name)
    : m_Name(name)
{
    m_Start = QTime::currentTime();
}

MinimalScopeProfiler::~MinimalScopeProfiler()
{
    qint64 duration = m_Start.msecsTo(QTime::currentTime());
    qDebug() << (m_Name + " : " + std::to_string(duration) + " milliseconds.").c_str();
}
