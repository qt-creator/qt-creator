// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboardstats_test.h"

#include "../qmlprofilerdashboardview.h"
#include "../qmlprofilersettings.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QScopeGuard>
#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

QmlProfilerDashboardStatsTest::QmlProfilerDashboardStatsTest()
    : stats(&manager)
{}

void QmlProfilerDashboardStatsTest::initTestCase()
{
    manager.initialize();

    const int typeIndex
        = manager.appendEventType(QmlEventType(Event, UndefinedRangeType, AnimationFrame));
    qint64 timestamp = 0;
    const auto addFrames = [&](int framesPerSecond, int count, AnimationThread thread) {
        for (int i = 0; i < count; ++i) {
            QmlEvent frame;
            frame.setTypeIndex(typeIndex);
            frame.setTimestamp(++timestamp);
            frame.setNumbers<qint32>({framesPerSecond, 1, thread});
            manager.appendEvent(std::move(frame));
        }
    };

    // Counts that differ by powers of two, so that any mix-up between buckets shows. None
    // of the rates sits on a threshold of the targets tested.
    addFrames(240, 1, GuiThread);
    addFrames(144, 2, GuiThread);
    addFrames(60, 4, GuiThread);
    addFrames(27, 8, GuiThread);
    addFrames(10, 16, GuiThread);
    // The render thread's frames are not what ends up on screen.
    addFrames(5, 32, RenderThread);

    manager.finalize();
}

void QmlProfilerDashboardStatsTest::testRatedAgainstTarget_data()
{
    QTest::addColumn<int>("target");
    QTest::addColumn<int>("onTarget");
    QTest::addColumn<int>("nearTarget");
    QTest::addColumn<int>("failed");
    QTest::addColumn<int>("stuttering");

    // 33.3 ms per frame, near up to 40 ms, stutter above 66.7 ms.
    QTest::newRow("30 FPS") << 30 << 1 + 2 + 4 << 8 << 16 << 16;
    // 16.7 ms per frame, near up to 20 ms, stutter above 33.3 ms.
    QTest::newRow("60 FPS") << 60 << 1 + 2 + 4 << 0 << 8 + 16 << 8 + 16;
    // 4.2 ms per frame, near up to 5 ms, stutter above 8.3 ms.
    QTest::newRow("240 FPS") << 240 << 1 << 0 << 2 + 4 + 8 + 16 << 4 + 8 + 16;
}

void QmlProfilerDashboardStatsTest::testRatedAgainstTarget()
{
    QFETCH(int, target);
    QFETCH(int, onTarget);
    QFETCH(int, nearTarget);
    QFETCH(int, failed);
    QFETCH(int, stuttering);

    const int eventsBefore = manager.numEvents();
    int changes = 0;
    const QMetaObject::Connection connection
        = connect(&stats, &QmlProfilerDashboardStats::changed, this, [&changes] { ++changes; });
    const auto disconnect = qScopeGuard([connection] { QObject::disconnect(connection); });

    // Rated again from what was loaded already: no replay, the same trace.
    stats.setTargetFrameRate(target);
    QCOMPARE(stats.targetFrameRate(), target);
    QCOMPARE(stats.framesOnTarget(), onTarget);
    QCOMPARE(stats.framesNearTarget(), nearTarget);
    QCOMPARE(stats.framesFailed(), failed);
    QCOMPARE(stats.framesStuttering(), stuttering);
    QCOMPARE(stats.framesTotal(), 31);
    QCOMPARE(stats.onTargetPercent(), onTarget * 100 / 31);
    // What was measured does not depend on the target: 16 of the 31 frames ran at 10 FPS.
    QCOMPARE(stats.medianFrameRate(), 10);
    QCOMPARE(manager.numEvents(), eventsBefore);
    // The view only redraws on changed(); a target that changes nothing need not say so.
    QVERIFY(changes <= 1);
}

// As the dashboard and the options page do it: through apply(), which is what announces it.
static void applyTargetFrameRate(int framesPerSecond)
{
    QmlProfilerSettings &settings = globalSettings();
    settings.targetFrameRate.setVolatileValue(framesPerSecond);
    settings.apply();
}

static bool hasLabelWithText(const QWidget &widget, const QString &text)
{
    const QList<QLabel *> labels = widget.findChildren<QLabel *>();
    return std::any_of(labels.begin(), labels.end(), [&text](const QLabel *label) {
        return label->text() == text;
    });
}

