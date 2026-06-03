// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timeruler.h"

#include "timelinecoordinates.h"
#include "timelineformattime.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

namespace Timeline {

static constexpr int kInitialBlockLength = 120; // target pixels per major block
static constexpr int kTextMargin = 5;
static constexpr int kFontPixelSize = 8;

TimeRuler::TimeRuler(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TimeRuler::setRange(qint64 rangeStart, qint64 rangeEnd)
{
    if (m_rangeStart == rangeStart && m_rangeEnd == rangeEnd)
        return;
    m_rangeStart = rangeStart;
    m_rangeEnd = rangeEnd;
    update();
}

QSize TimeRuler::sizeHint() const
{
    return QSize(200, Utils::StyleHelper::navigationWidgetHeight());
}

void TimeRuler::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    const QColor bgColor = Utils::creatorTheme()
                               ? Utils::creatorTheme()->color(Utils::Theme::PanelStatusBarBackgroundColor)
                               : palette().window().color();
    p.fillRect(rect(), bgColor);

    const qint64 rangeDuration = m_rangeEnd - m_rangeStart;
    if (rangeDuration <= 0 || width() == 0)
        return;

    const double spacing = double(width()) / double(rangeDuration);

    const qint64 timePerBlock = rulerBlockDuration(rangeDuration, double(width()),
                                                   kInitialBlockLength);

    // Align the first block to a timePerBlock boundary at or before rangeStart.
    const qint64 alignedStart = m_rangeStart - (m_rangeStart % timePerBlock);

    const double pixelsPerBlock = double(timePerBlock) * spacing;
    const double pixelsPerSection = pixelsPerBlock / 5.0;

    const int labelsHeight = height();
    const int ticksTop = labelsHeight / 2; // minor ticks occupy bottom half

    const QColor dividerColor = Utils::creatorTheme()
                                    ? Utils::creatorTheme()->color(Utils::Theme::Timeline_DividerColor)
                                    : QColor(Qt::gray);
    const QColor textColor = Utils::creatorTheme()
                                 ? Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight)
                                 : palette().text().color();

    QFont font = p.font();
    font.setPixelSize(kFontPixelSize);
    p.setFont(font);
    p.setPen(dividerColor);

    for (qint64 t = alignedStart; ; t += timePerBlock) {
        const double x = timeToPixel(t, m_rangeStart, m_rangeEnd, double(width()));
        if (x > double(width()))
            break;

        // Label for time t, left-aligned in [x, x+pixelsPerBlock].
        if (x + pixelsPerBlock > 0.0 && x < double(width())) {
            const QString label = formatTime(t, rangeDuration);
            const QRectF labelRect(x + kTextMargin, 0,
                                   pixelsPerBlock - kTextMargin, double(labelsHeight));
            p.setPen(textColor);
            p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);
            p.setPen(dividerColor);
        }

        // 4 minor ticks at 1/5..4/5 of the block.
        for (int s = 1; s <= 4; ++s) {
            const double sx = x + s * pixelsPerSection;
            if (sx >= 0.0 && sx <= double(width())) {
                const int ix = qRound(sx);
                p.drawLine(ix, ticksTop, ix, height() - 1);
            }
        }

