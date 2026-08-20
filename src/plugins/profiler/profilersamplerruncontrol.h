// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

namespace Profiler::Internal {

// The run mode that records the startup project with the backend `backendId`
// (see SamplerIds). Every backend has one, so a recording of the project gets
// the run configuration's command line, environment and device -- and Qt
// Creator's run machinery, rather than the backend, decides what can be
// profiled where.
Utils::Id samplerRunMode(Utils::Id backendId);

void setupProfilerSamplerRunning();

} // namespace Profiler::Internal
