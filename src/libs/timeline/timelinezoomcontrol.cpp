/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "timelinezoomcontrol.h"

namespace Timeline {

TimelineZoomControl::TimelineZoomControl(QObject *parent) : QObject(parent),
    m_traceStart(-1), m_traceEnd(-1), m_windowStart(-1), m_windowEnd(-1),
    m_rangeStart(-1), m_rangeEnd(-1), m_selectionStart(-1), m_selectionEnd(-1),
    m_windowLocked(false)
{
    connect(&m_timer, &QTimer::timeout, this, &TimelineZoomControl::moveWindow);
}

void TimelineZoomControl::clear()
{
    m_timer.stop();
    setWindowLocked(false);
    setRange(-1, -1);
    setTrace(-1, -1);
}

void TimelineZoomControl::setTrace(qint64 start, qint64 end)
{
    Q_ASSERT(start <= end);
    if (start != m_traceStart || end != m_traceEnd) {
        m_traceStart = start;
        m_traceEnd = end;
        emit traceChanged(start, end);
        rebuildWindow();
    }
}

void TimelineZoomControl::setRange(qint64 start, qint64 end)
{
    Q_ASSERT(start <= end);
    if (m_rangeStart != start || m_rangeEnd != end) {
        m_timer.stop();
        m_rangeStart = start;
        m_rangeEnd = end;
        rebuildWindow();
        emit rangeChanged(start, end);
    }
}

void TimelineZoomControl::setSelection(qint64 start, qint64 end)
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
    qint64 minDuration = 1; // qMax needs equal data types, so literal 1 won't do
    qint64 shownDuration = qMax(rangeDuration(), minDuration);

    qint64 oldWindowStart = m_windowStart;
    qint64 oldWindowEnd = m_windowEnd;
    if (traceDuration() / shownDuration < MAX_ZOOM_FACTOR) {
        m_windowStart = m_traceStart;
        m_windowEnd = m_traceEnd;
    } else if (windowDuration() / shownDuration > MAX_ZOOM_FACTOR ||
               windowDuration() / shownDuration * 2 < MAX_ZOOM_FACTOR) {
        qint64 keep = shownDuration * MAX_ZOOM_FACTOR / 2 - shownDuration;
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
        m_timer.start(500);
    }
    if (oldWindowStart != m_windowStart || oldWindowEnd != m_windowEnd) {
        clampRangeToWindow();
        emit windowChanged(m_windowStart, m_windowEnd);
    }
}

void TimelineZoomControl::moveWindow()
{
    if (m_windowLocked)
        return;
    m_timer.stop();

    qint64 offset = (m_rangeEnd - m_windowEnd + m_rangeStart - m_windowStart) / 2;
    if (offset == 0 || (offset < 0 && m_windowStart == m_traceStart) ||
            (offset > 0 && m_windowEnd == m_traceEnd)) {
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
    m_timer.start(100);
}

void TimelineZoomControl::clampRangeToWindow()
{
    qint64 rangeStart = qMin(qMax(m_rangeStart, m_windowStart), m_windowEnd);
    qint64 rangeEnd = qMin(qMax(rangeStart, m_rangeEnd), m_windowEnd);
    if (rangeStart != m_rangeStart || rangeEnd != m_rangeEnd)
        setRange(rangeStart, rangeEnd);
}

} // namespace Timeline
