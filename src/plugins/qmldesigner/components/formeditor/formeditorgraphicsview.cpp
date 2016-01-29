/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "formeditorgraphicsview.h"

#include <QWheelEvent>
#include <QDebug>
#include <QScrollBar>

namespace QmlDesigner {

FormEditorGraphicsView::FormEditorGraphicsView(QWidget *parent) :
    QGraphicsView(parent)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setAlignment(Qt::AlignCenter);
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
//    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    setRenderHint(QPainter::Antialiasing, false);

    setFrameShape(QFrame::NoFrame);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);

    activateCheckboardBackground();

    viewport()->setMouseTracking(true);
}

void FormEditorGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier))
        event->ignore();
    else
        QGraphicsView::wheelEvent(event);
}

void FormEditorGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::MiddleButton) && m_isPanning == Panning::NotStarted)
        startPanning(event);
    else
        QGraphicsView::mousePressEvent(event);
}

void FormEditorGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning != Panning::NotStarted) {
        if (!m_panningStartPosition.isNull()) {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                (event->x() - m_panningStartPosition.x()));
            verticalScrollBar()->setValue(verticalScrollBar()->value() -
                (event->y() - m_panningStartPosition.y()));
        }
        m_panningStartPosition = event->pos();
        event->accept();
    }else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void FormEditorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning == Panning::MouseWheelStarted)
        stopPanning(event);
    else
        QGraphicsView::mouseReleaseEvent(event);
}

void FormEditorGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && m_isPanning == Panning::NotStarted)
        startPanning(event);
    else
        QGraphicsView::keyPressEvent(event);
}

void FormEditorGraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat())
        stopPanning(event);
    else
        QGraphicsView::keyReleaseEvent(event);
}

void FormEditorGraphicsView::startPanning(QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
        m_isPanning = Panning::SpaceKeyStarted;
    else
        m_isPanning = Panning::MouseWheelStarted;
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void FormEditorGraphicsView::stopPanning(QEvent *event)
{
    m_isPanning = Panning::NotStarted;
    m_panningStartPosition = QPoint();
    setCursor(Qt::ArrowCursor);
    event->accept();
}

void FormEditorGraphicsView::setRootItemRect(const QRectF &rect)
{
    m_rootItemRect = rect;
    viewport()->update();
}

QRectF FormEditorGraphicsView::rootItemRect() const
{
    return m_rootItemRect;
}

void FormEditorGraphicsView::activateCheckboardBackground()
{
    const int checkerbordSize= 20;
    QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
    tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
}

void FormEditorGraphicsView::activateColoredBackground(const QColor &color)
{
    setBackgroundBrush(color);
}

void FormEditorGraphicsView::drawBackground(QPainter *painter, const QRectF &rectangle)
{
    painter->save();
    painter->setBrushOrigin(0, 0);

    painter->fillRect(rectangle.intersected(rootItemRect()), backgroundBrush());
    // paint rect around editable area
    painter->setPen(Qt::black);
    painter->drawRect( rootItemRect());
    painter->restore();
}

} // namespace QmlDesigner
