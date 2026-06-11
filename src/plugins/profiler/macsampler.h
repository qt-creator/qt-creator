// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <QString>

#include <atomic>

namespace QmlProfiler::Internal {

// Options for a single sampling session.
struct SamplerOptions
{
    QString processName;  // Executable basename to attach to, e.g. "Qt Creator".
    int intervalUs = 200; // Target delay between samples; 0 = as fast as possible.
};

// Attaches to the named running process, periodically suspends it and walks the
// call stack of every thread, then writes the collected samples to a temporary
// Common Trace Format directory (loadable via Window::loadTraceFile).
//
// Samples until `stop` becomes true, so it must run on a worker thread while the
// GUI thread owns `stop`. During post-processing (symbolication + writing) it
// publishes a 0..100 percentage to `progressPercent` if non-null, which the GUI
// thread may poll.
//
// macOS only: on other platforms this fails with an explanatory error.
QMLPROFILER_EXPORT Utils::Result<Utils::FilePath> recordSampleTrace(
    const SamplerOptions &opts,
    const std::atomic_bool &stop,
    std::atomic<int> *progressPercent = nullptr);

} // namespace QmlProfiler::Internal
