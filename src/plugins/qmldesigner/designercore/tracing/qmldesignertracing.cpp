// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignertracing.h"

namespace QmlDesigner {
namespace Tracing {

namespace {

using TraceFile = NanotraceHR::TraceFile<tracingStatus()>;

TraceFile &traceFile()
{
    static TraceFile traceFile{"qml_designer.json"};
    return traceFile;
}
} // namespace

EventQueue &eventQueue()
{
    thread_local NanotraceHR::EventQueueData<NanotraceHR::StringViewTraceEvent, 10000, tracingStatus()>
        stringViewEventQueueData(traceFile());

    return stringViewEventQueueData;
}

EventQueueWithStringArguments &eventQueueWithStringArguments()
{
    thread_local NanotraceHR::
        EventQueueData<NanotraceHR::StringViewWithStringArgumentsTraceEvent, 1000, tracingStatus()>
            stringViewWithStringArgumentsEventQueueData(traceFile());

    return stringViewWithStringArgumentsEventQueueData;
}

StringEventQueue &stringEventQueue()
{
    thread_local NanotraceHR::EventQueueData<NanotraceHR::StringTraceEvent, 1000, tracingStatus()> eventQueue(
        traceFile());

    return eventQueue;
}

} // namespace Tracing

namespace ModelTracing {
namespace {
using namespace NanotraceHR::Literals;

thread_local Category category_{"model"_t, Tracing::stringEventQueue(), category};

} // namespace

Category &category()
{
    return category_;
}

} // namespace ModelTracing
} // namespace QmlDesigner
