// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <QtTaskTree/QTaskTree>

#include <QString>

#include <atomic>
#include <memory>
#include <optional>

namespace QmlProfiler::Internal {

// Shared state for one recording session. The GUI thread owns it and observes
// progress while a worker thread records; the sampler task captures it. Always
// handled through a shared_ptr, never copied.
struct RecordingSession
{
    // Inputs, set before recording starts.
    int intervalUs = 200;            // Backend-specific cadence hint.
    QString processName;             // Attach-by-name target, used when pid == 0.

    // Set once a launched process is running; selects attach-by-pid instead.
    std::atomic<qint64> pid = 0;

    // Runtime control and output.
    std::atomic_bool stop = false;   // The GUI sets this to end recording.
    std::atomic<int> progress = 0;   // 0..100 post-processing percent.
    std::optional<Utils::Result<Utils::FilePath>> result; // Set on the GUI thread when done.
};

// A profiling backend: records a trace of a target process until the session is
// stopped, then writes it and reports the resulting trace directory.
//
// Implementations differ in how they capture (call-stack sampling today; a QML
// profiler or perf-based recorder could be added alongside). Each returns a
// QtTaskTree recipe so the window can compose it with process launching.
class QMLPROFILER_EXPORT Sampler
{
public:
    virtual ~Sampler() = default;

    virtual QString displayName() const = 0;

    // Whether this backend can run in the current environment; when it cannot,
    // fills *error with a human-readable reason.
    virtual bool isAvailable(QString *error = nullptr) const = 0;

    // A task that records the target described by `session`, runs off the GUI
    // thread, and stores its Result into session->result when done.
    virtual QtTaskTree::ExecutableItem recordRecipe(
        const std::shared_ptr<RecordingSession> &session) const = 0;
};

} // namespace QmlProfiler::Internal
