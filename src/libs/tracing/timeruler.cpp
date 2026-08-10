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

    const double rulerWidth = width() - contentsMargins().right();
    const int ticksTop = height() * 0.75;
    const QColor dividerColor = Utils::creatorTheme()
                                    ? Utils::creatorTheme()->color(Utils::Theme::Timeline_DividerColor)
                                    : QColor(Qt::gray);
    const QColor textColor = Utils::creatorTheme()
                                 ? Utils::creatorTheme()->color(Utils::Theme::PanelTextColorLight)
                                 : palette().text().color();
    const QFont labelFont = Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementCaption);

    p.setFont(labelFont);

    forEachRulerTick(
        m_rangeStart, m_rangeEnd, rulerWidth,
        // Label for time t, left-aligned in [x, x+pixelsPerBlock].
        [&](qint64 t, double x, double pixelsPerBlock) {
            if (x + pixelsPerBlock > 0.0 && x < rulerWidth) {
                const QString label = formatTime(t, rangeDuration);
                const int kTextMargin = Utils::StyleHelper::SpacingTokens::PaddingHS;
                const QRectF labelRect(x + kTextMargin, 0, pixelsPerBlock - kTextMargin, ticksTop);
                p.setPen(textColor);
                p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);
            }
        },
        [&](double x, bool isMajor) {
            const int ix = qRound(x);
            const int y = isMajor ? 0 : ticksTop;
            p.fillRect(QRect(ix, y, 1, height() - y), dividerColor);
        });

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
        for (qint64 ts : std::as_const(m_markers)) {
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
            const double rulerWidth = width() - contentsMargins().right();
            const qint64 ts = pixelToTime(event->pos().x(), rulerWidth,
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
    const double rulerWidth = width() - contentsMargins().right();
    m_markers[m_dragIndex] = pixelToTime(event->pos().x(), rulerWidth,
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
    const double rulerWidth = width() - contentsMargins().right();
    return timeToPixel(timestamp, m_rangeStart, m_rangeEnd, rulerWidth);
}

} // namespace Timeline
