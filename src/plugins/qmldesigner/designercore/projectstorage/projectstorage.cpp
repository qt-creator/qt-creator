// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorage.h"

#include <sqlitedatabase.h>

namespace QmlDesigner {

namespace {
NanotraceHR::TraceFile<projectStorageTracingIsEnabled()> traceFile{"projectstorage.json"};

thread_local auto eventQueueData = NanotraceHR::makeEventQueueData<NanotraceHR::StringViewTraceEvent, 1000>(
    traceFile);
thread_local NanotraceHR::EventQueue eventQueue = eventQueueData.createEventQueue();
} // namespace

NanotraceHR::StringViewCategory<projectStorageTracingIsEnabled()> projectStorageCategory{"project storage"_t,
                                                                                         eventQueue};
} // namespace QmlDesigner

template class QmlDesigner::ProjectStorage<Sqlite::Database>;
