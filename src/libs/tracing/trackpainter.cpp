// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "trackpainter.h"

#include "timelinecoordinates.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"

#include <utils/theme/theme.h>

#include <QCanvasPainter>
#include <QColor>
#include <QHash>
#include <QMouseEvent>
#include <QString>
#include <QVarLengthArray>
#include <QWheelEvent>

#include <cmath>
#include <limits>

namespace Timeline {

TrackPainter::TrackPainter(QWidget *parent)
    : QCanvasPainterWidget(parent)
{
    setMouseTracking(true);
}

void TrackPainter::setTracks(const QList<TimelineModel *> &models)
{
    m_tracks.clear();
    m_tracks.reserve(models.size());
    for (TimelineModel *model : models) {
        Track track;
        track.model = model;
        m_tracks.append(std::move(track));
    }
    rebuildGeometry();
    m_geometryValid = false;
    update();
}

void TrackPainter::rebuildGeometry()
{
    int y = 0;
    int totalRowsBefore = 0;
    for (Track &track : m_tracks) {
        track.yOffset = y;
        // Zebra parity continues across track boundaries (matches TimeMarks).
        track.startOdd = (totalRowsBefore % 2 == 0);
        if (track.model) {
            y += track.model->height();
            totalRowsBefore += track.model->rowCount();
        }
    }
    m_totalHeight = y;
}

void TrackPainter::refreshGeometry()
{
    rebuildGeometry();
    m_geometryValid = false;
    update();
}

TimelineModel *TrackPainter::trackModel(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < m_tracks.size())
        return m_tracks[trackIndex].model;
    return nullptr;
}

int TrackPainter::trackYOffset(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < m_tracks.size())
        return m_tracks[trackIndex].yOffset;
    return 0;
}

void TrackPainter::setNotes(const TimelineNotesModel *notes)
{
    if (m_notes == notes)
        return;
    if (m_notes)
        disconnect(m_notes, nullptr, this, nullptr);
    m_notes = notes;
    if (m_notes)
        connect(m_notes, &TimelineNotesModel::changed, this,
                [this] { m_geometryValid = false; update(); });
    m_geometryValid = false;
    update();
}

void TrackPainter::setMarkers(const QList<qint64> &markers)
{
    m_markers = markers;
    m_geometryValid = false;
    update();
}

void TrackPainter::setRange(qint64 rangeStart, qint64 rangeEnd)
{
    if (m_rangeStart == rangeStart && m_rangeEnd == rangeEnd)
        return;
    m_rangeStart = rangeStart;
    m_rangeEnd = rangeEnd;
    // Event/grid geometry is range-dependent; the cache is rebuilt lazily in
    // ensureGeometry() on the next paint (keyed on range + width).
    update();
}

void TrackPainter::setScrollOffset(int y)
{
    if (m_scrollOffset == y)
        return;
    m_scrollOffset = y;
    update();
}

void TrackPainter::setSelectedItem(int trackIndex, int itemIndex)
{
    if (m_selectedTrack == trackIndex && m_selectedItem == itemIndex)
        return;
    m_selectedTrack = trackIndex;
    m_selectedItem = itemIndex;
    update();
}

void TrackPainter::setHoveredItem(int trackIndex, int itemIndex)
{
    if (m_hoveredTrack == trackIndex && m_hoveredItem == itemIndex)
        return;
    m_hoveredTrack = trackIndex;
    m_hoveredItem = itemIndex;
    update();
}

void TrackPainter::setSelectionLocked(bool locked)
{
    if (m_selectionLocked == locked)
        return;
    m_selectionLocked = locked;
    update();
}

QSize TrackPainter::sizeHint() const
{
    // The widget fills the viewport; its size is driven externally. The hint is
    // only a sensible fallback.
    return QSize(200, TimelineModel::defaultRowHeight());
}

static QColor themeColor(Utils::Theme::Color role)
{
    if (Utils::creatorTheme())
        return Utils::creatorTheme()->color(role);
    return QColor();
}

