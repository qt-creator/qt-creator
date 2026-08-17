// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "trackpainterraster.h"

#include "timelinemodel.h"

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScopeGuard>
#include <QWheelEvent>

namespace Timeline {

TrackPainterRaster::TrackPainterRaster(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

QSize TrackPainterRaster::sizeHint() const
{
    // The widget fills the viewport; its size is driven externally. The hint is
    // only a sensible fallback.
    return QSize(200, TimelineModel::defaultRowHeight());
}

void TrackPainterRaster::paintEvent(QPaintEvent *event)
{
    // Time the CPU cost of producing this frame; this single widget renders all
    // tracks, so its paint is the full-frame render time (see painted()).
    QElapsedTimer frameTimer;
    frameTimer.start();
    const QScopeGuard reportFrame([&] {
        emit painted(std::chrono::nanoseconds(frameTimer.nsecsElapsed()));
    });

    QPainter p(this);

    ensureGeometry();

    const QColor trackBg1 = Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor1);
    const QColor trackBg2 = Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor2);
    const QColor divider = Utils::creatorColor(Utils::Theme::Timeline_DividerColor);
    const QColor outline = Utils::creatorColor(Utils::Theme::Token_Stroke_Subtle);
    const QColor handle = Utils::creatorColor(Utils::Theme::Timeline_HandleColor);

    p.fillRect(event->rect(), Utils::creatorColor(Utils::Theme::Token_Background_Default));

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
        p.translate(0.0, double(top));
        // Everything here is axis-aligned, so fill rectangles with fillRect (the
        // fast solid blitter) and leave antialiasing off: it only affects the
        // sub-pixel left/right edges of the bars and is far cheaper than routing
        // every rect through the antialiased path rasterizer. Only the round note
        // markers need antialiasing.
        p.setRenderHint(QPainter::Antialiasing, false);
        for (const QRectF &r : g.background[0]) p.fillRect(r, trackBg1);
        for (const QRectF &r : g.background[1]) p.fillRect(r, trackBg2);
        for (const QRectF &r : g.grid) p.fillRect(r, divider);
        for (const ColorRects &cr : g.fills) {
            const QColor c = QColor::fromRgb(cr.color);
            for (const QRectF &r : cr.rects)
                p.fillRect(r, c);
        }
        for (const QRectF &r : g.outlines) p.fillRect(r, outline);
        for (const QRectF &r : g.markers) p.fillRect(r, handle);
        if (!g.noteIcons.isEmpty()) {
            static const QIcon icon = noteIcon().icon();
            for (const QPoint &c : g.noteIcons) {
                icon.paint(&p, QRect(c.x() - kNoteIconSize / 2,
                                     c.y() - kNoteIconSize / 2,
                                     kNoteIconSize, kNoteIconSize));
            }
        }
        paintScaleOverlay(p, track);
        p.restore();
    }

    paintSelectionOverlay(p);
}

void TrackPainterRaster::ensureGeometry()
{
    if (m_geometryValid && m_geomRangeStart == rangeStart()
            && m_geomRangeEnd == rangeEnd() && m_geomWidth == width())
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

// Caches the backend-independent neutral geometry for one track. All the
// axis-aligned groups (backgrounds, grid, event bars, markers) are kept as plain
// rects and filled directly with fillRect at paint time (the fast solid blitter,
// far cheaper than the antialiased path rasterizer); note icons are kept as
// plain center points and painted as icons. ensureAttrCache is a base helper
// that populates the per-event attribute arrays the neutral build consumes.
void TrackPainterRaster::buildTrackGeometry(const Track &track, TrackGeometry &geom) const
{
    ensureAttrCache(const_cast<Track &>(track));

    NeutralTrackGeometry neutral;
    buildNeutralGeometry(track, neutral);

    geom.background[0] = std::move(neutral.background[0]);
    geom.background[1] = std::move(neutral.background[1]);
    geom.grid = std::move(neutral.grid);
    geom.outlines[0] = neutral.outlines[0];
    geom.outlines[1] = neutral.outlines[1];
    geom.markers = std::move(neutral.markers);
    geom.fills = std::move(neutral.fills);
    geom.noteIcons = std::move(neutral.noteIcons);
}

// Value scale for expanded rows with min/max values. Drawn in track-local
// coordinates (the painter is already translated).
void TrackPainterRaster::paintScaleOverlay(QPainter &p, const Track &track) const
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
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor scaleDiv = Utils::creatorColor(Utils::Theme::Timeline_DividerColor);
    for (const QRectF &line : std::as_const(ov.lines))
        p.fillRect(line, scaleDiv);

    const QColor scaleText = Utils::creatorColor(Utils::Theme::Timeline_TextColor);
    p.setPen(scaleText);
    for (const ScaleLabel &label : std::as_const(ov.labels))
        p.drawText(QPointF(label.x, label.baselineY), label.text);
    p.restore();
}

// Selection and hover borders, drawn on top of the track content in
// widget(content) space.
void TrackPainterRaster::paintSelectionOverlay(QPainter &p) const
{
    const QList<OverlayStroke> strokes = buildSelectionOverlay();
    if (strokes.isEmpty())
        return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(Qt::NoBrush);
    for (const OverlayStroke &s : strokes) {
        p.setPen(QPen(QColor::fromRgb(s.color), s.lineWidth, Qt::SolidLine, Qt::FlatCap,
                      Qt::MiterJoin));
        p.drawRect(s.rect);
    }
    p.restore();
}

void TrackPainterRaster::mouseMoveEvent(QMouseEvent *event)
{
    handleMouseMove(event->buttons(), event->pos(), event->globalPosition().toPoint());
}

void TrackPainterRaster::mousePressEvent(QMouseEvent *event)
{
    handleMousePress(event->button(), event->globalPosition().toPoint());
}

void TrackPainterRaster::mouseReleaseEvent(QMouseEvent *event)
{
    handleMouseRelease(event->button(), event->pos());
}

void TrackPainterRaster::wheelEvent(QWheelEvent *event)
{
    handleWheel(event);
}

void TrackPainterRaster::leaveEvent(QEvent *)
{
    handleLeave();
}

} // namespace Timeline