        // Major tick at the right edge of this block (= left edge of next block).
        const double tickX = x + pixelsPerBlock;
        if (tickX >= 0.0 && tickX <= double(width())) {
            const int ix = qRound(tickX);
            p.drawLine(ix, 0, ix, height() - 1);
        }
    }

    // Draw marker diamonds and delete buttons
    if (!m_markers.isEmpty()) {
        const QColor handleColor = Utils::creatorTheme()
                                       ? Utils::creatorTheme()->color(Utils::Theme::Timeline_HandleColor)
                                       : QColor(Qt::darkCyan);
        const QColor panelBgColor = Utils::creatorTheme()
                                        ? Utils::creatorTheme()->color(Utils::Theme::Timeline_PanelBackgroundColor)
                                        : palette().window().color();
        const QColor textColor2 = Utils::creatorTheme()
                                      ? Utils::creatorTheme()->color(Utils::Theme::Timeline_TextColor)
                                      : palette().text().color();
        // Half-width matches QML: S * sqrt(2) / 2, where S = height()
        const double hw = double(height()) * 0.707;
        const double dh = double(height()) * 0.707;
        const double btnSize = qMax(4.0, double(height()) / 4.0);
        for (qint64 ts : m_markers) {
            const double mx = markerPixel(ts);
            if (mx < -hw || mx > width() + hw)
                continue;
            // Diamond triangle
            p.setPen(Qt::NoPen);
            p.setBrush(handleColor);
            const QPointF pts[3] = {
                QPointF(mx - hw, 0),
                QPointF(mx + hw, 0),
                QPointF(mx,      dh)
            };
            p.drawConvexPolygon(pts, 3);
            // Delete button: small square with horizontal line, at the apex
            const QRectF btnRect(mx - btnSize / 2.0, dh, btnSize, btnSize);
            p.fillRect(btnRect, panelBgColor);
            const double lineW = btnSize - 2.0;
            const QRectF lineRect(mx - lineW / 2.0,
                                  dh + (btnSize - 1.0) / 2.0,
                                  lineW, 1.0);
            p.fillRect(lineRect, textColor2);
        }
    }
}

void TimeRuler::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    const double dh = double(height()) * 0.707;
    const double btnSize = qMax(4.0, double(height()) / 4.0);
    const double py = event->position().y();

    // Check delete button first (apex of triangle)
    for (int i = 0; i < m_markers.size(); ++i) {
        const double mx = markerPixel(m_markers[i]);
        const QRectF btnRect(mx - btnSize / 2.0, dh, btnSize, btnSize);
        if (btnRect.contains(event->position())) {
            m_markers.removeAt(i);
            emit markersChanged(m_markers);
            update();
            event->accept();
            return;
        }
    }

    // Check diamond body (drag start) — only when click is above the apex
    Q_UNUSED(py)
    const int idx = markerAt(event->pos().x());
    if (idx >= 0) {
        m_dragIndex = idx;
        m_dragStartX = event->pos().x();
        m_dragStartTime = m_markers[idx];
        m_dragged = false;
    } else {
        // Add a new marker at the clicked time
        if (m_rangeEnd > m_rangeStart) {
            const qint64 ts = pixelToTime(event->pos().x(), double(width()),
                                          m_rangeStart, m_rangeEnd);
            m_markers.append(ts);
            emit markersChanged(m_markers);
            update();
        }
    }
    event->accept();
}

void TimeRuler::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragIndex < 0 || m_dragIndex >= m_markers.size()) {
        event->ignore();
        return;
    }
    if (!m_dragged && qAbs(event->pos().x() - m_dragStartX) < 3) {
        event->accept();
        return;
    }
    m_dragged = true;
    m_markers[m_dragIndex] = pixelToTime(event->pos().x(), double(width()),
                                          m_rangeStart, m_rangeEnd);
    emit markersChanged(m_markers);
    update();
    event->accept();
}

void TimeRuler::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragIndex = -1;
        m_dragged = false;
    }
    event->accept();
}

int TimeRuler::markerAt(int x) const
{
    const double hw = double(height()) * 0.707;
    for (int i = 0; i < m_markers.size(); ++i) {
        const double mx = markerPixel(m_markers[i]);
        if (qAbs(double(x) - mx) <= hw)
            return i;
    }
    return -1;
}

double TimeRuler::markerPixel(qint64 timestamp) const
{
    if (m_rangeEnd <= m_rangeStart)
        return 0.0;
    return timeToPixel(timestamp, m_rangeStart, m_rangeEnd, double(width()));
}

} // namespace Timeline