// Matches QML TimeMarks prettyPrintScale/roundTo3Digits (units: " kMGT", base 1024).
static QString prettyPrintScale(qint64 amount)
{
    static const char units[] = " kMGT";
    const bool negative = amount < 0;
    double a = double(qAbs(amount));
    int unitIdx = 0;
    while (unitIdx + 1 < int(sizeof(units)) - 1 && a > 1024.0) {
        ++unitIdx;
        a /= 1024.0;
    }
    int decimals;
    if (a < 10.0) decimals = 2;
    else if (a < 100.0) decimals = 1;
    else decimals = 0;
    QString result = QString::number(a, 'f', decimals);
    if (negative)
        result.prepend(QLatin1Char('-'));
    if (units[unitIdx] != ' ')
        result.append(QLatin1Char(units[unitIdx]));
    return result;
}

int TrackPainter::trackAt(int contentY) const
{
    if (contentY < 0)
        return -1;
    for (int i = 0; i < m_tracks.size(); ++i) {
        const Track &t = m_tracks[i];
        if (!t.model)
            continue;
        if (contentY >= t.yOffset && contentY < t.yOffset + t.model->height())
            return i;
    }
    return -1;
}

void TrackPainter::paint(QCanvasPainter *painter)
{
    QCanvasPainter &p = *painter;

    ensureGeometry();

    // Striped background covering the whole viewport (continues below the last
    // track, like the QML striped background). Tracks overdraw their own rows.
    const QColor bg1 = themeColor(Utils::Theme::Timeline_BackgroundColor1);
    const QColor bg2 = themeColor(Utils::Theme::Timeline_BackgroundColor2);
    const int stripeH = qMax(1, TimelineModel::defaultRowHeight());
    for (int vy = -(m_scrollOffset % stripeH); vy < height(); vy += stripeH) {
        const int absRow = (vy + m_scrollOffset) / stripeH;
        p.setFillStyle((absRow % 2 == 0) ? bg1 : bg2);
        p.fillRect(0, vy, width(), stripeH);
    }

    const QColor divider   = themeColor(Utils::Theme::Timeline_DividerColor);
    const QColor handle     = themeColor(Utils::Theme::Timeline_HandleColor);
    const QColor highlight  = themeColor(Utils::Theme::Timeline_HighlightColor);

    // Replay the cached per-track geometry, translated into content space. No
    // event iteration happens here, so scrolling, hovering and selection (which
    // do not change the geometry) repaint almost for free.
    for (int i = 0; i < m_tracks.size(); ++i) {
        const Track &track = m_tracks[i];
        if (!track.model)
            continue;
        const int top = track.yOffset - m_scrollOffset;
        const int trackH = track.model->height();
        if (top + trackH <= 0 || top >= height())
            continue; // fully scrolled out of view

        const TrackGeometry &g = m_geometry[i];
        p.save();
        p.translate(0.0f, float(top));
        if (g.hasBackground[0]) { p.setFillStyle(bg1); p.fill(g.background[0]); }
        if (g.hasBackground[1]) { p.setFillStyle(bg2); p.fill(g.background[1]); }
        if (g.hasGrid) { p.setFillStyle(divider); p.fill(g.grid); }
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
    if (m_geometryValid && m_geomRangeStart == m_rangeStart
            && m_geomRangeEnd == m_rangeEnd && m_geomWidth == width())
        return;

    m_geometry.clear();
    m_geometry.resize(m_tracks.size());
    for (int i = 0; i < m_tracks.size(); ++i) {
        ensureAttrCache(m_tracks[i]);
        buildTrackGeometry(m_tracks[i], m_geometry[i]);
    }

    m_geomRangeStart = m_rangeStart;
    m_geomRangeEnd = m_rangeEnd;
    m_geomWidth = width();
    m_geometryValid = true;
}

void TrackPainter::ensureAttrCache(Track &track) const
{
    const TimelineModel *model = track.model;
    if (!model)
        return;
    const int n = model->count();
    // The track is recreated whenever expansion/colour/model content changes, so
    // a matching size means the cache is still valid for this content.
    if (track.rowCache.size() == n)
        return;
    track.rowCache.resize(n);
    track.colorCache.resize(n);
    track.relHeightCache.resize(n);
    for (int i = 0; i < n; ++i) {
        track.rowCache[i] = model->row(i);
        track.colorCache[i] = model->color(i);
        track.relHeightCache[i] = model->relativeHeight(i);
    }
}

// Builds the range-dependent fill geometry for one track in track-local
// coordinates (y = 0 at the track top). Called only when the cache is rebuilt.
void TrackPainter::buildTrackGeometry(const Track &track, TrackGeometry &geom) const
{
    const TimelineModel *model = track.model;
    if (!model || model->hidden())
        return;

    const int trackH = model->height();
    const int rowCount = model->rowCount();

    // Row backgrounds: one path per zebra color.
    for (int phase = 0; phase < 2; ++phase) {
        QCanvasPath &path = geom.background[phase];
        for (int row = 0; row < rowCount; ++row) {
            const bool odd = ((row % 2 == 0) == track.startOdd);
            if (odd != (phase == 0))
                continue;
            path.rect(0, model->rowOffset(row), width(), model->rowHeight(row));
            geom.hasBackground[phase] = true;
        }
    }

    const qint64 rangeDuration = m_rangeEnd - m_rangeStart;
    if (model->isEmpty() || rangeDuration <= 0 || width() == 0)
        return;

    // Vertical grid lines (same block geometry as TimeRuler) as 1px columns.
    {
        const double scale = double(width()) / double(rangeDuration);
        const qint64 timePerBlock = rulerBlockDuration(rangeDuration, double(width()));
        const double pixelsPerBlock = double(timePerBlock) * scale;
        const double pixelsPerSection = pixelsPerBlock / 5.0;
        const qint64 alignedStart = m_rangeStart - (m_rangeStart % timePerBlock);
        for (qint64 t = alignedStart; ; t += timePerBlock) {
            const double x = timeToPixel(t, m_rangeStart, m_rangeEnd, double(width()));
            if (x > double(width()))
                break;
            for (int s = 1; s <= 4; ++s) {
                const double sx = x + s * pixelsPerSection;
                if (sx >= 0.0 && sx <= double(width())) {
                    geom.grid.rect(qRound(sx), 0, 1, trackH);
                    geom.hasGrid = true;
                }
            }
            const double tickX = x + pixelsPerBlock;
            if (tickX >= 0.0 && tickX <= double(width())) {
                geom.grid.rect(qRound(tickX), 0, 1, trackH);
                geom.hasGrid = true;
            }
        }
    }

    // Density graphs: one path per row (all columns share the row color).
    if (model->rendersAsDensity()) {
        const int w = width();
        QList<float> columns;
        for (int row = 0; row < rowCount; ++row) {
            columns.assign(w, 0.0f);
            if (!model->fillDensityColumns(row, m_rangeStart, m_rangeEnd, columns))
                continue;
            const int rowH = model->rowHeight(row);
            const int rowY = model->rowOffset(row);
            QCanvasPath path;
            bool any = false;
            for (int x = 0; x < w; ++x) {
                const double frac = qBound(0.0f, columns[x], 1.0f);
                if (frac <= 0.0)
                    continue;
                const double itemH = rowH * frac;
                path.rect(QRectF(x, rowY + rowH - itemH, 1.0, itemH));
                any = true;
            }
            if (any)
                geom.fills.append({model->rowColor(row), std::move(path)});
        }
    } else {
    // Events grouped by color so each distinct color fills in one draw call.
    // Drawn event rects never overlap (per-row coverage skipping keeps them
    // disjoint horizontally, rows are disjoint vertically), so per-color draw
    // order is visually irrelevant.
    const int first = model->firstIndex(m_rangeStart);
    const int last = model->lastIndex(m_rangeEnd);

    if (first >= 0 && last >= first) {
        // Per-row "next available x": skip events entirely covered by an earlier draw in the
        // same row. Initialized to -1 so the first event in every row always passes.
        QVarLengthArray<double, 64> rowNextX(rowCount);
        std::fill(rowNextX.begin(), rowNextX.end(), -1.0);

        const double wf = double(width());
        // Precompute the pixels-per-time scale once so the per-event hot loop uses
        // a multiply instead of a division (timeToPixel divides every call).
        const double scale = wf / double(rangeDuration);
        const bool expanded = model->expanded();

        // Cache per-row geometry once instead of calling the (range-independent)
        // rowHeight/rowOffset accessors for every drawn event.
        QVarLengthArray<int, 64> rowH(rowCount), rowY(rowCount);
        for (int r = 0; r < rowCount; ++r) {
            rowH[r] = model->rowHeight(r);
            rowY[r] = model->rowOffset(r);
        }

        // Per-row "open" run coalescing, to cut the rect count the GPU has to
        // tessellate each frame:
        //   * contiguous events with the exact same color and height always
        //     merge (pixel-identical), and
        //   * while a run is still narrower than one pixel, contiguous sub-pixel
        //     events are absorbed into it regardless of color (the run keeps the
        //     first event's color/height).
        // The sub-pixel rule only fires when both the run and the incoming event
        // are < 1px wide, so when zoomed in (events span pixels) nothing is
        // merged across colors and the output is exact; when zoomed out, dense
        // sub-pixel events collapse to ~one rect per pixel column. A gap always
        // flushes the run. The run takes the color and height of its widest
        // (most significant) event, so the dominant activity shows through.
        struct OpenRun { bool active = false; QRgb color; double y; double h; double x0; double x1; double domW; };
        QVarLengthArray<OpenRun, 64> open(rowCount);
        for (OpenRun &o : open)
            o.active = false;

        QHash<QRgb, QCanvasPath> pathByColor;
        const auto flush = [&](OpenRun &o) {
            if (o.active) {
                pathByColor[o.color].rect(QRectF(o.x0, o.y, o.x1 - o.x0, o.h));
                o.active = false;
            }
        };

        const int *rowCache = track.rowCache.constData();
        const QRgb *colorCache = track.colorCache.constData();
        const float *relHeightCache = track.relHeightCache.constData();

        for (int i = first; i <= last; ++i) {
            const int row = rowCache[i];

            double x1 = double(model->startTime(i) - m_rangeStart) * scale;
            double x2 = double(model->endTime(i) - m_rangeStart) * scale;
            if (x2 - x1 < 1.0)
                x2 = x1 + 1.0;

            if (x2 < 0.0 || x1 > wf)
                continue;

            x1 = qMax(x1, 0.0);
            x2 = qMin(x2, wf);

            // Skip if entirely covered by a previous event in this row (expanded
            // mode only, matching the original; collapsed mode keeps overdraw so
            // per-event colors are preserved).
            if (x2 <= rowNextX[row] && expanded)
                continue;
            const double drawX1 = qMax(x1, rowNextX[row]);
            rowNextX[row] = x2;

            // Nothing visible (fully covered / zero width): skip the color()
            // lookup, but keep the advanced coverage above.
            if (x2 - drawX1 <= 0.0)
                continue;

            const QRgb color = colorCache[i];
            const double itemH = rowH[row] * relHeightCache[i];
            const double itemY = rowY[row] + rowH[row] - itemH;

            const double drawW = x2 - drawX1;
            OpenRun &o = open[row];
            const bool contiguous = o.active && drawX1 <= o.x1;
            const bool sameStyle = o.active && o.color == color && o.y == itemY && o.h == itemH;
            const bool subPixelAbsorb = contiguous && drawW < 1.0 && (o.x1 - o.x0) < 1.0;
            if (contiguous && (sameStyle || subPixelAbsorb)) {
                o.x1 = qMax(o.x1, x2);
                // The widest event in the run wins its color/height.
                if (drawW > o.domW) {
                    o.domW = drawW;
                    o.color = color;
                    o.y = itemY;
                    o.h = itemH;
                }
            } else {
                flush(o);
                o = {true, color, itemY, itemH, drawX1, x2, drawW};
            }
        }

        for (OpenRun &o : open)
            flush(o);

        geom.fills.reserve(pathByColor.size());
        for (auto it = pathByColor.begin(); it != pathByColor.end(); ++it)
            geom.fills.append({it.key(), std::move(it.value())});
    }
    }

    // Time ruler marker lines (2px wide filled columns).
    if (!m_markers.isEmpty()) {
        for (qint64 ts : m_markers) {
            const double mx = timeToPixel(ts, m_rangeStart, m_rangeEnd, double(width()));
            geom.markers.rect(QRectF(mx - 1.0, 0.0, 2.0, trackH));
            geom.hasMarkers = true;
        }
    }

    // Note markers: exclamation mark shape matching the QSG notes shader.
    // The shader draws d < 2/3 (stick) and d > 5/6 (dot), masking the gap between them.
    if (m_notes) {
        const int modelId = model->modelId();
        for (int i = 0; i < m_notes->count(); ++i) {
            if (m_notes->timelineModel(i) != modelId)
                continue;
            const int idx = m_notes->timelineIndex(i);
            if (model->startTime(idx) > m_rangeEnd || model->endTime(idx) < m_rangeStart)
                continue;
            const int row = model->row(idx);
            const double rowH = model->rowHeight(row);
            const double rowY = model->rowOffset(row);
            const qint64 center = (model->startTime(idx) + model->endTime(idx)) / 2;
            const double cx = timeToPixel(center, m_rangeStart, m_rangeEnd, double(width()));
            const double span = 0.8 * rowH;
            const double top = rowY + 0.1 * rowH;
            const double stickEnd = top + (2.0 / 3.0) * span;
            const double dotStart = top + (5.0 / 6.0) * span;
            const double dotEnd   = top + span;
            // Stick: a 3px wide filled column. Dot: a small filled disc.
            geom.notes.rect(QRectF(cx - 1.5, top, 3.0, stickEnd - top));
            geom.notes.circle(float(cx), float((dotStart + dotEnd) / 2.0), 1.5f);
            geom.hasNotes = true;
        }
    }
}

// Value scale for expanded rows with min/max values (TimeMarks.qml equivalent).
// Drawn in track-local coordinates (the painter is already translated).
void TrackPainter::paintScaleOverlay(QCanvasPainter &p, const Track &track) const
{
    const TimelineModel *model = track.model;
    if (!model || !model->expanded())
        return;

    static const int kScaleMinH = 60;
    static const int kScaleStep = 30;
    static const int kFontPx = 8;
    static const int kMarg = 2;

    p.save();
    QFont sf = font();
    sf.setPixelSize(kFontPx);
    p.setFont(sf);
    const QColor scaleText = themeColor(Utils::Theme::Timeline_TextColor);
    const QColor scaleDiv  = themeColor(Utils::Theme::Timeline_DividerColor);

    const int rowCount = model->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const int rowH = model->rowHeight(row);
        const int rowY = model->rowOffset(row);
        if (rowH < kScaleMinH)
            continue;
        const qint64 minVal = model->rowMinValue(row);
        const qint64 maxVal = model->rowMaxValue(row);
        if (maxVal <= minVal)
            continue;
        const qint64 valDiff = maxVal - minVal;

        const int numSteps = qMax(1, rowH / kScaleStep);
        double ugly = std::ceil(double(valDiff) / double(numSteps));
        qint64 stepVal = 1;
        while (ugly > 1.0) {
            ugly /= 2.0;
            stepVal *= 2;
        }
        const double stepH = double(stepVal) * double(rowH) / double(valDiff);
        const int topLabelBottom = rowY + kFontPx + kMarg * 2 + 2;

        p.setFillStyle(scaleText);
        p.fillText(prettyPrintScale(maxVal), float(kMarg), float(topLabelBottom));

        const int numLines = int(valDiff / stepVal);
        for (int i = 0; i < numLines; ++i) {
            if (rowY + qRound(double(rowH) - double(i + 1) * stepH) <= topLabelBottom)
                break;
            const int lineY = rowY + rowH - qRound(double(i) * stepH);
            p.setFillStyle(scaleDiv);
            p.fillRect(0, lineY, width(), 1);
            p.setFillStyle(scaleText);
            p.fillText(prettyPrintScale(minVal + qint64(i) * stepVal),
                       float(kMarg), float(lineY - kMarg));
        }
    }
    p.restore();
}

