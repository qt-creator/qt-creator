// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMap>
#include <QObject>

namespace QmlDebug {
class QmlEvent;
class QmlEventType;
} // namespace QmlDebug

namespace Profiler::Internal {

class QmlProfilerModelManager;

// A frame up to this many times the frame budget is near the target rather than failed.
constexpr double kNearTargetFrameBudgets = 1.2;

// Weight of a near-target frame relative to an on-target one, for
// uiResponsivenessPercent.
constexpr int kNearTargetResponsivenessWeightPercent = 50;

// p99Percent weighs failed frames this many times as heavily as a plain
// failure rate would, since a real 99th-percentile metric is dominated by
// the tail.
constexpr int kP99FailedFrameWeight = 3;

// Lowest score of each rating band. Applies to every score on the dashboard.
constexpr int kExcellentScore = 95;
constexpr int kGoodScore = 80;
constexpr int kAlertingScore = 50;

constexpr int kMaxScore = 100;

enum class ScoreRating {
    Excellent,
    Good,
    Alerting,
    Poor,
};

ScoreRating scoreRating(int score);

// One of the factors that make up QmlProfilerDashboardStats::overallScore().
class ScoreFactor
{
public:
    enum Id {
        UiResponsiveness,
        FrameConsistency,
        StutterPrevention,
        P99Quality,
        StartupSpeed,
    };

    static constexpr int Count = StartupSpeed + 1;
    static constexpr int MaxPoints = kMaxScore / Count;

    int pointsLost() const { return MaxPoints - points; }
    // Rated below "Excellent", so one of the reasons for a lower overall score.
    bool pullsScoreDown() const { return score < kExcellentScore; }

    Id id = UiResponsiveness;
    int score = 0;
    int points = 0;
};

// Frame timing statistics derived from the ProfileAnimations trace events,
// for QmlProfilerDashboardView.
class QmlProfilerDashboardStats : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerDashboardStats(QmlProfilerModelManager *manager,
                                       QObject *parent = nullptr);

    // The ProfileAnimations trace only carries a measured, instantaneous framerate; the
    // protocol has no concept of a target/display refresh rate, so the user sets it.
    // Changing it re-rates the frames already loaded.
    void setTargetFrameRate(int framesPerSecond);
    int targetFrameRate() const;
    double onTargetFrameTimeMs() const;
    double nearTargetFrameTimeMs() const;
    double stutterFrameTimeMs() const;

    int framesOnTarget() const;
    int framesNearTarget() const;
    int framesFailed() const;
    int framesStuttering() const;
    int framesTotal() const;
    // The median of the measured frame rates, which is the rate reached most of the time.
    int medianFrameRate() const;
    bool hasData() const;
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

    QList<ScoreFactor> scoreBreakdown() const;
    int overallScore() const;

    // The GUI thread frame with the lowest frame rate, or a negative
    // timestamp if there is none.
    qint64 slowestFrameTimestamp() const;
    double slowestFrameTimeMs() const;

signals:
    void changed();

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void reset();
    void rate();

    int m_targetFrameRate = 60;
    // Measured framerate -> number of GUI thread frames reporting it. Enough to rate the
    // frames again for another target without replaying the trace.
    QMap<qint32, int> m_frameRates;
    int m_framesOnTarget = 0;
    int m_framesNearTarget = 0;
    int m_framesFailed = 0;
    int m_framesStuttering = 0;
    qint64 m_slowestFrameTimestamp = -1;
    double m_slowestFrameTimeMs = 0;
};

} // namespace Profiler::Internal
