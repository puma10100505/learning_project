#pragma once

#include <stdio.h>
#include <string>
#include <memory>
#include <perfetto.h>

class PerfettoTraceHelper final
{
public:
    PerfettoTraceHelper(){}

public:
    static class PerfettoTraceHelper* GetInstance();
    ~PerfettoTraceHelper(){}

public:
    PerfettoTraceHelper* Init();

    void StartTracing(const std::string& InDSName, const std::string& InFileName);
    void StopTracing();

private:
    std::unique_ptr<perfetto::TracingSession> TracingSessionPtr;
    static std::unique_ptr<class PerfettoTraceHelper> InternalInst;
    std::string TraceFileName;
};