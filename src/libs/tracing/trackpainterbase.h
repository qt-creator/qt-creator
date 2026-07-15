// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QList>
#include <QMetaObject>
#include <QPoint>
#include <QRectF>
#include <QRgb>
#include <QString>

QT_BEGIN_NAMESPACE
class QWheelEvent;
class QWidget;
QT_END_NAMESPACE

namespace Timeline {

class TimelineModel;
class TimelineNotesModel;

// Which rendering backend the track area uses.
enum class TrackBackend {
    Automatic, // resolve per host OS (GPU on macOS/Windows, software on Linux)
    Gpu,       // QCanvasPainter (RHI) -> TrackPainter
    Software,  // QPainter          -> TrackPainterRaster
};

// Global override for the track backend. Automatic (the default) resolves per
// host OS in resolvedTrackBackend(). Read when a track area is created, so a
// change only affects track areas created afterwards.
TRACING_EXPORT void setTrackBackendOverride(TrackBackend backend);
TRACING_EXPORT TrackBackend trackBackendOverride();
// Automatic resolved to a concrete backend for this host OS.
TRACING_EXPORT TrackBackend resolvedTrackBackend();

// Backend-independent core of the track area: track bookkeeping, the per-event
// attribute cache, hit testing, mouse/wheel interaction and the range-dependent
// *neutral* geometry (plain rects/circles, no backend path type). The two
// concrete widgets - TrackPainter (QCanvasPainter/RHI) and TrackPainterRaster
// (QPainter) - derive from this together with their respective QWidget base and
// only add backend path caching plus the actual draw calls. Keeping the geometry
// decisions here single-sources the pixels both backends produce.
//
// This is a non-QObject mixin: the two backends have different QWidget-derived
// bases (QCanvasPainterWidget vs QWidget) and so cannot share a common QObject
// ancestor. Signals therefore live in the concrete widgets, which implement the
// notify*() hooks below to emit them.
class TRACING_EXPORT TrackPainterBase
{
public:
    virtual ~TrackPainterBase();

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

    // The concrete QWidget (for parenting, sizing and as a signal/connect
    // context). Implemented by each backend to return itself.
    virtual QWidget *widget() = 0;
    const QWidget *widget() const { return const_cast<TrackPainterBase *>(this)->widget(); }

    // Which backend this concrete widget is.
    virtual TrackBackend backend() const = 0;

protected:
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

    // Range-dependent fill geometry for one track, expressed as plain
    // rects/circles in track-local coordinates (y = 0 at the track top). Each
    // backend converts this into its own path type once per rebuild and replays
    // the paths every frame. Colours that come from the theme are applied by the
    // backend (see the role comments); per-event fill colours are baked in here.
    struct ColorRects {
        QRgb color;
        QList<QRectF> rects;
    };
    struct Circle {
        float x;
        float y;
        float radius;
    };
    struct NeutralTrackGeometry {
        QList<QRectF> background[2]; // [0] = bg1 rows, [1] = bg2 rows
        QList<QRectF> grid;          // Timeline_DividerColor
        QList<ColorRects> fills;     // event bars or density columns, grouped by colour
        QList<QRectF> markers;       // Timeline_HandleColor
        QList<QRectF> noteSticks;    // Timeline_HighlightColor
        QList<Circle> noteDots;      // Timeline_HighlightColor
    };

    // Viewport-filling zebra background (continues below the last track). Drawn
    // in widget space, not cached.
    struct Stripe {
        QRectF rect;
        bool bg1; // true -> Timeline_BackgroundColor1, false -> ...Color2
    };

    // Value-scale overlay primitives for one expanded track, in track-local
    // coordinates. labels use Timeline_TextColor, lines use Timeline_DividerColor.
    struct ScaleLabel {
        QString text;
        float x;
        float baselineY;
    };
    struct OverlayScale {
        QList<ScaleLabel> labels;
        QList<QRectF> lines; // 1px full-width divider rects
    };

    // Selection/hover borders, in widget(content) space, colour and line width
    // already resolved.
    struct OverlayStroke {
        QRectF rect;
        QRgb color;
        float lineWidth;
    };

    // --- Shared geometry / overlay computation (single-sourced) ---------------
    void ensureAttrCache(Track &track) const;
    void buildNeutralGeometry(const Track &track, NeutralTrackGeometry &geom) const;
    QList<Stripe> backgroundStripes() const;
    void buildScaleOverlay(const Track &track, OverlayScale &out) const;
    QList<OverlayStroke> buildSelectionOverlay() const;

    // --- Interaction (called by the concrete widgets' event handlers) ---------
    void handleMousePress(int button, const QPoint &globalPos);
    void handleMouseMove(int buttons, const QPoint &localPos, const QPoint &globalPos);
    void handleMouseRelease(int button, const QPoint &localPos);
    void handleWheel(QWheelEvent *event);
    void handleLeave();

    // --- Hooks the concrete backends implement -------------------------------
    // Clear the backend's cached paths so they are rebuilt on the next paint.
    virtual void invalidateBackendGeometry() = 0;
    // Emit the corresponding Qt signal.
    virtual void notifyItemHovered(int trackIndex, int itemIndex) = 0;
    virtual void notifyItemClicked(int trackIndex, int itemIndex) = 0;
    virtual void notifyHorizontalPan(int dx) = 0;
    virtual void notifyVerticalPan(int dy) = 0;
    virtual void notifyZoomRequested(double cursorX, int dy) = 0;

    // Convenience accessors onto the concrete widget.
    int viewWidth() const;
    int viewHeight() const;
    void requestUpdate();

    // State accessed by the backends when building/replaying their path cache.
    const QList<Track> &tracks() const { return m_tracks; }
    int selectedTrack() const { return m_selectedTrack; }
    int selectedItem() const { return m_selectedItem; }

private:
    void rebuildGeometry();          // recompute offsets/total height
    int trackAt(int contentY) const; // index of the track containing content-Y, or -1
    int indexInModel(const TimelineModel *model, const QPoint &local) const;

    QList<Track> m_tracks;
    const TimelineNotesModel *m_notes = nullptr;
    QMetaObject::Connection m_notesConnection;
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
