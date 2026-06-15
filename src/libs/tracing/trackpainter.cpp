// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "trackpainter.h"

#include "timelinecoordinates.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"

#include <utils/theme/theme.h>

#include <QVarLengthArray>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QString>
#include <QWheelEvent>

#include <cmath>

namespace Timeline {

TrackPainter::TrackPainter(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TrackPainter::setModel(TimelineModel *model)
{
    m_model = model;
    invalidateCache();
    update();
}

void TrackPainter::setMarkers(const QList<qint64> &markers)
{
    m_markers = markers;
    invalidateCache();
    update();
}

void TrackPainter::setNotes(const TimelineNotesModel *notes)
{
    if (m_notes == notes)
        return;
    if (m_notes)
        disconnect(m_notes, nullptr, this, nullptr);
    m_notes = notes;
    if (m_notes)
        connect(m_notes, &TimelineNotesModel::changed, this, [this] {
            invalidateCache();
            update();
        });
    invalidateCache();
    update();
}

void TrackPainter::setRange(qint64 rangeStart, qint64 rangeEnd)
{
    if (m_rangeStart == rangeStart && m_rangeEnd == rangeEnd)
        return;
    const qint64 oldStart = m_rangeStart;
    const qint64 oldDuration = m_rangeEnd - m_rangeStart;
    const qint64 newDuration = rangeEnd - rangeStart;
    m_rangeStart = rangeStart;
    m_rangeEnd = rangeEnd;

    const bool purePan = (oldDuration == newDuration && oldDuration > 0);
    if (purePan && !m_cache.isNull() && width() > 0) {
        const qint64 timeDelta = oldStart - rangeStart;
        const qint64 duration = m_rangeEnd - m_rangeStart;
        const qint64 width64 = static_cast<qint64>(width());

        // Use 64-bit integer math: (delta * width) / duration
        // This is mathematically identical to (delta / duration) * width
        // but avoids all floating point precision issues and is much faster.
        // We use qint64 to ensure the multiplication doesn't overflow.
        const qint64 shiftPx_64 = (timeDelta * width64) / duration;

        // Check if the jump is within bounds using 64-bit math to avoid qAbs assertion
        if (qAbs(static_cast<qint64>(m_pendingShiftPx) + shiftPx_64) < width64) {
            m_pendingShiftPx = static_cast<int>(shiftPx_64);
            update();
            return;
        }
    }

    invalidateCache();
    update();
}

void TrackPainter::setSelectedItem(int index)
{
    if (m_selectedItem == index)
        return;
    m_selectedItem = index;
    update();
}

void TrackPainter::setHoveredItem(int index)
{
    if (m_hoveredItem == index)
        return;
    m_hoveredItem = index;
    update();
}

void TrackPainter::setSelectionLocked(bool locked)
{
    if (m_selectionLocked == locked)
        return;
    m_selectionLocked = locked;
    update();
}

void TrackPainter::setStartOdd(bool startOdd)
{
    if (m_startOdd == startOdd)
        return;
    m_startOdd = startOdd;
    invalidateCache();
    update();
}

QSize TrackPainter::sizeHint() const
{
    int h = m_model ? m_model->height() : TimelineModel::defaultRowHeight();
    return QSize(200, h);
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

// First item index whose row() equals `row`, or -1. Used to read a density
// row's constant colour from the model's public color(int) API.
int TrackPainter::firstItemInRow(int row) const
{
    const int count = m_model->count();
    for (int i = 0; i < count; ++i) {
        if (m_model->row(i) == row)
            return i;
    }
    return -1;
}

void TrackPainter::renderContent(QPainter &p, qint64 iterStart, qint64 iterEnd) const
{
    const QColor bg1 = themeColor(Utils::Theme::Timeline_BackgroundColor1);
    const QColor bg2 = themeColor(Utils::Theme::Timeline_BackgroundColor2);

    if (!m_model || m_model->hidden()) {
        p.fillRect(0, 0, width(), height(), bg1);
        return;
    }

    const int rowCount = m_model->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const int rowY = m_model->rowOffset(row);
        const int rowH = m_model->rowHeight(row);
        p.fillRect(0, rowY, width(), rowH, ((row % 2 == 0) == m_startOdd) ? bg1 : bg2);
    }

    const qint64 rangeDuration = m_rangeEnd - m_rangeStart;
    if (m_model->isEmpty() || rangeDuration <= 0 || width() == 0)
        return;

    // Vertical grid lines (same block geometry as TimeRuler)
    {
        const double scale = double(width()) / double(rangeDuration);
        const qint64 timePerBlock = rulerBlockDuration(rangeDuration, double(width()));
        const double pixelsPerBlock = double(timePerBlock) * scale;
        const double pixelsPerSection = pixelsPerBlock / 5.0;
        const qint64 alignedStart = m_rangeStart - (m_rangeStart % timePerBlock);
        const QColor gridColor = themeColor(Utils::Theme::Timeline_DividerColor);
        p.setPen(QPen(gridColor, 1));
        for (qint64 t = alignedStart; ; t += timePerBlock) {
            const double x = timeToPixel(t, m_rangeStart, m_rangeEnd, double(width()));
            if (x > double(width()))
                break;
            for (int s = 1; s <= 4; ++s) {
                const double sx = x + s * pixelsPerSection;
                if (sx >= 0.0 && sx <= double(width()))
                    p.drawLine(qRound(sx), 0, qRound(sx), height() - 1);
            }
            const double tickX = x + pixelsPerBlock;
            if (tickX >= 0.0 && tickX <= double(width()))
                p.drawLine(qRound(tickX), 0, qRound(tickX), height() - 1);
        }
    }

    // Density graphs (e.g. CPU usage) fill each pixel column by the activity in
    // the time it covers, rather than drawing one bar per item. The density
    // branch ignores the [iterStart, iterEnd] strip range and renders every
    // column across [m_rangeStart, m_rangeEnd]; the painter's clip rect limits
    // what is shown during a strip (pan) render, like the value-scale block.
    if (m_model->rendersAsDensity()) {
        const int w = width();
        QList<float> columns;
        for (int row = 0; row < rowCount; ++row) {
            columns.assign(w, 0.0f);
            if (!m_model->fillDensityColumns(row, m_rangeStart, m_rangeEnd, columns))
                continue;
            const int rowH = m_model->rowHeight(row);
            const int rowY = m_model->rowOffset(row);
            // Constant per-row colour: use the first item that maps to this row.
            QColor rowColor = QColor::fromRgb(0xff808080);
            const int firstRowItem = firstItemInRow(row);
            if (firstRowItem >= 0)
                rowColor = QColor::fromRgb(m_model->color(firstRowItem));
            for (int x = 0; x < w; ++x) {
                const double frac = qBound(0.0f, columns[x], 1.0f);
                if (frac <= 0.0)
                    continue;
                const double itemH = rowH * frac;
                p.fillRect(QRectF(x, rowY + rowH - itemH, 1.0, itemH), rowColor);
            }
        }
    } else {
    // Events (fills only; selection/hover borders are overlaid live in paintEvent)
    const int first = m_model->firstIndex(iterStart);
    const int last = m_model->lastIndex(iterEnd);

    if (first >= 0 && last >= first) {
        // Per-row "next available x": skip events entirely covered by an earlier draw in the
        // same row. Initialized to -1 so the first event in every row always passes.
        QVarLengthArray<double, 64> rowNextX(rowCount);
        std::fill(rowNextX.begin(), rowNextX.end(), -1.0);

        const double wf = double(width());

        for (int i = first; i <= last; ++i) {
            const int row = m_model->row(i);
            const qint64 start = m_model->startTime(i);
            const qint64 end   = m_model->endTime(i);

            double x1 = timeToPixel(start, m_rangeStart, m_rangeEnd, wf);
            double x2 = timeToPixel(end,   m_rangeStart, m_rangeEnd, wf);
            if (x2 - x1 < 1.0)
                x2 = x1 + 1.0;

            if (x2 < 0.0 || x1 > wf)
                continue;

            x1 = qMax(x1, 0.0);
            x2 = qMin(x2, wf);

            // Skip if entirely covered by a previous event in this row.
            if (x2 <= rowNextX[row] && m_model->expanded())
                continue;
            const double drawX1 = qMax(x1, rowNextX[row]);
            rowNextX[row] = x2;

            const int rowH = m_model->rowHeight(row);
            const int rowY = m_model->rowOffset(row);
            const double relH = m_model->relativeHeight(i);
            const double itemH = rowH * relH;
            const double itemY = rowY + rowH - itemH;

            p.fillRect(QRectF(drawX1, itemY, x2 - drawX1, itemH),
                       QColor::fromRgb(m_model->color(i)));
        }
    }
    }

    // Time ruler marker lines
    if (!m_markers.isEmpty()) {
        const QColor handleColor = themeColor(Utils::Theme::Timeline_HandleColor);
        p.setPen(QPen(handleColor, 2));
        for (qint64 ts : m_markers) {
            const double mx = timeToPixel(ts, m_rangeStart, m_rangeEnd, double(width()));
            p.drawLine(QPointF(mx, 0), QPointF(mx, height()));
        }
    }

    // Note markers: exclamation mark shape matching the QSG notes shader.
    // The shader draws d < 2/3 (stick) and d > 5/6 (dot), masking the gap between them.
    if (m_notes && m_model) {
        const QColor noteColor = themeColor(Utils::Theme::Timeline_HighlightColor);
        p.setPen(QPen(noteColor, 3));
        const int modelId = m_model->modelId();
        for (int i = 0; i < m_notes->count(); ++i) {
            if (m_notes->timelineModel(i) != modelId)
                continue;
            const int idx = m_notes->timelineIndex(i);
            if (m_model->startTime(idx) > iterEnd || m_model->endTime(idx) < iterStart)
                continue;
            const int row = m_model->row(idx);
            const double rowH = m_model->rowHeight(row);
            const double rowY = m_model->rowOffset(row);
            const qint64 center = (m_model->startTime(idx) + m_model->endTime(idx)) / 2;
            const double cx = timeToPixel(center, m_rangeStart, m_rangeEnd, double(width()));
            const double span = 0.8 * rowH;
            const double top = rowY + 0.1 * rowH;
            const double stickEnd = top + (2.0 / 3.0) * span;
            const double dotStart = top + (5.0 / 6.0) * span;
            const double dotEnd   = top + span;
            p.drawLine(QPointF(cx, top), QPointF(cx, stickEnd));
            p.drawPoint(QPointF(cx, (dotStart + dotEnd) / 2.0));
        }
    }
}

// Value scale for expanded rows with min/max values (TimeMarks.qml equivalent)
void TrackPainter::paintScaleOverlay(QPainter &p)
{
    if (!m_model || !m_model->expanded())
        return;

    static const int kScaleMinH = 60;
    static const int kScaleStep = 30;
    static const int kFontPx = 8;
    static const int kMarg = 2;

    p.save();
    QFont sf = p.font();
    sf.setPixelSize(kFontPx);
    p.setFont(sf);
    const QColor scaleText = themeColor(Utils::Theme::Timeline_TextColor);
    const QColor scaleDiv  = themeColor(Utils::Theme::Timeline_DividerColor);

    const int rowCount = m_model->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const int rowH = m_model->rowHeight(row);
        const int rowY = m_model->rowOffset(row);
        if (rowH < kScaleMinH)
            continue;
        const qint64 minVal = m_model->rowMinValue(row);
        const qint64 maxVal = m_model->rowMaxValue(row);
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

        p.setPen(scaleText);
        p.drawText(QPointF(kMarg, topLabelBottom), prettyPrintScale(maxVal));

        const int numLines = int(valDiff / stepVal);
        for (int i = 0; i < numLines; ++i) {
            if (rowY + qRound(double(rowH) - double(i + 1) * stepH) <= topLabelBottom)
                break;
            const int lineY = rowY + rowH - qRound(double(i) * stepH);
            p.setPen(scaleDiv);
            p.drawLine(0, lineY, width() - 1, lineY);
            p.setPen(scaleText);
            p.drawText(QPointF(kMarg, lineY - kMarg),
                       prettyPrintScale(minVal + qint64(i) * stepVal));
        }
    }
    p.restore();
}

void TrackPainter::invalidateCache()
{
    m_cache = QPixmap();
    m_pendingShiftPx = 0;
}

void TrackPainter::rebuildCache()
{
    m_cache = QPixmap();
    m_pendingShiftPx = 0;
    if (width() <= 0 || height() <= 0)
        return;

    const qreal dpr = devicePixelRatioF();
    QPixmap pix(qRound(width() * dpr), qRound(height() * dpr));
    pix.setDevicePixelRatio(dpr);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    renderContent(p, m_rangeStart, m_rangeEnd);

    m_cache = pix;
}

void TrackPainter::paintEvent(QPaintEvent *)
{
    if (m_cache.isNull())
        rebuildCache();

    QPainter p(this);

    if (m_cache.isNull()) {
        // Degenerate state: no widget size yet — draw backgrounds manually.
        const QColor bg1 = themeColor(Utils::Theme::Timeline_BackgroundColor1);
        const QColor bg2 = themeColor(Utils::Theme::Timeline_BackgroundColor2);
        if (!m_model || m_model->hidden()) {
            p.fillRect(rect(), bg1);
        } else {
            for (int row = 0; row < m_model->rowCount(); ++row) {
                const int rowY = m_model->rowOffset(row);
                const int rowH = m_model->rowHeight(row);
                p.fillRect(0, rowY, width(), rowH,
                           ((row % 2 == 0) == m_startOdd) ? bg1 : bg2);
            }
        }
        return;
    }

    if (m_pendingShiftPx != 0) {
        // Apply the accumulated pan: shift the cache and render the newly exposed strip.
        const qreal dpr = devicePixelRatioF();
        QPixmap newCache(qRound(width() * dpr), qRound(height() * dpr));
        newCache.setDevicePixelRatio(dpr);
        newCache.fill(Qt::transparent);

        QPainter cp(&newCache);
        cp.drawPixmap(QPointF(m_pendingShiftPx, 0.0), m_cache);

        // Exposed strip in pixel space, and the corresponding time range.
        int stripLeft, stripRight;
        if (m_pendingShiftPx > 0) {
            stripLeft = 0;
            stripRight = qMin(m_pendingShiftPx, width());
        } else {
            stripLeft = qMax(0, width() + m_pendingShiftPx);
            stripRight = width();
        }

        if (stripRight > stripLeft) {
            const double wf = double(width());
            const qint64 iterStart = pixelToTime(double(stripLeft), wf,
                                                  m_rangeStart, m_rangeEnd);
            const qint64 iterEnd = pixelToTime(double(stripRight), wf,
                                                m_rangeStart, m_rangeEnd);
            cp.setClipRect(stripLeft, 0, stripRight - stripLeft, height());
            renderContent(cp, iterStart, iterEnd);
            cp.setClipping(false);
        }

        m_cache = newCache;
        m_pendingShiftPx = 0;
    }

    p.drawPixmap(QPointF(0, 0), m_cache);

    paintScaleOverlay(p);

    // Overlay selection/hover borders live (not cached).
    if (m_model && !m_model->hidden() && m_rangeStart < m_rangeEnd
            && (m_selectedItem >= 0 || m_hoveredItem >= 0)) {
        const QColor selectionColor = m_selectionLocked ? QColor(96, 0, 255) : Qt::blue;
        const QColor hoverColor = Qt::white;

        const auto paintOverlay = [&](int idx) {
            if (idx < 0)
                return;
            const int row = m_model->row(idx);
            const int rowH = m_model->rowHeight(row);
            const int rowY = m_model->rowOffset(row);
            const double relH = m_model->relativeHeight(idx);
            const double itemH = rowH * relH;
            const double itemY = rowY + rowH - itemH;
            const qint64 start = m_model->startTime(idx);
            const qint64 end = m_model->endTime(idx);
            double x1 = timeToPixel(start, m_rangeStart, m_rangeEnd, double(width()));
            double x2 = timeToPixel(end, m_rangeStart, m_rangeEnd, double(width()));
            if (x2 - x1 < 1.0)
                x2 = x1 + 1.0;
            x1 = qMax(x1, 0.0);
            x2 = qMin(x2, double(width()));
            const QRectF itemRect(x1, itemY, x2 - x1, itemH);
            if (idx == m_selectedItem) {
                p.setPen(QPen(selectionColor, 4));
                p.drawRect(itemRect.adjusted(2.0, 2.0, -2.0, -2.0));
            } else {
                p.setPen(QPen(hoverColor, 1));
                p.drawRect(itemRect.adjusted(0.5, 0.5, -0.5, -0.5));
            }
        };

        paintOverlay(m_hoveredItem);
        paintOverlay(m_selectedItem);
    }
}

void TrackPainter::resizeEvent(QResizeEvent *)
{
    invalidateCache();
}

int TrackPainter::indexAt(const QPoint &pos) const
{
    if (!m_model || m_model->isEmpty())
        return -1;
    const qint64 rangeDuration = m_rangeEnd - m_rangeStart;
    if (rangeDuration <= 0 || width() == 0)
        return -1;

    const qint64 t = pixelToTime(pos.x(), double(width()), m_rangeStart, m_rangeEnd);

    const int candidate = m_model->bestIndex(t);
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
    const int count = m_model->count();

    // Test whether item i is drawn under the cursor and, if so, whether it is
    // narrower than the current best (innermost nested item wins).
    const auto test = [&](int i) {
        const int row = m_model->row(i);
        const int rowH = m_model->rowHeight(row);
        const int rowY = m_model->rowOffset(row);

        const double relH = m_model->relativeHeight(i);
        const double itemH = rowH * relH;
        const double itemY = rowY + rowH - itemH;

        if (pos.y() < itemY || pos.y() >= itemY + itemH)
            return;

        const qint64 start = m_model->startTime(i);
        const qint64 end = m_model->endTime(i);

        double x1 = timeToPixel(start, m_rangeStart, m_rangeEnd, double(width()));
        double x2 = timeToPixel(end, m_rangeStart, m_rangeEnd, double(width()));
        if (x2 - x1 < 1.0)
            x2 = x1 + 1.0;

        if (pos.x() < x1 || pos.x() >= x2)
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
        if (timeToPixel(m_model->startTime(i), m_rangeStart, m_rangeEnd, double(width())) > pos.x())
            break;
        test(i);
    }

    // Backward: a short item that doesn't reach the cursor may still have a
    // long parent that does. Only stop once neither the item nor its parent can
    // cover the cursor time.
    for (int i = candidate - 1; i >= 0; --i) {
        const qint64 end = m_model->endTime(i);
        if (end < t) {
            const int parent = m_model->parentIndex(i);
            const qint64 parentEnd = parent == -1 ? end : m_model->endTime(parent);
            if (parentEnd < t)
                break;
        }
        test(i);
    }

    return best;
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

    const int idx = indexAt(event->pos());
    if (idx != m_hoveredItem) {
        m_hoveredItem = idx;
        update();
        emit itemHovered(idx);
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
        if (!m_panning)
            emit itemClicked(indexAt(event->pos()));
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
        event->ignore();
    }
}

void TrackPainter::leaveEvent(QEvent *)
{
    if (m_hoveredItem != -1) {
        m_hoveredItem = -1;
        update();
        emit itemHovered(-1);
    }
}

} // namespace Timeline
