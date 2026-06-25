// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"
#include "sampler.h"

#include <QtTaskTree/QTaskTree>

#include <memory>

namespace QmlProfiler::Internal {

// Composes process launching around a backend's capture item. When
// session->launchCommand is set, it launches that command, runs `capture` once
// the process has started, and terminates the process when capture finishes.
// If the target exits on its own before capture is stopped, session->stop is set
// so the in-progress trace is still written. When there is no launch command
// (attach or connect), this is just Group{capture}.
//
// Shared by all backends so the process plumbing lives in one place.
PROFILER_EXPORT QtTaskTree::Group launchThenCapture(
    const std::shared_ptr<RecordingSession> &session,
    const QtTaskTree::ExecutableItem &capture);

} // namespace QmlProfiler::Internal
