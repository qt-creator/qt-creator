// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QWidget>

namespace Timeline {

class TimelineZoomControl;

class TRACING_EXPORT SelectionRangeOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit SelectionRangeOverlay(TimelineZoomControl *zoom, QWidget *parent = nullptr);

    void setActive(bool active);
    bool isActive() const { return m_active; }
    bool isReady() const { return m_state == State::Finished; }

    void setRange(qint64 rangeStart, qint64 rangeEnd);
    void reset();

signals:
    void rangeDoubleClicked();
    void passthruClick(QPoint viewportPos); // click in Finished state outside handles/band

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    enum class State { Inactive, FirstLimit, SecondLimit, Finished };
    enum class DragMode { None, Left, Right, Move };

    void setPos(double x);
    void updateZoomer();
    void updateFromZoomer();
    double clamp(double x) const;

    TimelineZoomControl *m_zoom;
    State m_state = State::Inactive;
    bool m_active = false;

    double m_rangeLeft = 0;
    double m_rangeRight = 0;
    double m_creationRef = 0;

    DragMode m_dragMode = DragMode::None;
    double m_dragStartX = 0;
    double m_dragStartLeft = 0;
    double m_dragStartRight = 0;

    qint64 m_rangeStart = 0;
    qint64 m_rangeEnd = 0;
};

} // namespace Timeline
