// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignertracing.h"

namespace QmlDesigner {
namespace Tracing {

namespace {
using TraceFile = NanotraceHR::TraceFile<tracingStatus()>;

TraceFile traceFile{"qml_designer.json"};

thread_local NanotraceHR::EventQueueData<NanotraceHR::StringViewTraceEvent, 10000, tracingStatus()>
    strinViewEventQueueData(traceFile);
thread_local NanotraceHR::EventQueue stringViewEventQueue_ = strinViewEventQueueData.createEventQueue();

thread_local NanotraceHR::EventQueueData<NanotraceHR::StringViewWithStringArgumentsTraceEvent, 1000, tracingStatus()>
    stringViewWithStringArgumentsEventQueueData(traceFile);
thread_local NanotraceHR::EventQueue stringViewEventWithStringArgumentsQueue_ = stringViewWithStringArgumentsEventQueueData
                                                                                    .createEventQueue();
} // namespace

EventQueue &eventQueue()
{
    return stringViewEventQueue_;
}

} // namespace Tracing

namespace ModelTracing {
namespace {
using namespace NanotraceHR::Literals;

thread_local Category category_{"model"_t, Tracing::stringViewEventWithStringArgumentsQueue_};

} // namespace

Category &category()
{
    return category_;
}

} // namespace ModelTracing
} // namespace QmlDesigner
