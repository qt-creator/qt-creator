// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracefile.h"

#include "nanotracehr.h"

namespace NanotraceHR {

#ifdef ENABLE_TRACING_FILE

static std::unique_ptr<EnabledTraceFile> &getTraceFile()
{
    static auto traceFile = std::make_unique<EnabledTraceFile>("tracing.json");

    return traceFile;
}

EnabledTraceFile &traceFile()
{
    return *getTraceFile().get();
}

void resetTraceFilePointer()
{
    getTraceFile().reset();
}

#endif

} // namespace NanotraceHR
