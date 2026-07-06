// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QElapsedTimer>
#include <QList>
#include <QPixmap>
#include <QWidget>

#include <chrono>

namespace Timeline {

class TimelineModel;
class TimelineNotesModel;

class TRACING_EXPORT TrackPainter : public QWidget
{
    Q_OBJECT
public:
    explicit TrackPainter(QWidget *parent = nullptr);

    void setModel(TimelineModel *model);
    TimelineModel *model() { return m_model; }
    const TimelineModel *model() const { return m_model; }

    void setStartOdd(bool startOdd);
    bool startOdd() const { return m_startOdd; }

    void setNotes(const TimelineNotesModel *notes);
    void setMarkers(const QList<qint64> &markers);

    void setRange(qint64 rangeStart, qint64 rangeEnd);

    qint64 rangeStart() const { return m_rangeStart; }
    qint64 rangeEnd() const { return m_rangeEnd; }

    void setSelectedItem(int index);  // -1 = none
    void setHoveredItem(int index);   // -1 = none
    void setSelectionLocked(bool locked);

    int indexAt(const QPoint &pos) const;

    QSize sizeHint() const override;

signals:
    void itemHovered(int index);
    void itemClicked(int index);
    void horizontalPan(int dx);
    void verticalPan(int dy);
    void zoomRequested(double cursorX, int dy);

    // CPU time spent in this paintEvent(), for the frame-time overlay. Summed
    // across the track widgets over one repaint cycle it gives the full-frame
    // render time.
    void painted(std::chrono::nanoseconds renderTime);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    void invalidateCache();
    void rebuildCache(const QRect &viewRect);
    // The on-screen portion of this (potentially very tall) widget, in widget
    // coordinates. The cache and all rendering are bounded to this slice.
    QRect visibleRect() const;
    // Renders the cached content (events, markers, notes) using m_rangeStart/End
    // and width() for coordinates, in widget space. Only events in
    // [iterStart, iterEnd] are iterated and only rows intersecting viewRect are
    // drawn. Backgrounds and grid lines are drawn live (see paintBackground /
    // paintGridOverlay), not baked into the cache. Caller must translate the
    // painter so viewRect.topLeft() maps to the pixmap origin, and set the clip
    // rect for strip renders.
    void renderContent(QPainter &p, qint64 iterStart, qint64 iterEnd,
                       const QRect &viewRect) const;
    // Live layers (not cached): cheap and range/scroll dependent, so drawing
    // them every frame avoids the drift that baking + integer pixel shifting
    // introduced. Drawn in widget coordinates, bounded to viewRect.
    void paintBackground(QPainter &p, const QRect &viewRect) const;
    void paintGridOverlay(QPainter &p, const QRect &viewRect) const;
    void paintScaleOverlay(QPainter &p);

    TimelineModel *m_model = nullptr;
    const TimelineNotesModel *m_notes = nullptr;
    QList<qint64> m_markers;
    qint64 m_rangeStart = 0;
    qint64 m_rangeEnd = 0;
    int m_selectedItem = -1;
    int m_hoveredItem = -1;
    bool m_selectionLocked = true;
    bool m_startOdd = true;

    QPoint m_pressPos;
    bool m_panning = false;

    // Off-screen cache of the current view. Bounded to the on-screen slice
    // (m_cacheRect, widget coordinates), so an expanded track that is far taller
    // than the viewport never allocates a full-height pixmap.
    // m_pendingShiftPx: pixels the cache must be shifted to match the current range.
    // Positive = shift right (user panned left / earlier time is now visible).
    QPixmap m_cache;
    QRect m_cacheRect;
    int m_pendingShiftPx = 0;
};

} // namespace Timeline
