// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QList>
#include <QPixmap>
#include <QWidget>

namespace Timeline {

class TimelineModel;
class TimelineNotesModel;

class TRACING_EXPORT TrackPainter : public QWidget
{
    Q_OBJECT
public:
    explicit TrackPainter(QWidget *parent = nullptr);

    void setModel(TimelineModel *model);
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
    void rebuildCache();
    // Renders content using m_rangeStart/End and width() for coordinates.
    // Only events in [iterStart, iterEnd] are iterated.
    // Caller must set painter clip rect for strip renders.
    void renderContent(QPainter &p, qint64 iterStart, qint64 iterEnd) const;
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

    // Off-screen cache of the current view.
    // m_pendingShiftPx: pixels the cache must be shifted to match the current range.
    // Positive = shift right (user panned left / earlier time is now visible).
    QPixmap m_cache;
    int m_pendingShiftPx = 0;
};

} // namespace Timeline
