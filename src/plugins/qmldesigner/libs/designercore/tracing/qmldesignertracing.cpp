// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignertracing.h"

#include <sqlitebasestatement.h>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

namespace Tracing {

namespace {

using TraceFile = NanotraceHR::TraceFile<tracingStatus()>;

auto &traceFile()
{
    if constexpr (std::is_same_v<Sqlite::TraceFile, TraceFile>) {
        return Sqlite::traceFile();
    } else {
        static TraceFile traceFile{"tracing.json"};
        return traceFile;
    }
}
} // namespace

EventQueueWithStringArguments &eventQueueWithStringArguments()
{
    thread_local NanotraceHR::StringViewWithStringArgumentsEventQueue<tracingStatus()> eventQueue(
        traceFile());

    return eventQueue;
}

EventQueueWithoutArguments &eventQueueWithoutArguments()
{
    thread_local NanotraceHR::EventQueueWithoutArguments<tracingStatus()> eventQueue(traceFile());

    return eventQueue;
}

} // namespace Tracing

namespace ModelTracing {
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

} // namespace ModelTracing

namespace ProjectStorageTracing {

Category &projectStorageCategory()
{
    thread_local Category category{"project storage",
                                   Tracing::eventQueueWithStringArguments(),
                                   Tracing::eventQueueWithoutArguments(),
                                   projectStorageCategory};

    return category;
}

Category &projectStorageUpdaterCategory()
{
    thread_local Category category{"project storage updater",
                                   Tracing::eventQueueWithStringArguments(),
                                   Tracing::eventQueueWithoutArguments(),
                                   projectStorageCategory};

    return category;
}

} // namespace ProjectStorageTracing

namespace SourcePathStorageTracing {
Category &category()
{
    thread_local Category category_{"project storage updater",
                                    Tracing::eventQueueWithStringArguments(),
                                    Tracing::eventQueueWithoutArguments(),
                                    category};

    return category_;
}
} // namespace SourcePathStorageTracing
} // namespace QmlDesigner
