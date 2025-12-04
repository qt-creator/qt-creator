// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqliteglobal.h"

#include <nanotrace/nanotracehr.h>

#include <memory>

namespace Sqlite {
using namespace NanotraceHR::Literals;

#ifdef ENABLE_SQLITE_TRACING

using TraceFile = NanotraceHR::EnabledTraceFile;

SQLITE_EXPORT std::shared_ptr<NanotraceHR::EnabledTraceFile> traceFile();

NanotraceHR::EnabledCategory &sqliteLowLevelCategory();

SQLITE_EXPORT NanotraceHR::EnabledCategory &sqliteHighLevelCategory();
#else
inline NanotraceHR::DisabledCategory sqliteLowLevelCategory()
{
    return {};
}

inline NanotraceHR::DisabledCategory sqliteHighLevelCategory()
{
    return {};
}
#endif

} // namespace Sqlite
