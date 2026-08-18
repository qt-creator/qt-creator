// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertracebackend.h"

#include "profilertr.h"

namespace Profiler::Internal {

ProfilerTraceBackend::ProfilerTraceBackend(QObject *parent)
    : QObject(parent)
{}

ProfilerTraceBackend::~ProfilerTraceBackend() = default;

Utils::Result<> ProfilerTraceBackend::save(const Utils::FilePath &path)
{
    Q_UNUSED(path)
    return Utils::ResultError(Tr::tr("This trace format cannot be saved."));
}

} // namespace Profiler::Internal
