// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboard_test.h"

#include "../qmlprofilerdashboardstats.h"
#include "../qmlprofilerdashboardview.h"
#include "../qmlprofilermodelmanager.h"

#include <QAbstractButton>
#include <QLabel>
#include <QSignalSpy>
#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

static const int onTargetFps = qRound(kDisplayRefreshRate);
static const int nearTargetFps = qRound(2000 / (kOnTargetFrameTimeMs + kNearTargetFrameTimeMs));
// Fails and stutters.
static const int stutteringFps = qRound(kStutterFps) - 10;

static constexpr qint64 frameSpacing = 20'000'000; // ns

// Appends GUI thread animation frames with the given frame rates, one frameSpacing apart.
static void loadFrames(QmlProfilerModelManager &manager, const QList<int> &frameRates)
{
    manager.initialize();
    const int typeIndex = manager.appendEventType(
        QmlEventType(Event, UndefinedRangeType, AnimationFrame));
    qint64 timestamp = frameSpacing;
    for (int frameRate : frameRates) {
        manager.appendEvent(QmlEvent(timestamp, typeIndex, {frameRate, 1, int(GuiThread)}));
        timestamp += frameSpacing;
    }
    manager.finalize();
}

static QList<int> frames(int count, int frameRate)
{
    return QList<int>(count, frameRate);
}

static QLabel *label(const QWidget &view, const QString &name)
{
    auto result = view.findChild<QLabel *>(name);
    if (!result)
        qWarning() << "No label" << name;
    return result;
}

static QString shownScore(const QWidget &view)
{
    auto rating = view.findChild<QWidget *>("QmlProfiler.Dashboard.PerformanceRating");
    if (!rating)
        return {};
    QLabel *score = label(*rating, "score");
    return score ? score->text() : QString();
}

void QmlProfilerDashboardTest::testBreakdownSumsToScore_data()
{
    QTest::addColumn<QList<int>>("frameRates");

    QTest::newRow("all on target") << frames(100, onTargetFps);
    QTest::newRow("all stuttering") << frames(100, stutteringFps);
    QTest::newRow("few stutters")
        << frames(98, onTargetFps) + frames(2, stutteringFps);
    QTest::newRow("mixed")
        << frames(61, onTargetFps) + frames(23, nearTargetFps) + frames(9, stutteringFps);
}

void QmlProfilerDashboardTest::testBreakdownSumsToScore()
{
    QFETCH(QList<int>, frameRates);

    QmlProfilerModelManager manager;
    QmlProfilerDashboardStats stats(&manager);
    QmlProfilerDashboardView view(&manager);
    loadFrames(manager, frameRates);
    QCOMPARE(stats.framesTotal(), frameRates.size());

    const QList<ScoreFactor> factors = stats.scoreBreakdown();
    QCOMPARE(factors.size(), ScoreFactor::Count);

    int points = 0;
    int shownPoints = 0;
    for (const ScoreFactor &factor : factors) {
        QVERIFY(factor.points >= 0 && factor.points <= ScoreFactor::MaxPoints);
        QCOMPARE(factor.points + factor.pointsLost(), ScoreFactor::MaxPoints);
        points += factor.points;

        QLabel *pointsLabel = label(view, QString("points%1").arg(factor.id));
        QVERIFY(pointsLabel);
        QCOMPARE(pointsLabel->text(), QString::number(factor.points));
        shownPoints += pointsLabel->text().toInt();
    }
    QCOMPARE(points, stats.overallScore());

    QLabel *totalLabel = label(view, "totalPoints");
    QVERIFY(totalLabel);
    QCOMPARE(totalLabel->text(), QString::number(shownPoints));
    QCOMPARE(shownScore(view), QString::number(shownPoints));
}

void QmlProfilerDashboardTest::testFlaggedFactors()
{
    // 2 % failed frames cost P99 Quality 6 points of its score, and nothing else falls
    // below "Excellent".
    QmlProfilerModelManager manager;
    QmlProfilerDashboardStats stats(&manager);
    QmlProfilerDashboardView view(&manager);
    loadFrames(manager, frames(98, onTargetFps) + frames(2, stutteringFps));

    const QList<ScoreFactor> factors = stats.scoreBreakdown();
    QCOMPARE(factors.size(), ScoreFactor::Count);
    QCOMPARE(factors.at(ScoreFactor::UiResponsiveness).score, 98);
    QCOMPARE(factors.at(ScoreFactor::FrameConsistency).score, 98);
    QCOMPARE(factors.at(ScoreFactor::StutterPrevention).score, 98);
    QCOMPARE(factors.at(ScoreFactor::P99Quality).score, 94);
    QCOMPARE(factors.at(ScoreFactor::StartupSpeed).score, 96);
    QCOMPARE(stats.overallScore(), 98);

    for (const ScoreFactor &factor : factors) {
        const bool expectFlagged = factor.id == ScoreFactor::P99Quality;
        QCOMPARE(factor.pullsScoreDown(), expectFlagged);

        QLabel *status = label(view, QString("status%1").arg(factor.id));
        QVERIFY(status);
        QCOMPARE(status->accessibleName() == "Excellent", !expectFlagged);
    }
}

void QmlProfilerDashboardTest::testShowSlowestFrame()
{
    QmlProfilerModelManager manager;
    QmlProfilerDashboardStats stats(&manager);
    QmlProfilerDashboardView view(&manager);
    QList<int> frameRates = frames(10, onTargetFps);
    frameRates[3] = stutteringFps;
    frameRates[7] = nearTargetFps;
    loadFrames(manager, frameRates);

    const qint64 slowestTimestamp = 4 * frameSpacing;
    QCOMPARE(stats.slowestFrameTimestamp(), slowestTimestamp);
    QCOMPARE(stats.slowestFrameTimeMs(), 1000.0 / stutteringFps);

    auto showButton = view.findChild<QAbstractButton *>("showSlowestFrame");
    QVERIFY(showButton);
    QSignalSpy spy(&view, &QmlProfilerDashboardView::timelineRangeRequested);
    showButton->click();
    QCOMPARE(spy.count(), 1);
    const qint64 frameDuration = qRound64(1e9 / stutteringFps);
    QCOMPARE(spy.at(0).at(0).toLongLong(), slowestTimestamp - frameDuration);
    QCOMPARE(spy.at(0).at(1).toLongLong(), slowestTimestamp);
}

} // namespace Profiler::Internal
