#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer() : SecondsPerCount(0), mDeltaTime(-1.f), BaseTime(0), 
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
        mDeltaTime = 0.f;
        return;
    }

    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    CurrTime = currTime;

    mDeltaTime = (CurrTime - PrevTime) * SecondsPerCount;

    PrevTime = CurrTime;

    if (mDeltaTime < 0.f)
    {
        mDeltaTime = 0.f;
    }   
}

float GameTimer::DeltaTime() const 
{
    return (float)mDeltaTime;
}