// Selection and hover borders, drawn on top of the track content. The painter
// is in widget(content) space here, so the track's scroll-adjusted top is added.
void TrackPainter::paintSelectionOverlay(QCanvasPainter &p) const
{
    if (m_rangeStart >= m_rangeEnd)
        return;

    const QColor selectionColor = m_selectionLocked ? QColor(96, 0, 255) : QColor(Qt::blue);
    const QColor hoverColor(Qt::white);

    const auto paintOverlay = [&](int trackIndex, int idx) {
        if (trackIndex < 0 || trackIndex >= m_tracks.size() || idx < 0)
            return;
        const Track &track = m_tracks[trackIndex];
        const TimelineModel *model = track.model;
        if (!model)
            return;
        const int topPx = track.yOffset - m_scrollOffset;
        const int row = model->row(idx);
        const int rowH = model->rowHeight(row);
        const int rowY = model->rowOffset(row);
        const double relH = model->relativeHeight(idx);
        const double itemH = rowH * relH;
        const double itemY = topPx + rowY + rowH - itemH;
        const qint64 start = model->startTime(idx);
        const qint64 end = model->endTime(idx);
        double x1 = timeToPixel(start, m_rangeStart, m_rangeEnd, double(width()));
        double x2 = timeToPixel(end, m_rangeStart, m_rangeEnd, double(width()));
        if (x2 - x1 < 1.0)
            x2 = x1 + 1.0;
        x1 = qMax(x1, 0.0);
        x2 = qMin(x2, double(width()));
        const QRectF itemRect(x1, itemY, x2 - x1, itemH);
        if (trackIndex == m_selectedTrack && idx == m_selectedItem) {
            p.setStrokeStyle(selectionColor);
            p.setLineWidth(4);
            p.strokeRect(itemRect.adjusted(2.0, 2.0, -2.0, -2.0));
        } else {
            p.setStrokeStyle(hoverColor);
            p.setLineWidth(1);
            p.strokeRect(itemRect.adjusted(0.5, 0.5, -0.5, -0.5));
        }
    };

    paintOverlay(m_hoveredTrack, m_hoveredItem);
    paintOverlay(m_selectedTrack, m_selectedItem);
}

