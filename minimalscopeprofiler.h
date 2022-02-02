#pragma once

#include <string>
#include <QTime>

class MinimalScopeProfiler
{
public:
    MinimalScopeProfiler(const std::string& name);
    ~MinimalScopeProfiler();

private:
    std::string m_Name;
    QTime m_Start;
};

#ifdef QT_DEBUG
//#define profile() MinimalScopeProfiler minimalScopeProfiler(__func__);
#define profile(name) void();
#else
//#define profile() MinimalScopeProfiler minimalScopeProfiler(__func__);
#define profile() void();
#endif

#ifdef QT_DEBUG
//#define profileName(name) MinimalScopeProfiler minimalScopeProfiler(name);
#define profileName(name) void();
#else
//#define profileName(name) MinimalScopeProfiler minimalScopeProfiler(name);
#define profileName(name) void();
#endif
