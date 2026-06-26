// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QCanvasPainterWidget>
#include <QCanvasPath>
#include <QList>
#include <QRgb>

QT_BEGIN_NAMESPACE
class QCanvasPainter;
QT_END_NAMESPACE

namespace Timeline {

class TimelineModel;
class TimelineNotesModel;

// Renders the whole track area (all visible tracks stacked vertically) into a
// single hardware-accelerated QCanvasPainter surface. The widget fills the
// scroll-area viewport and scrolls its content by an internal pixel offset
// rather than being a tall widget moved by the scroll area; this keeps the GPU
// render target viewport-sized regardless of how many tracks there are.
class TRACING_EXPORT TrackPainter : public QCanvasPainterWidget
{
    Q_OBJECT
public:
    explicit TrackPainter(QWidget *parent = nullptr);

    // The ordered list of visible track models. Per-track Y offsets and zebra
    // parity are recomputed and the total content height is updated.
    void setTracks(const QList<TimelineModel *> &models);
    void refreshGeometry(); // recompute offsets/total height after row sizes change

    int trackCount() const { return m_tracks.size(); }
    TimelineModel *trackModel(int trackIndex) const;
    int trackYOffset(int trackIndex) const; // absolute Y of the track top in content space
    int totalHeight() const { return m_totalHeight; }

    void setNotes(const TimelineNotesModel *notes);
    void setMarkers(const QList<qint64> &markers);

    void setRange(qint64 rangeStart, qint64 rangeEnd);
    qint64 rangeStart() const { return m_rangeStart; }
    qint64 rangeEnd() const { return m_rangeEnd; }

    void setScrollOffset(int y);
    int scrollOffset() const { return m_scrollOffset; }

    void setSelectedItem(int trackIndex, int itemIndex); // -1 = none
    void setHoveredItem(int trackIndex, int itemIndex);   // -1 = none
    void setSelectionLocked(bool locked);

    // Hit test in widget(viewport)-local coordinates. Writes the track and item
    // index under the cursor, or -1 when nothing is hit.
    void itemAt(const QPoint &pos, int *trackIndex, int *itemIndex) const;

    QSize sizeHint() const override;

signals:
    void itemHovered(int trackIndex, int itemIndex);
    void itemClicked(int trackIndex, int itemIndex);
    void horizontalPan(int dx);
    void verticalPan(int dy);
    void zoomRequested(double cursorX, int dy);

protected:
    void paint(QCanvasPainter *painter) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    struct Track {
        TimelineModel *model = nullptr;
        int yOffset = 0;     // absolute top within the content
        bool startOdd = true;
        // Per-event attributes cached as flat arrays. row()/color()/relativeHeight()
        // are virtual and range-independent, so caching them avoids a few million
        // virtual calls per frame while panning/zooming. Rebuilt whenever the track
        // set is rebuilt (which is what an expand/colour/model change triggers).
        QList<int> rowCache;
        QList<QRgb> colorCache;
        QList<float> relHeightCache;
    };

    // Cached, range-dependent fill geometry for one track, built in track-local
    // coordinates and replayed each frame. Rebuilt only when the visible range,
    // the widget width, or the track set changes (not on scroll/hover/select).
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
        QCanvasPath markers;
        bool hasMarkers = false;
        QCanvasPath notes;
        bool hasNotes = false;
    };

    void rebuildGeometry();
    int trackAt(int contentY) const; // index of the track containing content-Y, or -1
    int indexInModel(const TimelineModel *model, const QPoint &local) const;
    void ensureGeometry();           // rebuild the cache if the range/width changed
    void ensureAttrCache(Track &track) const; // populate per-event attribute arrays
    void buildTrackGeometry(const Track &track, TrackGeometry &geom) const;
    void paintScaleOverlay(QCanvasPainter &p, const Track &track) const;
    void paintSelectionOverlay(QCanvasPainter &p) const;

    QList<Track> m_tracks;
    QList<TrackGeometry> m_geometry;  // parallel to m_tracks; valid for the cache key below
    bool m_geometryValid = false;
    qint64 m_geomRangeStart = 0;
    qint64 m_geomRangeEnd = 0;
    int m_geomWidth = -1;
    const TimelineNotesModel *m_notes = nullptr;
    QList<qint64> m_markers;
    qint64 m_rangeStart = 0;
    qint64 m_rangeEnd = 0;
    int m_scrollOffset = 0;
    int m_totalHeight = 0;
    int m_selectedTrack = -1;
    int m_selectedItem = -1;
    int m_hoveredTrack = -1;
    int m_hoveredItem = -1;
    bool m_selectionLocked = true;

    QPoint m_pressPos;
    bool m_panning = false;
};

} // namespace Timeline
