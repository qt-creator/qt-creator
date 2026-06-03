// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectionrangeoverlay.h"

#include "timelinecoordinates.h"
#include "timelinezoomcontrol.h"

#include <utils/theme/theme.h>

#include <QMouseEvent>
#include <QPainter>

namespace Timeline {

static QColor themeColor(Utils::Theme::Color role)
{
    if (Utils::creatorTheme())
        return Utils::creatorTheme()->color(role);
    return QColor();
}

SelectionRangeOverlay::SelectionRangeOverlay(TimelineZoomControl *zoom, QWidget *parent)
    : QWidget(parent)
    , m_zoom(zoom)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    connect(zoom, &TimelineZoomControl::rangeChanged, this, &SelectionRangeOverlay::setRange);
    setRange(zoom->rangeStart(), zoom->rangeEnd());
}

void SelectionRangeOverlay::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    setAttribute(Qt::WA_TransparentForMouseEvents, !active);
    if (!active)
        m_dragMode = DragMode::None;
    update();
}

void SelectionRangeOverlay::setRange(qint64 rangeStart, qint64 rangeEnd)
{
    m_rangeStart = rangeStart;
    m_rangeEnd = rangeEnd;
    if (m_state == State::Finished)
        updateFromZoomer();
    else
        update();
}

void SelectionRangeOverlay::reset()
{
    m_rangeRight = m_rangeLeft + 1.0;
    m_state = State::Inactive;
    m_creationRef = 0;
    m_dragMode = DragMode::None;
    update();
}

void SelectionRangeOverlay::paintEvent(QPaintEvent *)
{
    if (!m_active || m_state == State::Inactive)
        return;

    const QColor rangeColor = themeColor(Utils::Theme::Timeline_RangeColor);
    const QColor handleColor = themeColor(Utils::Theme::Timeline_HandleColor);
    const int h = height();

    QPainter p(this);

    if (m_rangeRight - m_rangeLeft > 1.0) {
        QColor bandColor = rangeColor;
        bandColor.setAlphaF(qBound(0.3, double(bandColor.alphaF()), 0.7));
        p.fillRect(QRectF(m_rangeLeft, 0, m_rangeRight - m_rangeLeft, h), bandColor);
    } else {
        p.setPen(QPen(rangeColor, 1));
        p.drawLine(qRound(m_rangeLeft), 0, qRound(m_rangeLeft), h - 1);
    }

    if (m_state == State::Finished) {
        const double handleW = 7.0;
        p.fillRect(QRectF(m_rangeLeft - handleW, 0, handleW, h), handleColor);
        p.fillRect(QRectF(m_rangeRight, 0, handleW, h), handleColor);
    }
}

void SelectionRangeOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    const double x = event->position().x();

    if (m_state == State::Finished) {
        const double handleW = 7.0;
        if (x >= m_rangeLeft - handleW && x < m_rangeLeft) {
            m_dragMode = DragMode::Left;
        } else if (x >= m_rangeRight && x < m_rangeRight + handleW) {
            m_dragMode = DragMode::Right;
        } else if (x >= m_rangeLeft && x < m_rangeRight) {
            m_dragMode = DragMode::Move;
        } else {
            m_state = State::Inactive;
            m_dragMode = DragMode::None;
            update();
            emit passthruClick(event->pos());
            event->accept();
            return;
        }
        if (m_dragMode != DragMode::None) {
            m_dragStartX = x;
            m_dragStartLeft = m_rangeLeft;
            m_dragStartRight = m_rangeRight;
            event->accept();
            return;
        }
    }

    if (m_state == State::Inactive) {
        m_state = State::FirstLimit;
        setPos(x);
        m_state = State::SecondLimit;
        updateZoomer();
        update();
    }
    event->accept();
}

void SelectionRangeOverlay::mouseMoveEvent(QMouseEvent *event)
{
    const double x = event->position().x();

    if (m_state == State::SecondLimit) {
        setPos(x);
        updateZoomer();
        update();
    } else if (m_state == State::Finished && m_dragMode != DragMode::None) {
        const double delta = x - m_dragStartX;
        const double w = double(width());

        if (m_dragMode == DragMode::Left) {
            m_rangeLeft = qBound(0.0, m_dragStartLeft + delta, m_rangeRight - 1.0);
        } else if (m_dragMode == DragMode::Right) {
            m_rangeRight = qBound(m_rangeLeft + 1.0, m_dragStartRight + delta, w);
        } else {
            const double rangeW = m_dragStartRight - m_dragStartLeft;
            m_rangeLeft = qBound(0.0, m_dragStartLeft + delta, w - rangeW);
            m_rangeRight = m_rangeLeft + rangeW;
        }
        updateZoomer();
        update();
    } else if (m_state == State::Finished) {
        const double handleW = 7.0;
        if (x >= m_rangeLeft - handleW && x < m_rangeLeft)
            setCursor(Qt::SizeHorCursor);
        else if (x >= m_rangeRight && x < m_rangeRight + handleW)
            setCursor(Qt::SizeHorCursor);
        else if (x >= m_rangeLeft && x < m_rangeRight)
            setCursor(Qt::SizeAllCursor);
        else
            unsetCursor();
    }
    event->accept();
}

void SelectionRangeOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    if (m_state == State::SecondLimit) {
        m_state = State::Finished;
        updateZoomer();
    } else if (m_state == State::Finished) {
        m_dragMode = DragMode::None;
    }
    event->accept();
}

void SelectionRangeOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    if (m_state == State::Finished) {
        const qint64 diff = m_zoom->minimumRangeLength() - m_zoom->selectionDuration();
        if (diff > 0) {
            m_zoom->setRange(m_zoom->selectionStart() - diff / 2,
                             m_zoom->selectionEnd() + diff / 2);
        } else {
            m_zoom->setRange(m_zoom->selectionStart(), m_zoom->selectionEnd());
        }
        emit rangeDoubleClicked();
    }
    event->accept();
}

void SelectionRangeOverlay::leaveEvent(QEvent *)
{
    unsetCursor();
}

void SelectionRangeOverlay::resizeEvent(QResizeEvent *)
{
    if (m_state == State::Finished)
        updateFromZoomer();
}

void SelectionRangeOverlay::setPos(double x)
{
    x = clamp(x);
    switch (m_state) {
    case State::FirstLimit:
        m_creationRef = x;
        m_rangeLeft = x;
        m_rangeRight = x + 1.0;
        break;
    case State::SecondLimit:
        if (x > m_creationRef) {
            m_rangeLeft = m_creationRef;
            m_rangeRight = x;
        } else if (x < m_creationRef) {
            m_rangeLeft = x;
            m_rangeRight = m_creationRef;
        }
        break;
    default:
        break;
    }
}

void SelectionRangeOverlay::updateZoomer()
{
    const double w = double(width());
    m_zoom->setSelection(
        pixelToTime(m_rangeLeft, w, m_rangeStart, m_rangeEnd),
        pixelToTime(m_rangeRight, w, m_rangeStart, m_rangeEnd)
    );
}

void SelectionRangeOverlay::updateFromZoomer()
{
    const double w = double(width());
    m_rangeLeft = timeToPixel(m_zoom->selectionStart(), m_rangeStart, m_rangeEnd, w);
    m_rangeRight = timeToPixel(m_zoom->selectionEnd(), m_rangeStart, m_rangeEnd, w);
    update();
}

double SelectionRangeOverlay::clamp(double x) const
{
    return qBound(0.0, x, double(width()));
}

} // namespace Timeline
