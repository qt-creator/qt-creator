// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignertracing.h"

#include <sqlitebasestatement.h>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

namespace Tracing {

#ifdef ENABLE_QMLDESIGNER_TRACING

namespace {

using TraceFile = NanotraceHR::EnabledTraceFile;

auto traceFile()
{
#  ifdef ENABLE_SQLITE_TRACING
    return Sqlite::traceFile();
#  else
    static auto traceFile = std::make_shared<TraceFile>("tracing.json");
    return traceFile;
#  endif
}
} // namespace

EventQueueWithStringArguments &eventQueueWithStringArguments()
{
    thread_local NanotraceHR::EnabledEventQueueWithArguments eventQueue(traceFile());

    return eventQueue;
}

EventQueueWithoutArguments &eventQueueWithoutArguments()
{
    thread_local NanotraceHR::EnabledEventQueueWithoutArguments eventQueue(traceFile());

    return eventQueue;
}

#endif

} // namespace Tracing

namespace ModelTracing {

#ifdef ENABLE_MODEL_TRACING

namespace {

thread_local Category category_{"model",
                                Tracing::eventQueueWithStringArguments(),
                                Tracing::eventQueueWithoutArguments(),
                                category};

} // namespace

Category &category()
{
    return category_;
}

#endif

} // namespace ModelTracing

} // namespace QmlDesigner
