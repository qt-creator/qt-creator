// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "trackpainterbase.h"

#include <QCanvasImage>
#include <QCanvasPainterWidget>
#include <QCanvasPath>
#include <QList>
#include <QRgb>

#include <chrono>

QT_BEGIN_NAMESPACE
class QCanvasPainter;
QT_END_NAMESPACE

namespace Timeline {

// Renders the whole track area (all visible tracks stacked vertically) into a
// single hardware-accelerated QCanvasPainter surface. The widget fills the
// scroll-area viewport and scrolls its content by an internal pixel offset
// rather than being a tall widget moved by the scroll area; this keeps the GPU
// render target viewport-sized regardless of how many tracks there are.
//
// This is the GPU backend of the track area; TrackPainterRaster is the QPainter
// twin. All backend-independent logic lives in TrackPainterBase; this class only
// caches the neutral geometry as QCanvasPath and issues the QCanvasPainter draw
// calls.
class TRACING_EXPORT TrackPainter : public QCanvasPainterWidget, public TrackPainterBase
{
    Q_OBJECT
public:
    explicit TrackPainter(QWidget *parent = nullptr);

    QWidget *widget() override { return this; }
    TrackBackend backend() const override { return TrackBackend::Gpu; }

    QSize sizeHint() const override;

signals:
    void itemHovered(int trackIndex, int itemIndex);
    void itemClicked(int trackIndex, int itemIndex);
    void horizontalPan(int dx);
    void verticalPan(int dy);
    void zoomRequested(double cursorX, int dy);

    // CPU time spent in this paint(), for the frame-time overlay. This single
    // widget renders all tracks, so one paint() is the full-frame render time.
    void painted(std::chrono::nanoseconds renderTime);

protected:
    void initializeResources(QCanvasPainter *painter) override;
    void graphicsResourcesInvalidated() override { m_noteIcon = {}; }
    void paint(QCanvasPainter *painter) override;
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
    // Cached, range-dependent fill geometry for one track as QCanvasPath, built
    // once per rebuild from the neutral geometry and replayed each frame.
    struct ColorPath {
        QRgb color;
        QCanvasPath path;
    };
    struct TrackGeometry {
        QCanvasPath background[2];      // [0] = bg1 rows, [1] = bg2 rows
        bool hasBackground[2] = {false, false};
        QCanvasPath grid;
        bool hasGrid = false;
        QList<ColorPath> fills;         // event bars or density columns, grouped by color
        QCanvasPath outlines;           // Token_Stroke_Subtle, above and below the track
        bool hasOutlines = false;
        QCanvasPath markers;
        bool hasMarkers = false;
        QList<QPoint> noteIcons;        // center point of each note icon
    };

    void ensureGeometry();           // rebuild the cache if the range/width changed
    void buildTrackGeometry(const Track &track, TrackGeometry &geom) const;
    void paintScaleOverlay(QCanvasPainter &p, const Track &track) const;
    void paintSelectionOverlay(QCanvasPainter &p) const;

    QList<TrackGeometry> m_geometry;  // parallel to tracks(); valid for the cache key below
    bool m_geometryValid = false;
    qint64 m_geomRangeStart = 0;
    qint64 m_geomRangeEnd = 0;
    int m_geomWidth = -1;

    QCanvasImage m_noteIcon;
};

} // namespace Timeline
