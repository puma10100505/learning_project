#include <Windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "CommonDefines.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "perfetto.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

/**
 * @brief 下面的Trace宏定义要放到PerfettoTracingHelper.h之前
 * 
 */
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category(TraceEventCategory(D3DMath)).SetDescription("D3D Math perf")  
);

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

/** */

#include "PerfettoTracingHelper.h"

ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v)
{
    XMFLOAT4 dst;
    XMStoreFloat4(&dst, v);

    os << "(" << dst.x << ", " << dst.y << ", " << dst.z << ", " << dst.w << ")";

    return os;
}

ostream& XM_CALLCONV operator << (ostream& os, FXMMATRIX m)
{
    for (int i = 0; i < 4; ++i)
    {
        os << XMVectorGetX(m.r[i]) << "\t";
        os << XMVectorGetY(m.r[i]) << "\t";
        os << XMVectorGetZ(m.r[i]) << "\t";
        os << XMVectorGetW(m.r[i]);
        os << std::endl;
    }

    return os;
}

int main(int argc, char** argv)
{
    auto TracingSession = StartPerfettoTracing("track_event", "TestPerf.ptrace", "D3DMatrixPerf");

    plog::init(plog::debug, (DefaultLogDirectory + "D3DMathMatrix.log").c_str());

    if (XMVerifyCPUSupport() == false)
    {
        PLOGD << "CPU does not support SSE2";
        return EXIT_FAILURE;
    }

    std::cout << "Hello " << std::endl;

    TRACE_EVENT_BEGIN(TraceEventCategory(D3DMath), TraceEventName(TotalMath));
    XMMATRIX OriginalMat(
        1.f, 0.f, 0.f, 0.f, 
        0.f, 2.f, 0.f, 0.f, 
        0.f, 0.f, 4.f, 0.f, 
        1.f, 2.f, 3.f, 1.f);

    XMMATRIX Identity = XMMatrixIdentity();

    std::cout << OriginalMat << std::endl;
    std::cout << Identity << std::endl;

    {
        TRACE_EVENT(TraceEventCategory(D3DMath), TraceEventName(MatrixScaling));
        XMMATRIX ScaleMat = XMMatrixScaling(3.f, 3.f, 3.f);
        std::cout << OriginalMat * ScaleMat << std::endl;
    }

    {
        TRACE_EVENT(TraceEventCategory(D3DMath), TraceEventName(MatrixRot));
        XMMATRIX RotXMat = XMMatrixRotationX(90.f);
        std::cout << RotXMat << std::endl;
    }

    XMFLOAT4 OriginalVec = {1.f, 0.f, 0.f, 0.f};

    {
        TRACE_EVENT(TraceEventCategory(D3DMath), TraceEventName(LoadFloat));
        XMVECTOR Vec = XMLoadFloat4(&OriginalVec);
    }

    TRACE_EVENT_END(TraceEventCategory(D3DMath));

    StopPerfettoTracing(std::move(TracingSession));

    return EXIT_SUCCESS;
}