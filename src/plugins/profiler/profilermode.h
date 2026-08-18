// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Profiler::Internal {

// Brings the Profile mode to the front. Opening a trace calls this first, so
// the editor lands in this mode rather than sending the user to Edit.
void activateProfilerMode();

// The recorder driving the sampler backends. It outlives the mode's widget and
// the start page, so a recording survives closing either.
class ProfilerRecorder *profilerRecorder();

void setupProfilerMode();
void destroyProfilerMode();

} // namespace Profiler::Internal
