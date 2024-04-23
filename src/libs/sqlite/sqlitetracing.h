// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqliteglobal.h"

#include <nanotrace/nanotracehr.h>

namespace Sqlite {
using namespace NanotraceHR::Literals;

constexpr NanotraceHR::Tracing sqliteTracingStatus()
{
#ifdef ENABLE_SQLITE_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using TraceFile = NanotraceHR::TraceFile<sqliteTracingStatus()>;

SQLITE_EXPORT TraceFile &traceFile();

NanotraceHR::StringViewWithStringArgumentsCategory<sqliteTracingStatus()> &sqliteLowLevelCategory();

SQLITE_EXPORT NanotraceHR::StringViewWithStringArgumentsCategory<sqliteTracingStatus()> &
sqliteHighLevelCategory();

} // namespace Sqlite
