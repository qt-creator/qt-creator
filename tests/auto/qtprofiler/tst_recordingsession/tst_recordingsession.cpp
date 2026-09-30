// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sampler.h"

#include <QtTest>

using namespace Profiler::Internal;

class tst_RecordingSession : public QObject
{
    Q_OBJECT

private slots:
    // notifyReports() queues events that own the session's reporter. Left
    // queued they are destroyed inside the application's own teardown, where
    // releasing the reporter asks for deleteLater() under a lock that teardown
    // already holds.
    void cleanup()
    {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    void startsNotPaused()
    {
        RecordingSession session;
        QVERIFY(!session.isPaused());
        QVERIFY(session.pausedIntervals().empty());
        QVERIFY(!session.wasPausedAt(RecordingSession::steadyNowNs()));
    }

    void pauseAndResumeRecordAnInterval()
    {
        RecordingSession session;
        const qint64 before = RecordingSession::steadyNowNs();
        session.setPaused(true);
        QVERIFY(session.isPaused());
        const qint64 during = RecordingSession::steadyNowNs();
        QVERIFY(session.wasPausedAt(during));
        QVERIFY(!session.wasPausedAt(before - 1));

        session.setPaused(false);
        const qint64 after = RecordingSession::steadyNowNs();
        QVERIFY(!session.isPaused());
        // Judged by when it was taken, not by the state now.
        QVERIFY(session.wasPausedAt(during));
        QVERIFY(!session.wasPausedAt(after + 1));

        const auto intervals = session.pausedIntervals();
        QCOMPARE(intervals.size(), size_t(1));
        QVERIFY(intervals.front().first >= before);
        QVERIFY(intervals.front().second >= intervals.front().first);
        QVERIFY(intervals.front().second <= after);
    }

    void intervalIsHalfOpen()
    {
        RecordingSession session;
        session.setPaused(true);
        session.setPaused(false);
        const auto [start, end] = session.pausedIntervals().front();
        QVERIFY(session.wasPausedAt(start));
        QVERIFY(!session.wasPausedAt(start - 1));
        QVERIFY(!session.wasPausedAt(end));
    }

    void anOpenIntervalRunsUntilResumed()
    {
        RecordingSession session;
        session.setPaused(true);
        QCOMPARE(session.pausedIntervals().front().second, qint64(-1));
        QVERIFY(session.wasPausedAt(RecordingSession::steadyNowNs() + 1'000'000'000));
    }

    void settingTheSameStateChangesNothing()
    {
        RecordingSession session;
        int calls = 0;
        QObject context;
        session.onPausedChanged(&context, [&calls](bool) { ++calls; });

        session.setPaused(false);
        QCOMPARE(calls, 0);
        session.setPaused(true);
        session.setPaused(true);
        QCOMPARE(calls, 1);
        QCOMPARE(session.pausedIntervals().size(), size_t(1));
    }

    void handlersSeeEachChange()
    {
        RecordingSession session;
        QList<bool> seen;
        QObject context;
        session.onPausedChanged(&context, [&seen](bool paused) { seen.append(paused); });

        session.setPaused(true);
        session.setPaused(false);
        session.setPaused(true);
        QCOMPARE(seen, (QList<bool>{true, false, true}));
        QCOMPARE(session.pausedIntervals().size(), size_t(2));
    }

    void aHandlerEndsWithItsContext()
    {
        RecordingSession session;
        int calls = 0;
        {
            QObject context;
            session.onPausedChanged(&context, [&calls](bool) { ++calls; });
            session.setPaused(true);
            QCOMPARE(calls, 1);
        }
        session.setPaused(false);
        QCOMPARE(calls, 1);
    }

    // A handler owns what it captures, so one that is not dropped keeps that
    // alive -- a handler capturing its own session would keep the session.
    void aHandlerIsDroppedOnceItsContextIsGone()
    {
        RecordingSession session;
        auto captured = std::make_shared<int>(0);
        {
            QObject context;
            session.onPausedChanged(&context, [captured](bool) {});
        }
        QCOMPARE(captured.use_count(), 2);
        QObject other;
        session.onPausedChanged(&other, [](bool) {});
        QCOMPARE(captured.use_count(), 1);
    }

    void stoppingDropsThePauseHandlers()
    {
        RecordingSession session;
        auto captured = std::make_shared<int>(0);
        QObject context;
        session.onPausedChanged(&context, [captured](bool) {});
        QCOMPARE(captured.use_count(), 2);
        session.requestStop();
        QCOMPARE(captured.use_count(), 1);
    }

    void pausingReachesTheSubSessions()
    {
        auto parent = std::make_shared<RecordingSession>();
        auto first = std::make_shared<RecordingSession>();
        auto second = std::make_shared<RecordingSession>();
        parent->addSubSession(first);
        parent->addSubSession(second);

        parent->setPaused(true);
        QVERIFY(first->isPaused());
        QVERIFY(second->isPaused());
        QVERIFY(first->wasPausedAt(RecordingSession::steadyNowNs()));

        parent->setPaused(false);
        QVERIFY(!first->isPaused());
        QVERIFY(!second->isPaused());
    }

    void aSubSessionAddedWhilePausedStartsPaused()
    {
        auto parent = std::make_shared<RecordingSession>();
        parent->setPaused(true);

        auto child = std::make_shared<RecordingSession>();
        parent->addSubSession(child);
        QVERIFY(child->isPaused());
        QCOMPARE(child->pausedIntervals().size(), size_t(1));

        parent->setPaused(false);
        QVERIFY(!child->isPaused());
    }

    void noEffectOnceStopped()
    {
        RecordingSession session;
        session.requestStop();
        session.setPaused(true);
        QVERIFY(!session.isPaused());
        QVERIFY(session.pausedIntervals().empty());
    }

    void stoppingWhilePausedLeavesItPaused()
    {
        RecordingSession session;
        session.setPaused(true);
        session.requestStop();
        session.setPaused(false);
        QVERIFY(session.isPaused());
        QCOMPARE(session.pausedIntervals().front().second, qint64(-1));
    }
};

QTEST_GUILESS_MAIN(tst_RecordingSession)

#include "tst_recordingsession.moc"
