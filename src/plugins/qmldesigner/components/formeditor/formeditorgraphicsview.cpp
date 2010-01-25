/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "formeditorgraphicsview.h"

#include <QWheelEvent>
#include <QApplication>
#include <QtDebug>

namespace QmlDesigner {

FormEditorGraphicsView::FormEditorGraphicsView(QWidget *parent) :
    QGraphicsView(parent)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
//    setCacheMode(QGraphicsView::CacheNone);
     setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
//    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    setRenderHint(QPainter::Antialiasing, false);

    setFrameShape(QFrame::NoFrame);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);

    const int checkerbordSize= 20;
    QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
    tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);

    viewport()->setMouseTracking(true);
}

void FormEditorGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        event->ignore();
    } else {
        QGraphicsView::wheelEvent(event);
    }

}

void FormEditorGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos())) {
        QGraphicsView::mouseMoveEvent(event);
    } else {
        QPoint position = event->pos();
        QPoint topLeft = rect().topLeft();
        QPoint bottomRight = rect().bottomRight();
        position.rx() = qMax(topLeft.x(), qMin(position.x(), bottomRight.x()));
        position.ry() = qMax(topLeft.y(), qMin(position.y(), bottomRight.y()));
        QMouseEvent *mouseEvent = QMouseEvent::createExtendedMouseEvent(event->type(), position, mapToGlobal(position), event->button(), event->buttons(), event->modifiers());

        QGraphicsView::mouseMoveEvent(mouseEvent);
        delete mouseEvent;
    }
}


void FormEditorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos())) {
        QGraphicsView::mouseReleaseEvent(event);
    } else {
        QPoint position = event->pos();
        QPoint topLeft = rect().topLeft();
        QPoint bottomRight = rect().bottomRight();
        position.rx() = qMax(topLeft.x(), qMin(position.x(), bottomRight.x()));
        position.ry() = qMax(topLeft.y(), qMin(position.y(), bottomRight.y()));
        QMouseEvent *mouseEvent = QMouseEvent::createExtendedMouseEvent(event->type(), position, mapToGlobal(position), event->button(), event->buttons(), event->modifiers());

        QGraphicsView::mouseReleaseEvent(mouseEvent);
        delete mouseEvent;
    }
}

void FormEditorGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setBrushOrigin(0, 0);
    painter->fillRect(rect.intersected(sceneRect()), backgroundBrush());
    // paint rect around editable area
    painter->setPen(Qt::darkGray);
    QRectF frameRect = sceneRect().adjusted(-1, -1, 0, 0);
    painter->drawRect(frameRect);
    painter->restore();
}

} // namespace QmlDesigner
