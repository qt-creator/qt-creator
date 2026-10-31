// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QTimer>

#include <chrono>

namespace Timeline {

class TRACING_EXPORT TimelineZoomControl : public QObject
{
    Q_OBJECT

public:
    TimelineZoomControl(QObject *parent = nullptr);

    qint64 maximumZoomFactor() const { return 1 << 10; }
    std::chrono::nanoseconds minimumRangeLength() const { return std::chrono::nanoseconds(500); }

    std::chrono::nanoseconds traceStart() const { return m_traceStart; }
    std::chrono::nanoseconds traceEnd() const { return m_traceEnd; }
    std::chrono::nanoseconds traceDuration() const { return m_traceEnd - m_traceStart; }

    std::chrono::nanoseconds windowStart() const { return m_windowStart; }
    std::chrono::nanoseconds windowEnd() const { return m_windowEnd; }
    std::chrono::nanoseconds windowDuration() const { return m_windowEnd - m_windowStart; }

    std::chrono::nanoseconds rangeStart() const { return m_rangeStart; }
    std::chrono::nanoseconds rangeEnd() const { return m_rangeEnd; }
    std::chrono::nanoseconds rangeDuration() const { return m_rangeEnd - m_rangeStart; }

    std::chrono::nanoseconds selectionStart() const { return m_selectionStart; }
    std::chrono::nanoseconds selectionEnd() const { return m_selectionEnd; }
    std::chrono::nanoseconds selectionDuration() const { return m_selectionEnd - m_selectionStart; }

    bool windowLocked() const { return m_windowLocked; }
    bool windowMoving() const { return m_timer.isActive(); }

    virtual void clear();

    void setTrace(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void setRange(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void setSelection(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void setWindowLocked(bool windowLocked);

signals:
    void traceChanged(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void windowChanged(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void rangeChanged(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void selectionChanged(std::chrono::nanoseconds start, std::chrono::nanoseconds end);
    void windowLockedChanged(bool windowLocked);
    void windowMovingChanged(bool windowMoving);

protected:
    void moveWindow();
    void rebuildWindow();
    void clampRangeToWindow();

    std::chrono::nanoseconds m_traceStart{-1};
    std::chrono::nanoseconds m_traceEnd{-1};
    std::chrono::nanoseconds m_windowStart{-1};
    std::chrono::nanoseconds m_windowEnd{-1};
    std::chrono::nanoseconds m_rangeStart{-1};
    std::chrono::nanoseconds m_rangeEnd{-1};
    std::chrono::nanoseconds m_selectionStart{-1};
    std::chrono::nanoseconds m_selectionEnd{-1};

    QTimer m_timer;
    bool m_windowLocked = false;
};

} // namespace Timeline
