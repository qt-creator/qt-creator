// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlitetracing.h"

namespace Sqlite {

TraceFile &traceFile()
{
    static TraceFile traceFile{"tracing.json"};

    return traceFile;
}

namespace {

thread_local NanotraceHR::StringViewWithStringArgumentsEventQueue<sqliteTracingStatus()> eventQueue(
    traceFile());
thread_local NanotraceHR::EventQueueWithoutArguments<sqliteTracingStatus()> eventQueueWithoutArguments(
    traceFile());

} // namespace

NanotraceHR::StringViewWithStringArgumentsCategory<sqliteTracingStatus()> &sqliteLowLevelCategory()
{
    thread_local NanotraceHR::StringViewWithStringArgumentsCategory<sqliteTracingStatus()>
        sqliteLowLevelCategory_{"sqlite low level",
                                eventQueue,
                                eventQueueWithoutArguments,
                                sqliteLowLevelCategory};
    return sqliteLowLevelCategory_;
}

NanotraceHR::StringViewWithStringArgumentsCategory<sqliteTracingStatus()> &sqliteHighLevelCategory()
{
    thread_local NanotraceHR::StringViewWithStringArgumentsCategory<sqliteTracingStatus()>
        sqliteHighLevelCategory_{"sqlite high level",
                                 eventQueue,
                                 eventQueueWithoutArguments,
                                 sqliteHighLevelCategory};

    return sqliteHighLevelCategory_;
}

} // namespace Sqlite
