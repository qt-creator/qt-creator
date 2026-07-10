// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

#include <QLatin1StringView>

#include <memory>

namespace QmlProfiler::Internal {

class QmlProfilerSampler;

// Bundle layout written by CombinedSampler and read back by CombinedTraceLoader.
inline constexpr QLatin1StringView combinedManifestName("manifest.json");
inline constexpr QLatin1StringView combinedSamplerSubdir("sampler");
inline constexpr QLatin1StringView combinedQmlFileName("qml.qtd");

// Settings for the combined backend: it always launches an executable (running
// two captures against one already-running or attached target is a later
// addition), so it reuses the base launch fields and adds the two sub-backends'
// options -- the native sampler's cadence and the QML profiler's feature set --
// which createSession() carries on the session for the sub-captures to read.
class CombinedSamplerSettings : public SamplerSettings
{
    Q_OBJECT

public:
    CombinedSamplerSettings();

    Utils::Result<std::shared_ptr<RecordingSession>> createSession() const override;

    Utils::IntegerAspect intervalUs{this}; // Native sampler cadence.

    // The QmlDebug::ProfileFeature bitmask to request, OR'd from the toggles.
    quint64 requestedFeatures() const;
    QList<Utils::BoolAspect *> featureAspects; // One per QmlDebug::ProfileFeature.
};

// Records a target with a native call-stack sampler AND the QML profiler at the
// same time. Launches the target once (as a QML debug server), then runs both
// captures in parallel against it and writes a bundle directory holding both
// traces -- the native CTF2 trace under "sampler/" and the QML ".qtd" as
// "qml.qtd" -- plus a "manifest.json". The two are later merged into one
// native-mixed report by mergeQmlIntoSamples() (see
// design-docs/native-mixed-profiler-design.md).
class PROFILER_EXPORT CombinedSampler : public Sampler
{
public:
    CombinedSampler();
    ~CombinedSampler() override;

    QString displayName() const override;
    bool isAvailable(QString *error = nullptr) const override;

    void prepareLaunch(const std::shared_ptr<RecordingSession> &session) const override;
    QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const override;

    SamplerSettings *settings() const override;

    // True if `dir` is a bundle written by this backend (has a manifest.json
    // naming a sampler trace and a QML trace). Reads only the manifest.
    static bool isCombinedTrace(const Utils::FilePath &dir);

private:
    std::unique_ptr<CombinedSamplerSettings> m_settings;
    std::unique_ptr<Sampler> m_native;        // CallStackSampler (macOS) / PerfSampler (Linux)
    std::unique_ptr<QmlProfilerSampler> m_qml; // QML debug-protocol capture
};

} // namespace QmlProfiler::Internal
