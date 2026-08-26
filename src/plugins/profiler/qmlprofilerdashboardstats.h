// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlDebug {
class QmlEvent;
class QmlEventType;
} // namespace QmlDebug

namespace Profiler::Internal {

class QmlProfilerModelManager;

// Frame time thresholds
// The ProfileAnimations trace only carries a measured, instantaneous
// framerate; the protocol has no concept of a target/display refresh rate,
// so 60 fps is assumed here.
constexpr double kDisplayRefreshRate = 60; // Frames per second
constexpr double kOnTargetFrameTimeMs = 1000 / kDisplayRefreshRate;
constexpr double kNearTargetFrameTimeMs = kOnTargetFrameTimeMs * 1.2;

// A frame slower than half the display refresh rate is a perceptible stutter.
constexpr double kStutterRefreshRateDivisor = 2;
constexpr double kStutterFps = kDisplayRefreshRate / kStutterRefreshRateDivisor;
constexpr double kStutterFrameTimeMs = 1000 / kStutterFps;

// Frame timing statistics derived from the ProfileAnimations trace events,
// for QmlProfilerDashboardView.
class QmlProfilerDashboardStats : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerDashboardStats(QmlProfilerModelManager *manager,
                                       QObject *parent = nullptr);

    int framesOnTarget() const;
    int framesNearTarget() const;
    int framesFailed() const;
    int framesStuttering() const;
    int framesTotal() const;
    int onTargetPercent() const;
    int stutterFreePercent() const;

    // Placeholder formulas: uiResponsivenessPercent, p99Percent and
    // startupSpeedPercent have no real trace data behind them yet
    // (input-latency events, per-frame percentiles, warm-up frame count,
    // respectively). Until that data exists, these are approximations
    // recombined from the frame buckets above. Replace once each metric
    // gets its own data source.
    int uiResponsivenessPercent() const;
    int p99Percent() const;
    int startupSpeedPercent() const;
    int overallPercent() const;

signals:
    void changed();

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void reset();

    int m_framesOnTarget = 0;
    int m_framesNearTarget = 0;
    int m_framesFailed = 0;
    int m_framesStuttering = 0;
};

} // namespace Profiler::Internal
