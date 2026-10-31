// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinezoomcontrol.h"
#include <utils/qtcassert.h>

using namespace std::chrono;

namespace Timeline {

TimelineZoomControl::TimelineZoomControl(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &TimelineZoomControl::moveWindow);
}

void TimelineZoomControl::clear()
{
    const bool changeTrace = m_traceStart != -1ns || m_traceEnd != -1ns;
    const bool changeWindow = m_windowStart != -1ns || m_windowEnd != -1ns;
    const bool changeRange = m_rangeStart != -1ns || m_rangeEnd != -1ns;

    setWindowLocked(false);
    if (changeWindow && !m_timer.isActive())
        emit windowMovingChanged(true);

    m_traceStart = m_traceEnd = m_windowStart = m_windowEnd = m_rangeStart = m_rangeEnd = -1ns;
    if (changeTrace)
        emit traceChanged(-1ns, -1ns);

    if (changeWindow) {
        emit windowChanged(-1ns, -1ns);
        m_timer.stop();
        emit windowMovingChanged(false);
    } else {
        QTC_ASSERT(!m_timer.isActive(), m_timer.stop());
    }

    if (changeRange)
        emit rangeChanged(-1ns, -1ns);

    setSelection(-1ns, -1ns);
}

void TimelineZoomControl::setTrace(nanoseconds start, nanoseconds end)
{
    QTC_ASSERT(start <= end, { std::swap(start, end); });
    if (start != m_traceStart || end != m_traceEnd) {
        m_traceStart = start;
        m_traceEnd = end;
        emit traceChanged(start, end);
        rebuildWindow();
    }
}

void TimelineZoomControl::setRange(nanoseconds start, nanoseconds end)
{
    QTC_ASSERT(start <= end, { std::swap(start, end); });
    if (m_rangeStart != start || m_rangeEnd != end) {
        if (m_timer.isActive()) {
            m_timer.stop();
            emit windowMovingChanged(false);
        }
        m_rangeStart = start;
        m_rangeEnd = end;
        rebuildWindow();
        if (m_rangeStart == start && m_rangeEnd == end)
            emit rangeChanged(m_rangeStart, m_rangeEnd);
        // otherwise rebuildWindow() has changed it again.
    }
}

void TimelineZoomControl::setSelection(nanoseconds start, nanoseconds end)
{
    if (m_selectionStart != start || m_selectionEnd != end) {
        m_selectionStart = start;
        m_selectionEnd = end;
        emit selectionChanged(start, end);
    }
}

void TimelineZoomControl::setWindowLocked(bool windowLocked)
{
    if (windowLocked  != m_windowLocked) {
        m_windowLocked = windowLocked;
        emit windowLockedChanged(windowLocked);
    }
}

void TimelineZoomControl::rebuildWindow()
{
    nanoseconds shownDuration = qMax(rangeDuration(), 1ns);

    nanoseconds oldWindowStart = m_windowStart;
    nanoseconds oldWindowEnd = m_windowEnd;
    if (traceDuration() / shownDuration < maximumZoomFactor()) {
        m_windowStart = m_traceStart;
        m_windowEnd = m_traceEnd;
    } else if (windowDuration() / shownDuration > maximumZoomFactor() ||
               windowDuration() / shownDuration * 2 < maximumZoomFactor() ||
               m_rangeStart < m_windowStart || m_rangeEnd > m_windowEnd) {
        nanoseconds keep = shownDuration * maximumZoomFactor() / 2 - shownDuration;
        m_windowStart = m_rangeStart - keep;
        if (m_windowStart < m_traceStart) {
            keep += m_traceStart - m_windowStart;
            m_windowStart = m_traceStart;
        }

        m_windowEnd = m_rangeEnd + keep;
        if (m_windowEnd > m_traceEnd) {
            m_windowStart = qMax(m_traceStart, m_windowStart - (m_windowEnd - m_traceEnd));
            m_windowEnd = m_traceEnd;
        }
    } else {
        m_timer.start(501ms);
    }
    if (oldWindowStart != m_windowStart || oldWindowEnd != m_windowEnd) {
        bool runTimer = m_timer.isActive();
        if (!runTimer)
            m_timer.start(std::numeric_limits<int>::max());
        emit windowMovingChanged(true);
        clampRangeToWindow(); // can stop the timer
        emit windowChanged(m_windowStart, m_windowEnd);
        if (!runTimer && m_timer.isActive()) {
            m_timer.stop();
            emit windowMovingChanged(false);
        }
    }
}

void TimelineZoomControl::moveWindow()
{
    if (m_windowLocked)
        return;
    m_timer.stop();

    nanoseconds offset = (m_rangeEnd - m_windowEnd + m_rangeStart - m_windowStart) / 2;
    if (offset == 0ns || (offset < 0ns && m_windowStart == m_traceStart) ||
            (offset > 0ns && m_windowEnd == m_traceEnd)) {
        emit windowMovingChanged(false);
        return;
    } else if (offset > rangeDuration()) {
        offset = (offset + rangeDuration()) / 2;
    } else if (offset < -rangeDuration()) {
        offset = (offset - rangeDuration()) / 2;
    }
    m_windowStart += offset;
    if (m_windowStart < m_traceStart) {
        m_windowEnd += m_traceStart - m_windowStart;
        m_windowStart = m_traceStart;
    }
    m_windowEnd += offset;
    if (m_windowEnd > m_traceEnd) {
        m_windowStart -= m_windowEnd - m_traceEnd;
        m_windowEnd = m_traceEnd;
    }

    clampRangeToWindow();
    emit windowChanged(m_windowStart, m_windowEnd);
    m_timer.start(100ms);
}

void TimelineZoomControl::clampRangeToWindow()
{
    nanoseconds rangeStart = qMin(qMax(m_rangeStart, m_windowStart), m_windowEnd);
    nanoseconds rangeEnd = qMin(qMax(rangeStart, m_rangeEnd), m_windowEnd);
    if (rangeStart != m_rangeStart || rangeEnd != m_rangeEnd)
        setRange(rangeStart, rangeEnd);
}

} // namespace Timeline
