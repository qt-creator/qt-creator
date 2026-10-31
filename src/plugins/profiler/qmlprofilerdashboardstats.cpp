// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboardstats.h"

#include "qmlprofilermodelmanager.h"
#include "qmlprofilersettings.h"

#include <utils/qtcassert.h>

using namespace QmlDebug;
using namespace std::chrono;

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
                              [this] { rate(); emit changed(); },
                              [this] { reset(); emit changed(); });
}

static QmlProfilerDashboardStats::FrameTime frameDuration(int framesPerSecond)
{
    using FrameTime = QmlProfilerDashboardStats::FrameTime;
    return framesPerSecond > 0 ? FrameTime(1s) / framesPerSecond : FrameTime(0);
}

void QmlProfilerDashboardStats::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    Q_UNUSED(type)

    // Only the GUI thread's animation frames are what ends up on screen.
    if (static_cast<AnimationThread>(event.number<qint32>(2)) != GuiThread)
        return;

    ++m_frameRates[event.number<qint32>(0)]; // frames per second

    const FrameTime frameTime = frameDuration(event.number<qint32>(0));
    if (frameTime > m_slowestFrameTime) {
        m_slowestFrameTime = frameTime;
        m_slowestFrameTimestamp = event.timestamp();
    }
}

void QmlProfilerDashboardStats::rate()
{
    m_framesOnTarget = 0;
    m_framesNearTarget = 0;
    m_framesFailed = 0;
    m_framesStuttering = 0;

    for (auto it = m_frameRates.constBegin(), end = m_frameRates.constEnd(); it != end; ++it) {
        const FrameTime frameTime = frameDuration(it.key());
        if (frameTime <= onTargetFrameTime())
            m_framesOnTarget += it.value();
        else if (frameTime <= nearTargetFrameTime())
            m_framesNearTarget += it.value();
        else
            m_framesFailed += it.value();

        if (frameTime > stutterFrameTime())
            m_framesStuttering += it.value();
    }
}

void QmlProfilerDashboardStats::setTargetFrameRate(int framesPerSecond)
{
    QTC_ASSERT(framesPerSecond > 0, return);
    if (framesPerSecond == m_targetFrameRate)
        return;
    m_targetFrameRate = framesPerSecond;
    rate();
    emit changed();
}

int QmlProfilerDashboardStats::targetFrameRate() const
{
    return m_targetFrameRate;
}

QmlProfilerDashboardStats::FrameTime QmlProfilerDashboardStats::onTargetFrameTime() const
{
    return frameDuration(m_targetFrameRate);
}

QmlProfilerDashboardStats::FrameTime QmlProfilerDashboardStats::nearTargetFrameTime() const
{
    return onTargetFrameTime() * kNearTargetFrameBudgets;
}

QmlProfilerDashboardStats::FrameTime QmlProfilerDashboardStats::stutterFrameTime() const
{
    return onTargetFrameTime() * kStutterFrameBudgets;
}

void QmlProfilerDashboardStats::reset()
{
    m_frameRates.clear();
    rate();
    m_slowestFrameTimestamp = -1ns;
    m_slowestFrameTime = FrameTime(0);
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

int QmlProfilerDashboardStats::medianFrameRate() const
{
    const int middle = framesTotal() / 2;
    int seen = 0;
    for (auto it = m_frameRates.constBegin(), end = m_frameRates.constEnd(); it != end; ++it) {
        seen += it.value();
        if (seen > middle)
            return it.key();
    }
    return 0;
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

nanoseconds QmlProfilerDashboardStats::slowestFrameTimestamp() const
{
    return m_slowestFrameTimestamp;
}

QmlProfilerDashboardStats::FrameTime QmlProfilerDashboardStats::slowestFrameTime() const
{
    return m_slowestFrameTime;
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
