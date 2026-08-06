// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <functional>
#include <memory>

namespace Profiler::Internal {

// Records the process selected by `session` the way winsampler.cpp does, but
// from etwcapture.exe, which Qt Creator starts elevated: the NT Kernel Logger
// needs administrator rights that Qt Creator itself does not have. Only the
// recording is elevated -- the target is launched by Qt Creator and attached to
// by process id, so it keeps running as the user.
//
// Blocks until the recording ends -- `isCanceled` answers true, or the target
// exits -- and returns the trace directory the elevated process wrote. Marks
// the session started once that process reports it is about to sample, so the
// consent prompt is not counted against the recording.
//
// This file is only part of the build on Windows (see CMakeLists.txt).
Utils::Result<Utils::FilePath> recordSampleTraceElevated(
    const std::shared_ptr<RecordingSession> &session, int intervalUs,
    const std::function<bool()> &isCanceled);

} // namespace Profiler::Internal