// Hit test within a single model. local is in track-local coordinates.
int TrackPainter::indexInModel(const TimelineModel *model, const QPoint &local) const
{
    if (!model || model->isEmpty())
        return -1;
    const qint64 rangeDuration = m_rangeEnd - m_rangeStart;
    if (rangeDuration <= 0 || width() == 0)
        return -1;

    const qint64 t = pixelToTime(local.x(), double(width()), m_rangeStart, m_rangeEnd);

    const int candidate = model->bestIndex(t);
    if (candidate < 0)
        return -1;

    // Search outward from the candidate for the smallest item whose drawn
    // rectangle contains the cursor. Items are ordered by start time, not by
    // row, so the item under the cursor (a long parent range, or an item on a
    // different row) can be arbitrarily far from the candidate in index space.
    // A fixed-size window therefore misses such items; instead we expand until
    // the start-time ordering proves no further item can match.
    int best = -1;
    qint64 bestWidth = std::numeric_limits<qint64>::max();
    const int count = model->count();

    // Test whether item i is drawn under the cursor and, if so, whether it is
    // narrower than the current best (innermost nested item wins).
    const auto test = [&](int i) {
        const int row = model->row(i);
        const int rowH = model->rowHeight(row);
        const int rowY = model->rowOffset(row);

        const double relH = model->relativeHeight(i);
        const double itemH = rowH * relH;
        const double itemY = rowY + rowH - itemH;

        if (local.y() < itemY || local.y() >= itemY + itemH)
            return;

        const qint64 start = model->startTime(i);
        const qint64 end = model->endTime(i);

        double x1 = timeToPixel(start, m_rangeStart, m_rangeEnd, double(width()));
        double x2 = timeToPixel(end, m_rangeStart, m_rangeEnd, double(width()));
        if (x2 - x1 < 1.0)
            x2 = x1 + 1.0;

        if (local.x() < x1 || local.x() >= x2)
            return;

        const qint64 w = end - start;
        if (best == -1 || w < bestWidth) {
            best = i;
            bestWidth = w;
        }
    };

    // Forward: start times only increase, so once an item starts to the right
    // of the cursor no later item can contain it.
    for (int i = candidate; i < count; ++i) {
        if (timeToPixel(model->startTime(i), m_rangeStart, m_rangeEnd, double(width())) > local.x())
            break;
        test(i);
    }

    // Backward: a short item that doesn't reach the cursor may still have a
    // long parent that does. Only stop once neither the item nor its parent can
    // cover the cursor time.
    for (int i = candidate - 1; i >= 0; --i) {
        const qint64 end = model->endTime(i);
        if (end < t) {
            const int parent = model->parentIndex(i);
            const qint64 parentEnd = parent == -1 ? end : model->endTime(parent);
            if (parentEnd < t)
                break;
        }
        test(i);
    }

    return best;
}

