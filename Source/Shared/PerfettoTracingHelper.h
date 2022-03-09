#pragma once

#include <stdio.h>
#include <string>
#include <memory>
#include "perfetto.h"

static std::unique_ptr<perfetto::TracingSession> StartPerfettoTracing(const std::string& InDataSourceName, 
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

    std::unique_ptr<perfetto::TracingSession> tracing_session = 
        perfetto::Tracing::NewTrace(perfetto::kInProcessBackend);
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

static std::unique_ptr<perfetto::TracingSession> StartPerfettoTracing2(const std::string& InDataSourceName, const std::string& InProcessName)
{
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();    
    
    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024);

    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name(InDataSourceName.c_str());

    // FILE* TraceFileStream = fopen(InTraceFilePath.c_str(), "wb+");

    std::unique_ptr<perfetto::TracingSession> tracing_session = perfetto::Tracing::NewTrace(perfetto::kInProcessBackend);
    // if (TraceFileStream == nullptr)
    // {
    //     return tracing_session;
    // }

    tracing_session->Setup(cfg);
    tracing_session->StartBlocking();

    perfetto::ProcessTrack process_track = perfetto::ProcessTrack::Current();
    perfetto::protos::gen::TrackDescriptor desc = process_track.Serialize();
    desc.mutable_process()->set_process_name(InProcessName.c_str());
    perfetto::TrackEvent::SetTrackDescriptor(process_track, desc);

    return tracing_session;
}

static void StopPerfettoTracing(std::unique_ptr<perfetto::TracingSession> Session)
{
    if (Session.get())
    {
        perfetto::TrackEvent::Flush();
        Session->StopBlocking();
    }
}

static void StopPerfettoTracing2(std::unique_ptr<perfetto::TracingSession> Session, const std::string& InTracingFile)
{
    // Make sure the last event is closed for this example.
    perfetto::TrackEvent::Flush();

    // Stop tracing and read the trace data.
    Session->StopBlocking();
    std::vector<char> trace_data(Session->ReadTraceBlocking());

    // Write the result into a file.
    // Note: To save memory with longer traces, you can tell Perfetto to write
    // directly into a file by passing a file descriptor into Setup() above.
    std::ofstream output;
    output.open(InTracingFile.c_str(), std::ios::out | std::ios::binary);
    output.write(&trace_data[0], trace_data.size());
    output.close();
}