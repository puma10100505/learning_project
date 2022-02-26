#include "GameTimer.h"

GameTimer::GameTimer() : SecondsPerCount(0), DeltaTime(-1.f), BaseTime(0), 
    PausedTime(0), PrevTime(0), CurrTime(0), bStopped(false)
{
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    SecondsPerCount = 1.f / (double)countsPerSec;
}

void GameTimer::Tick()
{
    if (bStopped)
    {
        DeltaTime = 0.f;
        return;
    }

    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    CurrTime = currTime;

    DeltaTime = (CurrTime - PrevTime) * SecondsPerCount;

    PrevTime = CurrTime;

    if (DeltaTime < 0.f)
    {
        DeltaTime = 0,f;
    }   
}

float GameTimer::DeltaTime() const 
{
    return (float)DeltaTime;
}