void TrackPainter::itemAt(const QPoint &pos, int *trackIndex, int *itemIndex) const
{
    *trackIndex = -1;
    *itemIndex = -1;
    const int contentY = pos.y() + m_scrollOffset;
    const int track = trackAt(contentY);
    if (track < 0)
        return;
    const Track &t = m_tracks[track];
    const QPoint local(pos.x(), contentY - t.yOffset);
    const int item = indexInModel(t.model, local);
    *trackIndex = track;
    *itemIndex = item;
}

void TrackPainter::mouseMoveEvent(QMouseEvent *event)
{
    // Use global position so that verticalPan-induced widget movement doesn't
    // corrupt the delta: widget-local coords shift when the scroll area moves
    // the widget under the cursor, causing a feedback loop.
    const QPoint globalPos = event->globalPosition().toPoint();
    if ((event->buttons() & Qt::LeftButton) && !m_panning) {
        const int dx = globalPos.x() - m_pressPos.x();
        const int dy = globalPos.y() - m_pressPos.y();
        if (qAbs(dx) + qAbs(dy) > 4)
            m_panning = true;
    }

    if (m_panning) {
        const int dx = globalPos.x() - m_pressPos.x();
        const int dy = globalPos.y() - m_pressPos.y();
        m_pressPos = globalPos;
        if (dx != 0)
            emit horizontalPan(dx);
        if (dy != 0)
            emit verticalPan(dy);
        return;
    }

    int track = -1, item = -1;
    itemAt(event->pos(), &track, &item);
    if (track != m_hoveredTrack || item != m_hoveredItem) {
        m_hoveredTrack = track;
        m_hoveredItem = item;
        update();
        emit itemHovered(track, item);
    }
}

