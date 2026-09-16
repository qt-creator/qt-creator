// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

#include <memory>

namespace Profiler::Internal {

// Settings for the Qt tracepoints backend: which providers to record, and where
// to leave the trace. The target is always launched -- QTRACE_LOCATION is read
// when the traced process starts, so there is nothing to attach to.
class QtTraceSamplerSettings : public SamplerSettings
{
    Q_OBJECT

public:
    QtTraceSamplerSettings();

    Utils::Result<std::shared_ptr<RecordingSession>> createSession() const override;

    // Providers to record, separated by commas or spaces, e.g. "qtcore, qtquick".
    // Empty records every provider the target has.
    Utils::StringAspect providers{this};

    // Where the per-recording trace directory is created. Empty uses the
    // temporary location, as the other backends do.
    Utils::FilePathAspect traceDirectory{this};

    // providers(), split and cleaned up; empty when every provider is wanted.
    QStringList providerList() const;

    // Whether traceDirectory() can be recorded into, or why it cannot.
    Utils::Result<> checkTraceDirectory() const;
};

// Records a Common Trace Format trace written by the target itself: the Qt
// tracepoints backend (a Qt configured with -trace ctf) writes one channel per
// thread into the directory named by QTRACE_LOCATION. There is nothing to
// capture at this end -- the backend points the target at a directory, waits,
// and hands over what was written.
class PROFILER_EXPORT QtTraceSampler : public Sampler
{
public:
    QtTraceSampler();
    ~QtTraceSampler() override;

    Utils::Id id() const override { return SamplerIds::QtTrace; }
    QString displayName() const override;
    bool isAvailable(QString *error = nullptr) const override;
    void prepareLaunch(const std::shared_ptr<RecordingSession> &session) const override;
    QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const override;
    void completeRecording(const std::shared_ptr<RecordingSession> &session) const override;

    SamplerSettings *settings() const override;

private:
    std::unique_ptr<QtTraceSamplerSettings> m_settings;
};

// Prepares `location` for a recording of `target`: creates the directory and
// writes the session file that names the session and selects `providers` (empty
// selects every provider). Exposed for testing.
Utils::Result<> writeTraceSession(const Utils::FilePath &location, const QString &target,
                                  const QStringList &providers);

// The trace `location` holds, or an error explaining why there is none. Qt
// writes into the location itself, or into its "ust" subdirectory when a
// session file is present, so this answers with the directory that actually
// holds the trace. Exposed for testing.
Utils::Result<Utils::FilePath> collectTrace(const Utils::FilePath &location);

} // namespace Profiler::Internal
