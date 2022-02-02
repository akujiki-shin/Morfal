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
