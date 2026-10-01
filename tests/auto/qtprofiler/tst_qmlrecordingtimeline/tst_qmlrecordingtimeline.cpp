// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlrecordingtimeline.h"

#include <QtTest>

using namespace Profiler::Internal;

using TraceEnd = QmlPauseTracker::TraceEnd;
using Range = QmlPauseTracker::Range;

Q_DECLARE_METATYPE(TraceEnd)

// The requests are what the capture sends, the reports what the server answers,
// in the order it does: each request in turn, and each only once it has
// answered the ones before.
class tst_QmlRecordingTimeline : public QObject
{
    Q_OBJECT

private slots:
    void rebaserStartsAtTheFirstTrace()
    {
        QmlServerTimeRebaser rebaser;
        QCOMPARE(rebaser.map(1234), 0); // Nothing started yet.
        QCOMPARE(rebaser.traceStarted(5000), 0);
        QCOMPARE(rebaser.map(7000), 2000);
        QCOMPARE(rebaser.map(4000), 0); // Before the recording's zero.
        QCOMPARE(rebaser.traceStarted(9000), 4000); // A resume keeps the zero.
    }

    void rebaserResets()
    {
        QmlServerTimeRebaser rebaser;
        rebaser.traceStarted(5000);
        rebaser.reset();
        QCOMPARE(rebaser.traceStarted(8000), 0);
        QCOMPARE(rebaser.map(8500), 500);
    }

    void stopEndsTheRecording()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        QCOMPARE(tracker.traceStarted(100), std::nullopt);
        tracker.stopRequested();
        QCOMPARE(tracker.traceFinished(500, false), TraceEnd::Recording);
    }

    void theApplicationEndingTheTraceEndsTheRecording()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        QCOMPARE(tracker.traceFinished(500, false), TraceEnd::Recording);
    }

    void pauseAndResumeMakeARange()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        QCOMPARE(tracker.traceFinished(200, true), TraceEnd::Pause);  // Pause.
        tracker.startRequested();                                      // Resume.
        QCOMPARE(tracker.traceStarted(300), std::optional(Range(200, 300)));
        tracker.stopRequested();
        QCOMPARE(tracker.traceFinished(400, false), TraceEnd::Recording);
    }

    // Pause, resume, pause again, all before the server answers the first:
    // the trace it starts for the resume ends with the second pause.
    void pauseResumePauseBeforeTheServerAnswers()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        tracker.startRequested(); // The resume, still unanswered.
        QCOMPARE(tracker.traceFinished(200, true), TraceEnd::Pause);
        QCOMPARE(tracker.traceStarted(300), std::optional(Range(200, 300)));
        QCOMPARE(tracker.traceFinished(310, true), TraceEnd::Pause);
        tracker.startRequested();
        QCOMPARE(tracker.traceStarted(400), std::optional(Range(310, 400)));
    }

    // A resume that is not paused any more by the time its pause is reported.
    void resumeBeforeThePauseIsReported()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        tracker.startRequested(); // Paused, then resumed before the answer.
        QCOMPARE(tracker.traceFinished(200, false), TraceEnd::Pause);
        tracker.traceStarted(300);
    }

    // Resume, then stop, before the server starts the trace for the resume:
    // the recording ends with that trace, not before it.
    void stopWhileAResumeIsUnanswered()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        QCOMPARE(tracker.traceFinished(200, true), TraceEnd::Pause);
        QVERIFY(tracker.isSettled(false));

        tracker.startRequested();
        QVERIFY(!tracker.isSettled(false));
        tracker.stopRequested();
        tracker.traceStarted(300);
        QCOMPARE(tracker.traceFinished(400, false), TraceEnd::Recording);
    }

    // Pause, resume, stop, all before the server answers: the first end is
    // the pause, and the second the recording's.
    void pauseResumeStopBeforeTheServerAnswers()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        tracker.startRequested();
        tracker.stopRequested();
        QCOMPARE(tracker.traceFinished(200, false), TraceEnd::Pause);
        tracker.traceStarted(300);
        QCOMPARE(tracker.traceFinished(400, false), TraceEnd::Recording);
    }

    // From a confirmed pause: resume, pause, resume, stop, all unanswered.
    void severalResumesBeforeTheServerAnswers()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        QCOMPARE(tracker.traceFinished(200, true), TraceEnd::Pause);
        tracker.startRequested();
        tracker.startRequested();
        tracker.stopRequested();
        tracker.traceStarted(300);
        QCOMPARE(tracker.traceFinished(350, false), TraceEnd::Pause);
        tracker.traceStarted(400);
        QCOMPARE(tracker.traceFinished(500, false), TraceEnd::Recording);
    }

    // Stop while the pause is still being reported: that report ends it.
    void stopWhileThePauseIsInFlight()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        tracker.traceStarted(100);
        QVERIFY(!tracker.isSettled(true));
        tracker.stopRequested();
        QCOMPARE(tracker.traceFinished(200, true), TraceEnd::Recording);
    }

    void startingPausedIsSettled()
    {
        QmlPauseTracker tracker;
        QVERIFY(tracker.isSettled(false));
        tracker.stopRequested();
        QVERIFY(tracker.isSettled(false));
    }

    void noRangeWithoutAPause()
    {
        QmlPauseTracker tracker;
        tracker.startRequested();
        QCOMPARE(tracker.traceStarted(100), std::nullopt);
        QCOMPARE(tracker.traceFinished(200, true), TraceEnd::Pause);
        tracker.startRequested();
        QCOMPARE(tracker.traceStarted(200), std::nullopt); // Empty.
    }
};

QTEST_GUILESS_MAIN(tst_QmlRecordingTimeline)

#include "tst_qmlrecordingtimeline.moc"
