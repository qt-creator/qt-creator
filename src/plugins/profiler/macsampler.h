// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include "sampler.h"

namespace Profiler::Internal {

#ifdef Q_OS_MACOS
// Attaches to the target process, periodically suspends it and walks the
// call stack of every thread, then writes the collected samples to a temporary
// Common Trace Format directory (loadable via Window::loadTraceFile).
//
// Samples until `stop` becomes true, so it must run on a worker thread while the
// GUI thread owns `stop`. During post-processing (symbolication + writing) it
// reports a 0..100 percentage through `reportProgress`, from that same worker
// thread, whenever the whole percent changes.
//
// macOS only: on other platforms this fails with an explanatory error.
PROFILER_EXPORT Utils::Result<Utils::FilePath> recordSampleTrace(
    const SamplerOptions &opts,
    const std::atomic_bool &stop,
    const std::function<void(int)> &reportProgress = {});

// Whether this process may sample another one at all. task_for_pid() is only
// granted to a binary signed with com.apple.security.cs.debugger, or to root,
// and finding that out at attach time means finding it out after a recording.
PROFILER_EXPORT bool canSampleOtherProcesses();
#endif

} // namespace Profiler::Internal
