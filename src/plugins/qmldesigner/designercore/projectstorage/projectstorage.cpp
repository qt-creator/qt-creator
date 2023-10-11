// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorage.h"

#include <sqlitedatabase.h>

namespace QmlDesigner {

#ifdef ENABLE_PROJECT_STORAGE_TRACING
namespace {
NanotraceHR::TraceFile traceFile{"projectstorage.json"};

thread_local auto eventQueueData = NanotraceHR::makeEventQueueData<NanotraceHR::StringViewTraceEvent, 1000>(
    traceFile);
thread_local NanotraceHR::EventQueue<NanotraceHR::StringViewTraceEvent> eventQueue = eventQueueData;
} // namespace

NanotraceHR::Category<NanotraceHR::StringViewTraceEvent> projectStorageCategory{"project storage"_t,
                                                                                eventQueue};
#endif
} // namespace QmlDesigner

template class QmlDesigner::ProjectStorage<Sqlite::Database>;