void QmlProfilerDashboardStatsTest::testDashboardFollowsSetting()
{
    QmlProfilerSettings &settings = globalSettings();
    const int originalTarget = settings.targetFrameRate();
    const auto restore = qScopeGuard([originalTarget, &settings] {
        applyTargetFrameRate(originalTarget);
        // The dashboard persists what is picked in it.
        settings.writeSettings();
    });

    QmlProfilerModelManager viewManager;
    QmlProfilerDashboardView view(&viewManager);
    auto combo = view.findChild<QComboBox *>("QmlProfiler.Dashboard.TargetFrameRate");
    QVERIFY(combo);

    // A target set elsewhere, in the options, is the one the dashboard shows and rates
    // against.
    applyTargetFrameRate(240);
    QCOMPARE(combo->currentText(), QString("240"));
    QVERIFY(hasLabelWithText(view, "On Target (>= 240 FPS, <= 4.2 ms)"));
    QVERIFY(hasLabelWithText(view, "Share of frames faster than half the target, 120 FPS "
                                   "(8.3 ms). Slower frames cause noticeable UI freezes"));
    QVERIFY(hasLabelWithText(view, "Score Breakdown (Target: 240 FPS)"));
    // The target is never presented as if it had been measured.
    QVERIFY(hasLabelWithText(view, "FPS measured (median)\nTarget: 240 FPS"));
    QVERIFY(hasLabelWithText(view, "Measured frames, rated against the 240 FPS target"));
    // Both take more than one line, and capped at one line, the target is cut off.
    for (const char *name : {"gaugeText", "framesText"}) {
        auto label = view.findChild<QLabel *>(name);
        QVERIFY2(label, name);
        QVERIFY2(label->maximumHeight() >= 2 * label->fontMetrics().height(), name);
    }

    // A value that is no preset is shown as it is.
    applyTargetFrameRate(75);
    QCOMPARE(combo->currentText(), QString("75"));
    QVERIFY(hasLabelWithText(view, "On Target (>= 75 FPS, <= 13.3 ms)"));

    // Picked in the dashboard, it becomes the setting. Stepping from one preset to the
    // next is a pick like any other.
    const int preset30 = combo->findData(30);
    QVERIFY(preset30 >= 0);
    QCOMPARE(combo->itemData(preset30 + 1).toInt(), 60);
    combo->setCurrentIndex(preset30);
    QTest::keyClick(combo, Qt::Key_Down);
    QCOMPARE(settings.targetFrameRate(), 60);
    QVERIFY(hasLabelWithText(view, "On Target (>= 60 FPS, <= 16.7 ms)"));

    // Typed in it, too.
    QLineEdit *edit = combo->lineEdit();
    QVERIFY(edit);
    edit->setText("50");
    QTest::keyClick(edit, Qt::Key_Return);
    QCOMPARE(settings.targetFrameRate(), 50);
    QVERIFY(hasLabelWithText(view, "On Target (>= 50 FPS, <= 20.0 ms)"));
}

void QmlProfilerDashboardStatsTest::testBreakdownRatedAgainstTarget()
{
    QmlProfilerSettings &settings = globalSettings();
    const int originalTarget = settings.targetFrameRate();
    const auto restore = qScopeGuard([originalTarget] { applyTargetFrameRate(originalTarget); });

    QmlProfilerModelManager viewManager;
    QmlProfilerDashboardView view(&viewManager);
    viewManager.initialize();
    const int typeIndex
        = viewManager.appendEventType(QmlEventType(Event, UndefinedRangeType, AnimationFrame));
    // A steady 58 FPS: plenty for 30 FPS, nowhere near 240 FPS.
    for (int i = 1; i <= 100; ++i)
        viewManager.appendEvent(QmlEvent(i * 17'000'000ll, typeIndex, {58, 1, int(GuiThread)}));
    viewManager.finalize();

    const auto shown = [&view](const QString &name) {
        auto label = view.findChild<QLabel *>(name);
        return label ? label->text() : QString("<no label %1>").arg(name);
    };

    applyTargetFrameRate(30);
    QCOMPARE(shown("totalPoints"), QString("100"));
    for (int id = 0; id < ScoreFactor::Count; ++id)
        QCOMPARE(shown(QString("points%1").arg(id)), QString::number(ScoreFactor::MaxPoints));
    QVERIFY(hasLabelWithText(view, "Share of frames on the 30 FPS target (<= 33.3 ms)"));

    // The same frames, re-rated without being loaded again.
    applyTargetFrameRate(240);
    QCOMPARE(shown("totalPoints"), QString("0"));
    for (int id = 0; id < ScoreFactor::Count; ++id)
        QCOMPARE(shown(QString("points%1").arg(id)), QString("0"));
    QVERIFY(hasLabelWithText(view, "Share of frames on the 240 FPS target (<= 4.2 ms)"));
    QVERIFY(hasLabelWithText(view, "0 of 100 frame(s) on target"));
}

} // namespace Profiler::Internal
