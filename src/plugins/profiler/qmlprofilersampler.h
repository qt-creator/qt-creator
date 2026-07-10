// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

#include <memory>

namespace Profiler::Internal {
class QmlProfilerClientManager;
class QmlProfilerModelManager;
class QmlProfilerStateManager;
}

namespace QmlProfiler::Internal {

// Settings for the QML profiler backend: whether to connect to a running QML debug
// server (host/port) or launch the configured executable, and the set of profiler
// features to record.
class QmlProfilerSamplerSettings : public SamplerSettings
{
    Q_OBJECT

public:
    QmlProfilerSamplerSettings();

    Utils::Result<std::shared_ptr<RecordingSession>> createSession() const override;

    Utils::BoolAspect connectToServer{this}; // Connect to host/port instead of launching.
    Utils::StringAspect host{this};
    Utils::IntegerAspect port{this};

    // The QmlDebug::ProfileFeature bitmask to request, OR'd from the checked
    // feature toggles.
    quint64 requestedFeatures() const;

    // One toggle per QmlDebug::ProfileFeature, indexed by the feature value.
    QList<Utils::BoolAspect *> featureAspects;
};

// Records a real QML profiler trace (.qtd) over the QML debug protocol, either by
// launching an executable (injecting -qmljsdebugger) or by connecting to an
// already-running QML debug server. Available on every platform.
class PROFILER_EXPORT QmlProfilerSampler : public Sampler
{
public:
    QmlProfilerSampler();
    ~QmlProfilerSampler() override;

    QString displayName() const override;
    bool isAvailable(QString *error = nullptr) const override;
    void prepareLaunch(const std::shared_ptr<RecordingSession> &session) const override;
    QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const override;

    SamplerSettings *settings() const override;

private:
    std::unique_ptr<QmlProfilerSamplerSettings> m_settings;
    std::unique_ptr<Profiler::Internal::QmlProfilerModelManager> m_modelManager;
    std::unique_ptr<Profiler::Internal::QmlProfilerStateManager> m_stateManager;
    std::unique_ptr<Profiler::Internal::QmlProfilerClientManager> m_clientManager;
};

} // namespace QmlProfiler::Internal
