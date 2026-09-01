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
// Samples until `isCanceled` answers true -- the task's promise, asked from the
// worker thread this runs on. Cancelling ends the capture; what was sampled
// until then is still symbolized and written, which is what the Stop button
// wants. During post-processing it reports a 0..100 percentage through
// `reportProgress`, from that same thread, whenever the whole percent changes.
//
// macOS only: on other platforms this fails with an explanatory error.
PROFILER_EXPORT Utils::Result<Utils::FilePath> recordSampleTrace(
    const SamplerOptions &opts,
    const std::function<bool()> &isCanceled,
    const std::function<void(int)> &reportProgress = {});

// Whether this process may sample another one at all. task_for_pid() is only
// granted to a binary signed with com.apple.security.cs.debugger, or to root,
// and finding that out at attach time means finding it out after a recording.
PROFILER_EXPORT bool canSampleOtherProcesses();
#endif

} // namespace Profiler::Internal
