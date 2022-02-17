#include "perfetto_helper.h"
#include <perfetto.h>

std::unique_ptr<PerfettoTraceHelper> PerfettoTraceHelper::InternalInst = std::make_unique<PerfettoTraceHelper>();

PerfettoTraceHelper* PerfettoTraceHelper::GetInstance()
{
    // if (InternalInst.get() == nullptr)
    // {
    //     InternalInst = std::make_unique<PerfettoTraceHelper>();
    // }

    return InternalInst.get();
}

PerfettoTraceHelper* PerfettoTraceHelper::Init()
{
    // perfetto::TracingInitArgs args;

    // args.backends = perfetto::kInProcessBackend;
    // perfetto::Tracing::Initialize(args);
    // perfetto::TrackEvent::Register();

    return InternalInst.get();
}

void PerfettoTraceHelper::StartTracing(const std::string& InDSName, const std::string& InFileName)
{
    // perfetto::TraceConfig cfg;
    // cfg.add_buffers()->set_size_kb(1024);

    // auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    // ds_cfg->set_name(InDSName.c_str());

    // FILE* TraceFileStream = fopen(InFileName.c_str(), "w+");

    // if (TraceFileStream == nullptr)
    // {
    //     return;
    // }

    // TracingSessionPtr = perfetto::Tracing::NewTrace();
    // TracingSessionPtr->Setup(cfg, _fileno(TraceFileStream));
    // TracingSessionPtr->StartBlocking();
}

void PerfettoTraceHelper::StopTracing()
{
    // perfetto::TrackEvent::Flush();
    // TracingSessionPtr->StopBlocking();

    // std::vector<char> trace_data(TracingSessionPtr->ReadTraceBlocking());

    // std::ofstream output;
    // output.open("D3DMathMatrix.ptrace", std::ios::out | std::ios::binary);
    // output.write(&trace_data[0], trace_data.size());
    
    // output.close();
}