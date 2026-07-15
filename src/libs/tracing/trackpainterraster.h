// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "trackpainterbase.h"

#include <QList>
#include <QPainterPath>
#include <QRgb>
#include <QWidget>

#include <chrono>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace Timeline {

// QPainter (software) twin of TrackPainter. Feature- and pixel-identical to the
// QCanvasPainter/RHI backend: it shares all geometry, hit-testing and
// interaction logic with it through TrackPainterBase, and only differs in that
// it caches the neutral geometry as QPainterPath and paints with QPainter in a
// regular paintEvent().
class TRACING_EXPORT TrackPainterRaster : public QWidget, public TrackPainterBase
{
    Q_OBJECT
public:
    explicit TrackPainterRaster(QWidget *parent = nullptr);

    QWidget *widget() override { return this; }
    TrackBackend backend() const override { return TrackBackend::Software; }

    QSize sizeHint() const override;

signals:
    void itemHovered(int trackIndex, int itemIndex);
    void itemClicked(int trackIndex, int itemIndex);
    void horizontalPan(int dx);
    void verticalPan(int dy);
    void zoomRequested(double cursorX, int dy);

    // CPU time spent in this paintEvent(), for the frame-time overlay. This
    // single widget renders all tracks, so one paint is the full-frame time.
    void painted(std::chrono::nanoseconds renderTime);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void leaveEvent(QEvent *) override;

    void invalidateBackendGeometry() override { m_geometryValid = false; }
    void notifyItemHovered(int trackIndex, int itemIndex) override
    { emit itemHovered(trackIndex, itemIndex); }
    void notifyItemClicked(int trackIndex, int itemIndex) override
    { emit itemClicked(trackIndex, itemIndex); }
    void notifyHorizontalPan(int dx) override { emit horizontalPan(dx); }
    void notifyVerticalPan(int dy) override { emit verticalPan(dy); }
    void notifyZoomRequested(double cursorX, int dy) override
    { emit zoomRequested(cursorX, dy); }

private:
    // Cached, range-dependent fill geometry for one track, built once per rebuild
    // from the neutral geometry and replayed each frame. Axis-aligned, opaque
    // groups (backgrounds, grid, markers) are kept as plain rects and filled with
    // QPainter::fillRect, which hits the fast solid-fill blitter; the antialiased
    // event bars and notes are pre-assembled into QPainterPaths.
    struct TrackGeometry {
        QList<QRectF> background[2];    // [0] = bg1 rows, [1] = bg2 rows
        QList<QRectF> grid;             // 1px columns
        QList<ColorRects> fills;        // event bars or density columns, by color
        QList<QRectF> markers;          // 2px columns
        QPainterPath notes;             // exclamation marks (antialiased)
    };

    void ensureGeometry();           // rebuild the cache if the range/width changed
    void buildTrackGeometry(const Track &track, TrackGeometry &geom) const;
    void paintScaleOverlay(QPainter &p, const Track &track) const;
    void paintSelectionOverlay(QPainter &p) const;

    QList<TrackGeometry> m_geometry;  // parallel to tracks(); valid for the cache key below
    bool m_geometryValid = false;
    qint64 m_geomRangeStart = 0;
    qint64 m_geomRangeEnd = 0;
    int m_geomWidth = -1;
};

} // namespace Timeline
