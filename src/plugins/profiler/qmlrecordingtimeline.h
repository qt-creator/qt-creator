// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <optional>
#include <utility>

namespace Profiler::Internal {

// Puts the QML debug server's times on the recording's own timeline. The
// server's clock runs from when the application started and keeps running while
// nothing is recorded, so the first trace it starts is taken as zero, however
// long the application -- or a paused start -- came before.
class QmlServerTimeRebaser
{
public:
    void reset() { m_offset.reset(); }

    // The server started a trace at `time`. Returns that time on the timeline.
    qint64 traceStarted(qint64 time)
    {
        if (!m_offset)
            m_offset = time;
        return map(time);
    }

    // Anything reported before the first trace is at its start.
    qint64 map(qint64 time) const { return qMax<qint64>(0, time - m_offset.value_or(time)); }

private:
    std::optional<qint64> m_offset;
};

// Tells apart the two things the QML debug server means by ending a trace: that
// the recording is over, and that it was paused. It reports a pause as the end
// of a trace and a resume as the start of another, and it answers each request
// late and in order. So what a report means depends on what was asked of it
// since -- a resume it has not answered yet, a stop -- and not on whether the
// recording happens to be paused when the report arrives.
class QmlPauseTracker
{
public:
    enum class TraceEnd { Pause, Recording };
    using Range = std::pair<qint64, qint64>;

    // The server was asked to start a trace, and will answer with one.
    void startRequested() { ++m_unansweredStarts; }
    void stopRequested() { m_stopping = true; }

    // The server started a trace at `time`. Returns the pause that ended there.
    std::optional<Range> traceStarted(qint64 time)
    {
        if (m_unansweredStarts > 0)
            --m_unansweredStarts;
        std::optional<Range> range;
        if (m_pauseStart && time > *m_pauseStart)
            range = Range(*m_pauseStart, time);
        m_pauseStart.reset();
        return range;
    }

    // The server ended a trace at `time`, while the recording is `paused`. A
    // trace ends the recording when nothing is still to be answered and either
    // a stop was asked for or the application ended the trace itself.
    TraceEnd traceFinished(qint64 time, bool paused)
    {
        if (m_unansweredStarts == 0 && (m_stopping || !paused))
            return TraceEnd::Recording;
        m_pauseStart = time;
        return TraceEnd::Pause;
    }

    // Whether the server has answered everything, so that with its trace
    // ended no further report is coming.
    bool isSettled(bool serverRecording) const
    {
        return !serverRecording && m_unansweredStarts == 0;
    }

private:
    int m_unansweredStarts = 0;
    bool m_stopping = false;
    std::optional<qint64> m_pauseStart;
};

} // namespace Profiler::Internal
