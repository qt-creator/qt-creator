// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include "sampler.h"

namespace QmlProfiler::Internal {

// Attaches to the target process and records the NT Kernel Logger's sampled
// profile (with call stacks) of it, then writes the collected samples to a
// temporary Common Trace Format directory (loadable via
// Window::loadTraceFile). ETW samples the threads that are on a CPU at each
// tick; blocked threads produce no samples — unlike the macOS backend, which
// records those too, with running=false.
//
// Samples until `stop` becomes true, so it must run on a worker thread while the
// GUI thread owns `stop`. During post-processing (symbolication + writing) it
// publishes a 0..100 percentage to `progressPercent` if non-null, which the GUI
// thread may poll.
//
// This is one of three sampler backends: the Windows ETW sampler (this file),
// the macOS call-stack sampler (macsampler.cpp), and the Linux Perf Sampler
// (perfsampler.cpp). All three produce SampleTraceData that is displayed by the
// same generic SamplerViewManager.
//
// This file is only part of the build on Windows (see CMakeLists.txt); on
// other platforms callstacksampler.cpp supplies a fallback that fails with an
// explanatory error.
PROFILER_EXPORT Utils::Result<Utils::FilePath> recordSampleTrace(
    const SamplerOptions &opts,
    const std::atomic_bool &stop,
    std::atomic<int> *progressPercent = nullptr);

} // namespace QmlProfiler::Internal
