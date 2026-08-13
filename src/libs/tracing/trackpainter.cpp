// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "trackpainter.h"

#include "timelinemodel.h"

#include <utils/theme/theme.h>

#include <QCanvasPainter>
#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QWheelEvent>

namespace Timeline {

TrackPainter::TrackPainter(QWidget *parent)
    : QCanvasPainterWidget(parent)
{
    setMouseTracking(true);
    setFillColor(Utils::creatorColor(Utils::Theme::Token_Background_Default));
}

QSize TrackPainter::sizeHint() const
{
    // The widget fills the viewport; its size is driven externally. The hint is
    // only a sensible fallback.
    return QSize(200, TimelineModel::defaultRowHeight());
}

void TrackPainter::paint(QCanvasPainter *painter)
{
    // Time the CPU cost of producing this frame; this single widget renders all
    // tracks, so its paint() is the full-frame render time (see painted()).
    QElapsedTimer frameTimer;
    frameTimer.start();
    const QScopeGuard reportFrame([&] {
        emit painted(std::chrono::nanoseconds(frameTimer.nsecsElapsed()));
    });

    QCanvasPainter &p = *painter;

    ensureGeometry();

    const QColor bg1 = Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor1);
    const QColor bg2 = Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor2);
    const QColor divider = Utils::creatorColor(Utils::Theme::Timeline_DividerColor);
    const QColor outline = Utils::creatorColor(Utils::Theme::Token_Stroke_Subtle);
    const QColor handle = Utils::creatorColor(Utils::Theme::Timeline_HandleColor);
    const QColor highlight = Utils::creatorColor(Utils::Theme::Timeline_HighlightColor);

    // Replay the cached per-track geometry, translated into content space. No
    // event iteration happens here, so scrolling, hovering and selection (which
    // do not change the geometry) repaint almost for free.
    const QList<Track> &allTracks = tracks();
    for (int i = 0; i < allTracks.size(); ++i) {
        const Track &track = allTracks[i];
        if (!track.model)
            continue;
        const int top = track.yOffset - scrollOffset();
        const int trackH = track.model->height();
        if (top + trackH <= 0 || top >= height())
            continue; // fully scrolled out of view

        const TrackGeometry &g = m_geometry[i];
        p.save();
        p.setAntialias(0);
        p.translate(0.0f, float(top));
        if (g.hasBackground[0]) { p.setFillStyle(bg1); p.fill(g.background[0]); }
        if (g.hasBackground[1]) { p.setFillStyle(bg2); p.fill(g.background[1]); }
        if (g.hasGrid) { p.setFillStyle(divider); p.fill(g.grid); }
        if (g.hasOutlines) { p.setFillStyle(outline); p.fill(g.outlines); }
        p.setAntialias(1.0);
        for (const ColorPath &cp : g.fills) {
            p.setFillStyle(QColor::fromRgb(cp.color));
            p.fill(cp.path);
        }
        if (g.hasMarkers) { p.setFillStyle(handle); p.fill(g.markers); }
        if (g.hasNotes) { p.setFillStyle(highlight); p.fill(g.notes); }
        paintScaleOverlay(p, track);
        p.restore();
    }

    paintSelectionOverlay(p);
}

void TrackPainter::ensureGeometry()
{
    if (m_geometryValid && m_geomRangeStart == rangeStart() && m_geomRangeEnd == rangeEnd()
        && m_geomWidth == width())
        return;

    const QList<Track> &allTracks = tracks();
    m_geometry.clear();
    m_geometry.resize(allTracks.size());
    for (int i = 0; i < allTracks.size(); ++i)
        buildTrackGeometry(allTracks[i], m_geometry[i]);

    m_geomRangeStart = rangeStart();
    m_geomRangeEnd = rangeEnd();
    m_geomWidth = width();
    m_geometryValid = true;
}

// Converts the backend-independent neutral geometry into cached QCanvasPath
// objects, one draw call per group. ensureAttrCache is a base helper that
// populates the per-event attribute arrays the neutral build consumes.
void TrackPainter::buildTrackGeometry(const Track &track, TrackGeometry &geom) const
{
    ensureAttrCache(const_cast<Track &>(track));

    NeutralTrackGeometry neutral;
    buildNeutralGeometry(track, neutral);

    for (int phase = 0; phase < 2; ++phase) {
        for (const QRectF &r : neutral.background[phase])
            geom.background[phase].rect(r);
        geom.hasBackground[phase] = !neutral.background[phase].isEmpty();
    }

    for (const QRectF &r : std::as_const(neutral.grid))
        geom.grid.rect(r);
    geom.hasGrid = !neutral.grid.isEmpty();

    geom.fills.reserve(neutral.fills.size());
    for (const ColorRects &cr : std::as_const(neutral.fills)) {
        QCanvasPath path;
        for (const QRectF &r : cr.rects)
            path.rect(r);
        geom.fills.append({cr.color, std::move(path)});
    }

    for (const QRectF &r : neutral.outlines)
        geom.outlines.rect(r);
    geom.hasOutlines = neutral.outlines[0].isValid();

    for (const QRectF &r : std::as_const(neutral.markers))
        geom.markers.rect(r);
    geom.hasMarkers = !neutral.markers.isEmpty();

    for (const QRectF &r : std::as_const(neutral.noteSticks))
        geom.notes.rect(r);
    for (const Circle &c : std::as_const(neutral.noteDots))
        geom.notes.circle(c.x, c.y, c.radius);
    geom.hasNotes = !neutral.noteSticks.isEmpty() || !neutral.noteDots.isEmpty();
}

// Value scale for expanded rows with min/max values. Drawn in track-local
// coordinates (the painter is already translated).
void TrackPainter::paintScaleOverlay(QCanvasPainter &p, const Track &track) const
{
    OverlayScale ov;
    buildScaleOverlay(track, ov);
    if (ov.labels.isEmpty() && ov.lines.isEmpty())
        return;

    static const int kFontPx = 8;

    p.save();
    QFont sf = font();
    sf.setPixelSize(kFontPx);
    p.setFont(sf);

    const QColor scaleDiv = Utils::creatorColor(Utils::Theme::Timeline_DividerColor);
    p.setFillStyle(scaleDiv);
    for (const QRectF &line : std::as_const(ov.lines))
        p.fillRect(line);

    const QColor scaleText = Utils::creatorColor(Utils::Theme::Timeline_TextColor);
    p.setFillStyle(scaleText);
    for (const ScaleLabel &label : std::as_const(ov.labels))
        p.fillText(label.text, label.x, label.baselineY);
    p.restore();
}

// Selection and hover borders, drawn on top of the track content in
// widget(content) space.
void TrackPainter::paintSelectionOverlay(QCanvasPainter &p) const
{
    for (const OverlayStroke &s : buildSelectionOverlay()) {
        p.setStrokeStyle(QColor::fromRgb(s.color));
        p.setLineWidth(s.lineWidth);
        p.strokeRect(s.rect);
    }
}

void TrackPainter::mouseMoveEvent(QMouseEvent *event)
{
    handleMouseMove(event->buttons(), event->pos(), event->globalPosition().toPoint());
}

void TrackPainter::mousePressEvent(QMouseEvent *event)
{
    handleMousePress(event->button(), event->globalPosition().toPoint());
}

void TrackPainter::mouseReleaseEvent(QMouseEvent *event)
{
    handleMouseRelease(event->button(), event->pos());
}

void TrackPainter::wheelEvent(QWheelEvent *event)
{
    handleWheel(event);
}

void TrackPainter::leaveEvent(QEvent *)
{
    handleLeave();
}

} // namespace Timeline