void TrackPainter::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressPos = event->globalPosition().toPoint();
        m_panning = false;
    }
}

void TrackPainter::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_panning) {
            int track = -1, item = -1;
            itemAt(event->pos(), &track, &item);
            emit itemClicked(track, item);
        }
        m_panning = false;
    }
}

void TrackPainter::wheelEvent(QWheelEvent *event)
{
    const QPoint pixelDelta = event->pixelDelta();
    const QPoint angleDelta = event->angleDelta();

    if (event->modifiers() & Qt::ControlModifier) {
        const int dy = pixelDelta.y() != 0 ? pixelDelta.y() : angleDelta.y() / 8;
        if (dy == 0) {
            event->ignore();
            return;
        }
        emit zoomRequested(event->position().x(), dy);
        event->accept();
        return;
    }

    const int dx = pixelDelta.x() != 0 ? pixelDelta.x() : angleDelta.x() / 8;
    if (dx != 0) {
        emit horizontalPan(-dx);
        event->accept();
    } else {
        // Vertical wheel falls through to the scroll area for vertical scrolling.
        event->ignore();
    }
}

void TrackPainter::leaveEvent(QEvent *)
{
    if (m_hoveredTrack != -1 || m_hoveredItem != -1) {
        m_hoveredTrack = -1;
        m_hoveredItem = -1;
        update();
        emit itemHovered(-1, -1);
    }
}

} // namespace Timeline
