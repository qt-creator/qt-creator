// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "resizehandle.h"

#include <QDebug>
#include <QStyle>
#include <QStyleOption>
#include <QMouseEvent>
#include <QRubberBand>
#include <QPointer>

namespace ADS {

/**
 * Private data class of CResizeHandle class (pimpl)
 */
class ResizeHandlePrivate
{
public:
    ResizeHandle *q = nullptr;
    Qt::Edge m_handlePosition = Qt::LeftEdge;
    QWidget *m_target = nullptr;
    int m_mouseOffset = 0;
    bool m_pressed = false;
    int m_minSize = 0;
    int m_maxSize = 1;
    QPointer<QRubberBand> m_rubberBand;
    bool m_opaqueResize = false;
    int m_handleWidth = 4;

    /**
     * Private data constructor
     */
    ResizeHandlePrivate(ResizeHandle *parent);

    /**
     * Pick position component from pos depending on orientation
     */
    int pick(const QPoint &pos) const
    {
        return q->orientation() == Qt::Horizontal ? pos.x() : pos.y();
    }

    /**
     * Returns true, if orientation is horizontal
     */
    bool isHorizontal() const { return q->orientation() == Qt::Horizontal; }

    /**
     * Set rubberband position
     */
    void setRubberBand(int pos);

    /**
     * Calculates the resize position and geometry
     */
    void doResizing(QMouseEvent *event, bool forceResize = false);
};
// class ResizeHandlePrivate

ResizeHandlePrivate::ResizeHandlePrivate(ResizeHandle *parent)
    : q(parent)
{}

void ResizeHandlePrivate::setRubberBand(int pos)
{
    if (!m_rubberBand)
        m_rubberBand = new QRubberBand(QRubberBand::Line, m_target->parentWidget());

    auto geometry = q->geometry();
    auto topLeft = m_target->mapTo(m_target->parentWidget(), geometry.topLeft());
    switch (m_handlePosition) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        topLeft.rx() += pos;
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        topLeft.ry() += pos;
        break;
    }

    geometry.moveTopLeft(topLeft);
    m_rubberBand->setGeometry(geometry);
    m_rubberBand->show();
}

void ResizeHandlePrivate::doResizing(QMouseEvent *event, bool forceResize)
{
    int pos = pick(event->pos()) - m_mouseOffset;
    auto oldGeometry = m_target->geometry();
    auto newGeometry = oldGeometry;
    switch (m_handlePosition) {
    case Qt::LeftEdge: {
        newGeometry.adjust(pos, 0, 0, 0);
        int size = qBound(m_minSize, newGeometry.width(), m_maxSize);
        pos += (newGeometry.width() - size);
        newGeometry.setWidth(size);
        newGeometry.moveTopRight(oldGeometry.topRight());
    } break;

    case Qt::RightEdge: {
        newGeometry.adjust(0, 0, pos, 0);
        int size = qBound(m_minSize, newGeometry.width(), m_maxSize);
        pos -= (newGeometry.width() - size);
        newGeometry.setWidth(size);
    } break;

    case Qt::TopEdge: {
        newGeometry.adjust(0, pos, 0, 0);
        int size = qBound(m_minSize, newGeometry.height(), m_maxSize);
        pos += (newGeometry.height() - size);
        newGeometry.setHeight(size);
        newGeometry.moveBottomLeft(oldGeometry.bottomLeft());
    } break;

    case Qt::BottomEdge: {
        newGeometry.adjust(0, 0, 0, pos);
        int size = qBound(m_minSize, newGeometry.height(), m_maxSize);
        pos -= (newGeometry.height() - size);
        newGeometry.setHeight(size);
    } break;
    }

    if (q->opaqueResize() || forceResize) {
        m_target->setGeometry(newGeometry);
    } else {
        setRubberBand(pos);
    }
}

ResizeHandle::ResizeHandle(Qt::Edge handlePosition, QWidget *parent)
    : Super(parent)
    , d(new ResizeHandlePrivate(this))
{
    d->m_target = parent;
    setMinResizeSize(48);
    setHandlePosition(handlePosition);
}

ResizeHandle::~ResizeHandle()
{
    delete d;
}

void ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    d->doResizing(event);
}

void ResizeHandle::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->m_mouseOffset = d->pick(event->pos());
        d->m_pressed = true;
        update();
    }
}

void ResizeHandle::mouseReleaseEvent(QMouseEvent *event)
{
    if (!opaqueResize() && event->button() == Qt::LeftButton) {
        if (d->m_rubberBand)
            d->m_rubberBand->deleteLater();

        d->doResizing(event, true);
    }

    if (event->button() == Qt::LeftButton) {
        d->m_pressed = false;
        update();
    }
}

void ResizeHandle::setHandlePosition(Qt::Edge handlePosition)
{
    d->m_handlePosition = handlePosition;
    switch (d->m_handlePosition) {
    case Qt::LeftEdge: // fall through
    case Qt::RightEdge:
        setCursor(Qt::SizeHorCursor);
        break;

    case Qt::TopEdge: // fall through
    case Qt::BottomEdge:
        setCursor(Qt::SizeVerCursor);
        break;
    }

    setMaxResizeSize(d->isHorizontal() ? parentWidget()->height() : parentWidget()->width());
    if (!d->isHorizontal()) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
}

Qt::Edge ResizeHandle::handlePostion() const
{
    return d->m_handlePosition;
}

Qt::Orientation ResizeHandle::orientation() const
{
    switch (d->m_handlePosition) {
    case Qt::LeftEdge: // fall through
    case Qt::RightEdge:
        return Qt::Horizontal;

    case Qt::TopEdge: // fall through
    case Qt::BottomEdge:
        return Qt::Vertical;
    }

    return Qt::Horizontal;
}

QSize ResizeHandle::sizeHint() const
{
    QSize result;
    switch (d->m_handlePosition) {
    case Qt::LeftEdge: // fall through
    case Qt::RightEdge:
        result = QSize(d->m_handleWidth, d->m_target->height());
        break;

    case Qt::TopEdge: // fall through
    case Qt::BottomEdge:
        result = QSize(d->m_target->width(), d->m_handleWidth);
        break;
    }

    return result;
}

bool ResizeHandle::isResizing() const
{
    return d->m_pressed;
}

void ResizeHandle::setMinResizeSize(int minSize)
{
    d->m_minSize = minSize;
}

void ResizeHandle::setMaxResizeSize(int maxSize)
{
    d->m_maxSize = maxSize;
}

void ResizeHandle::setOpaqueResize(bool opaque)
{
    if (d->m_opaqueResize == opaque)
        return;

    d->m_opaqueResize = opaque;
    emit opaqueResizeChanged();
}

bool ResizeHandle::opaqueResize() const
{
    return d->m_opaqueResize;
}

} // namespace ADS
