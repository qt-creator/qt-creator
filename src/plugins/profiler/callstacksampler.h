// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

#include <memory>

namespace QmlProfiler::Internal {

// Settings for the call-stack sampler: the sampling cadence, plus an option to
// attach to an already-running process (picked via a button) instead of launching
// the configured executable.
class CallStackSamplerSettings : public SamplerSettings
{
    Q_OBJECT

public:
    CallStackSamplerSettings();

    Utils::Result<std::shared_ptr<RecordingSession>> createSession() const override;

    Utils::IntegerAspect intervalUs{this};
    Utils::BoolAspect attach{this}; // Attach to a running process instead of launching.

private:
    // The process chosen by the picker; used when attach is enabled.
    qint64 m_pickedPid = 0;
    QString m_pickedName;
};

// Records a trace by periodically suspending the target process and walking
// every thread's call stack, then symbolizing the samples (see macsampler.cpp).
// macOS only.
class PROFILER_EXPORT CallStackSampler : public Sampler
{
public:
    CallStackSampler();
    ~CallStackSampler() override;

    QString displayName() const override;
    bool isAvailable(QString *error = nullptr) const override;
    QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const override;

    SamplerSettings *settings() const override;

private:
    std::unique_ptr<CallStackSamplerSettings> m_settings;
};

} // namespace QmlProfiler::Internal
