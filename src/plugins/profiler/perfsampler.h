// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

#include "perfsettings.h"

#include <memory>

namespace QmlProfiler::Internal {

// Settings for the Linux perf-based sampler. Perf-specific configuration
// (events, call-graph mode, frequency, extra arguments) is not reimplemented
// here: it is the same Profiler::PerfSettings used by the IDE's CPU Usage
// analyzer (see perfsettings.h), embedded and reused verbatim, including its
// existing perfRecordArguments()/createPerfConfigWidget(). Only the
// attach-to-a-running-process toggle common to every sampler backend (see
// CallStackSamplerSettings) is added here.
class PerfSamplerSettings : public SamplerSettings
{
    Q_OBJECT

public:
    PerfSamplerSettings();

    Utils::Result<std::shared_ptr<RecordingSession>> createSession() const override;

    void readSettings() override;
    void writeSettings() const override;

    Profiler::PerfSettings perfSettings;

    Utils::BoolAspect attach{this}; // Attach to a running process instead of launching.

private:
    // The process chosen by the picker; used when attach is enabled.
    qint64 m_pickedPid = 0;
    QString m_pickedName;
};

// Records a trace by running "perf record --pid <pid> -o -" against the target
// and piping its output through perfparser -- the same unwinding and
// symbolication engine the IDE's CPU Usage analyzer uses (see
// perfprofilerruncontrol.cpp) -- decoding the result directly into a
// SampleTraceData so it can be shown by the same generic SamplerViewManager as
// the macOS call-stack sampler (see macsampler.cpp).
//
// Linux only.
class PROFILER_EXPORT PerfSampler : public Sampler
{
public:
    PerfSampler();
    ~PerfSampler() override;

    QString displayName() const override;
    bool isAvailable(QString *error = nullptr) const override;
    QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const override;

    SamplerSettings *settings() const override;

private:
    std::unique_ptr<PerfSamplerSettings> m_settings;
};

} // namespace QmlProfiler::Internal
