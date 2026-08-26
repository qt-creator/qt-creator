// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboardstats.h"

#include "qmlprofilermodelmanager.h"

using namespace QmlDebug;

namespace Profiler::Internal {

QmlProfilerDashboardStats::QmlProfilerDashboardStats(QmlProfilerModelManager *manager,
                                                     QObject *parent)
    : QObject(parent)
{
    manager->qmlLoaders.append({
        1ULL << ProfileAnimations,
        [this](const QmlEvent &event, const QmlEventType &type) { loadEvent(event, type); }});
    manager->registerFeatures(1ULL << ProfileAnimations,
                              [this] { reset(); },
                              [this] { emit changed(); },
                              [this] { reset(); emit changed(); });
}

void QmlProfilerDashboardStats::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    Q_UNUSED(type)

    // Only the GUI thread's animation frames are what ends up on screen.
    if (static_cast<AnimationThread>(event.number<qint32>(2)) != GuiThread)
        return;

    const qint32 framerate = event.number<qint32>(0); // frames per second
    const double frameTimeMs = framerate > 0 ? 1000.0 / framerate : 0.0;
    if (frameTimeMs <= kOnTargetFrameTimeMs)
        ++m_framesOnTarget;
    else if (frameTimeMs <= kNearTargetFrameTimeMs)
        ++m_framesNearTarget;
    else
        ++m_framesFailed;

    if (frameTimeMs > kStutterFrameTimeMs)
        ++m_framesStuttering;
}

void QmlProfilerDashboardStats::reset()
{
    m_framesOnTarget = 0;
    m_framesNearTarget = 0;
    m_framesFailed = 0;
    m_framesStuttering = 0;
}

int QmlProfilerDashboardStats::framesOnTarget() const
{
    return m_framesOnTarget;
}

int QmlProfilerDashboardStats::framesNearTarget() const
{
    return m_framesNearTarget;
}

int QmlProfilerDashboardStats::framesFailed() const
{
    return m_framesFailed;
}

int QmlProfilerDashboardStats::framesStuttering() const
{
    return m_framesStuttering;
}

int QmlProfilerDashboardStats::framesTotal() const
{
    return m_framesOnTarget + m_framesNearTarget + m_framesFailed;
}

int QmlProfilerDashboardStats::onTargetPercent() const
{
    const int total = framesTotal();
    return total > 0 ? (m_framesOnTarget * 100) / total : 0;
}

int QmlProfilerDashboardStats::stutterFreePercent() const
{
    const int total = framesTotal();
    return total > 0 ? (total - m_framesStuttering) * 100 / total : 100;
}

int QmlProfilerDashboardStats::uiResponsivenessPercent() const
{
    const int total = framesTotal();
    // Approximation: near-target frames count half as much towards
    // responsiveness as on-target ones.
    return total > 0 ? (m_framesOnTarget * 100 + m_framesNearTarget * 50) / total : 100;
}

int QmlProfilerDashboardStats::p99Percent() const
{
    const int total = framesTotal();
    const int failedPercent = total > 0 ? m_framesFailed * 100 / total : 0;
    // Approximation: a P99 stand-in that weighs failed frames three times as
    // heavily, since a real 99th-percentile metric is dominated by the tail.
    return qMax(0, 100 - failedPercent * 3);
}

int QmlProfilerDashboardStats::startupSpeedPercent() const
{
    // Approximation: treat the distance from "all frames on target" as
    // leftover warm-up churn; the closer to it, the faster startup looks.
    return qMax(0, onTargetPercent() - (100 - stutterFreePercent()));
}

int QmlProfilerDashboardStats::overallPercent() const
{
    // Approximation: overall rating is a plain average of the categories above.
    return (onTargetPercent() + stutterFreePercent() + uiResponsivenessPercent()
            + p99Percent() + startupSpeedPercent()) / 5;
}

} // namespace Profiler::Internal
