/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "formeditorgraphicsview.h"

#include <QWheelEvent>
#include <QDebug>
#include <QScrollBar>

namespace QmlDesigner {

FormEditorGraphicsView::FormEditorGraphicsView(QWidget *parent) :
    QGraphicsView(parent),
    m_isPanning(false)
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
    if (event->buttons().testFlag(Qt::MiddleButton)) {
        m_isPanning = true;
        m_panStartX = event->x();
        m_panStartY = event->y();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void FormEditorGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->x() - m_panStartX));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (event->y() - m_panStartY));
        m_panStartX = event->x();
        m_panStartY = event->y();
        event->accept();
    }else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void FormEditorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning) {

        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }else {
        QGraphicsView::mouseReleaseEvent(event);
    }
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
