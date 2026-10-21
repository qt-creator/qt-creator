// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboardstats.h"

#include "qmlprofilermodelmanager.h"

#include <utils/qtcassert.h>

using namespace QmlDebug;

namespace Profiler::Internal {

QmlProfilerDashboardStats::QmlProfilerDashboardStats(QmlProfilerModelManager *manager,
                                                     QObject *parent)
    : QObject(parent)
{
    const quint64 features = 1ULL << ProfileAnimations;
    manager->qmlLoaders.append({
        features,
        [this](const QmlEvent &event, const QmlEventType &type) { loadEvent(event, type); }});
    manager->registerFeatures(features,
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

    if (frameTimeMs > m_slowestFrameTimeMs) {
        m_slowestFrameTimeMs = frameTimeMs;
        m_slowestFrameTimestamp = event.timestamp();
    }
}

void QmlProfilerDashboardStats::reset()
{
    m_framesOnTarget = 0;
    m_framesNearTarget = 0;
    m_framesFailed = 0;
    m_framesStuttering = 0;
    m_slowestFrameTimestamp = -1;
    m_slowestFrameTimeMs = 0;
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

bool QmlProfilerDashboardStats::hasData() const
{
    return framesTotal() > 0;
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
    return total > 0
               ? (m_framesOnTarget * 100
                  + m_framesNearTarget * kNearTargetResponsivenessWeightPercent) / total
               : 100;
}

int QmlProfilerDashboardStats::p99Percent() const
{
    const int total = framesTotal();
    const int failedPercent = total > 0 ? m_framesFailed * 100 / total : 0;
    // Approximation: a P99 stand-in that weighs failed frames three times as
    // heavily, since a real 99th-percentile metric is dominated by the tail.
    return qMax(0, 100 - failedPercent * kP99FailedFrameWeight);
}

int QmlProfilerDashboardStats::startupSpeedPercent() const
{
    // Approximation: treat the distance from "all frames on target" as
    // leftover warm-up churn; the closer to it, the faster startup looks.
    return qMax(0, onTargetPercent() - (100 - stutterFreePercent()));
}

QList<ScoreFactor> QmlProfilerDashboardStats::scoreBreakdown() const
{
    const auto factor = [](ScoreFactor::Id id, int score) {
        ScoreFactor result;
        result.id = id;
        result.score = score;
        result.points = qRound(double(score) * ScoreFactor::MaxPoints / kMaxScore);
        return result;
    };
    const QList<ScoreFactor> factors {
        factor(ScoreFactor::UiResponsiveness, uiResponsivenessPercent()),
        factor(ScoreFactor::FrameConsistency, onTargetPercent()),
        factor(ScoreFactor::StutterPrevention, stutterFreePercent()),
        factor(ScoreFactor::P99Quality, p99Percent()),
        factor(ScoreFactor::StartupSpeed, startupSpeedPercent()),
    };
    QTC_CHECK(factors.size() == ScoreFactor::Count);
    return factors;
}

int QmlProfilerDashboardStats::overallScore() const
{
    int score = 0;
    for (const ScoreFactor &factor : scoreBreakdown())
        score += factor.points;
    return score;
}

qint64 QmlProfilerDashboardStats::slowestFrameTimestamp() const
{
    return m_slowestFrameTimestamp;
}

double QmlProfilerDashboardStats::slowestFrameTimeMs() const
{
    return m_slowestFrameTimeMs;
}

ScoreRating scoreRating(int score)
{
    if (score >= kExcellentScore)
        return ScoreRating::Excellent;
    if (score >= kGoodScore)
        return ScoreRating::Good;
    if (score >= kAlertingScore)
        return ScoreRating::Alerting;
    return ScoreRating::Poor;
}

} // namespace Profiler::Internal
