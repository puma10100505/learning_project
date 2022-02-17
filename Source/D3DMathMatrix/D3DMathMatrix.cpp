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

PERFETTO_DEFINE_CATEGORIES(
        perfetto::Category("d3d.math").SetDescription("D3D Math perf")  
    );

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

 std::unique_ptr<perfetto::TracingSession> StartPerfettoTracing(const std::string& InDataSourceName, 
            const std::string& InTraceFilePath, const std::string& InProcessName)
{
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();    
    
    
    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024);

    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name(InDataSourceName.c_str());

    FILE* TraceFileStream = fopen(InTraceFilePath.c_str(), "wb+");

    std::unique_ptr<perfetto::TracingSession> tracing_session = perfetto::Tracing::NewTrace(perfetto::kInProcessBackend);
    if (TraceFileStream == nullptr)
    {
        return tracing_session;
    }

    tracing_session->Setup(cfg, _fileno(TraceFileStream));
    tracing_session->StartBlocking();

    perfetto::ProcessTrack process_track = perfetto::ProcessTrack::Current();
    perfetto::protos::gen::TrackDescriptor desc = process_track.Serialize();
    desc.mutable_process()->set_process_name(InProcessName.c_str());
    perfetto::TrackEvent::SetTrackDescriptor(process_track, desc);
        

    return tracing_session;
}

void StopPerfettoTracing(std::unique_ptr<perfetto::TracingSession> Session)
{
    if (Session.get())
    {
        perfetto::TrackEvent::Flush();
        Session->StopBlocking();
    }
}

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

    TRACE_EVENT_BEGIN("d3d.math", "TotalMath");
    XMMATRIX OriginalMat(
        1.f, 0.f, 0.f, 0.f, 
        0.f, 2.f, 0.f, 0.f, 
        0.f, 0.f, 4.f, 0.f, 
        1.f, 2.f, 3.f, 1.f);

    XMMATRIX Identity = XMMatrixIdentity();

    std::cout << OriginalMat << std::endl;
    std::cout << Identity << std::endl;

    {
        TRACE_EVENT("d3d.math", "MatrixScaling");
        XMMATRIX ScaleMat = XMMatrixScaling(3.f, 3.f, 3.f);
        std::cout << OriginalMat * ScaleMat << std::endl;
    }

    {
        TRACE_EVENT("d3d.math", "MatrixRot");
        XMMATRIX RotXMat = XMMatrixRotationX(90.f);
        std::cout << RotXMat << std::endl;
    }

    XMFLOAT4 OriginalVec = {1.f, 0.f, 0.f, 0.f};

    {
        TRACE_EVENT("d3d.math", "LoadFloat");
        XMVECTOR Vec = XMLoadFloat4(&OriginalVec);
    }

    TRACE_EVENT_END("d3d.math");

    StopPerfettoTracing(std::move(TracingSession));

    return EXIT_SUCCESS;
}