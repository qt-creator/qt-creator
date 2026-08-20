// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/filepath.h>

namespace Profiler::Internal {

// A trace's format determines which manager and view set render it. Chrome
// Trace Format and Common Trace Format both render through the CTF views.
// Combined shows the QML profiler views and the sampler views together, for a
// combined recording (native-mixed sampler trace plus its source QML trace).
enum class TraceFormat { Qml, Perf, Ctf, Sampler, Combined };

struct TraceFile
{
    TraceFormat format = TraceFormat::Qml;
    // The path the format's loader wants: the containing directory for the
    // directory-based formats, otherwise the file itself.
    Utils::FilePath path;
};

// Identifies the trace at `path`. A directory, or the "metadata" or
// "manifest.json" file naming one, holds a combined bundle, a recorded sampler
// trace or a Common Trace Format trace; a .json file is Chrome Trace Format, a
// .ptq file a perf trace; anything else is a QML profiler trace.
PROFILER_EXPORT TraceFile identifyTrace(const Utils::FilePath &path);

} // namespace Profiler::Internal
