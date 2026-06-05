// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QTimer>

namespace Timeline {

class TRACING_EXPORT TimelineZoomControl : public QObject
{
    Q_OBJECT

public:
    TimelineZoomControl(QObject *parent = nullptr);

    qint64 maximumZoomFactor() const { return 1 << 10; }
    qint64 minimumRangeLength() const { return 500; }

    qint64 traceStart() const { return m_traceStart; }
    qint64 traceEnd() const { return m_traceEnd; }
    qint64 traceDuration() const { return m_traceEnd - m_traceStart; }

    qint64 windowStart() const { return m_windowStart; }
    qint64 windowEnd() const { return m_windowEnd; }
    qint64 windowDuration() const { return m_windowEnd - m_windowStart; }

    qint64 rangeStart() const { return m_rangeStart; }
    qint64 rangeEnd() const { return m_rangeEnd; }
    qint64 rangeDuration() const { return m_rangeEnd - m_rangeStart; }

    qint64 selectionStart() const { return m_selectionStart; }
    qint64 selectionEnd() const { return m_selectionEnd; }
    qint64 selectionDuration() const { return m_selectionEnd - m_selectionStart; }

    bool windowLocked() const { return m_windowLocked; }
    bool windowMoving() const { return m_timer.isActive(); }

    virtual void clear();

    void setTrace(qint64 start, qint64 end);
    void setRange(qint64 start, qint64 end);
    void setSelection(qint64 start, qint64 end);
    void setWindowLocked(bool windowLocked);

signals:
    void traceChanged(qint64 start, qint64 end);
    void windowChanged(qint64 start, qint64 end);
    void rangeChanged(qint64 start, qint64 end);
    void selectionChanged(qint64 start, qint64 end);
    void windowLockedChanged(bool windowLocked);
    void windowMovingChanged(bool windowMoving);

protected:
    void moveWindow();
    void rebuildWindow();
    void clampRangeToWindow();

    qint64 m_traceStart = -1;
    qint64 m_traceEnd = -1;
    qint64 m_windowStart = -1;
    qint64 m_windowEnd = -1;
    qint64 m_rangeStart = -1;
    qint64 m_rangeEnd = -1;
    qint64 m_selectionStart = -1;
    qint64 m_selectionEnd = -1;

    QTimer m_timer;
    bool m_windowLocked = false;
};

} // namespace Timeline
