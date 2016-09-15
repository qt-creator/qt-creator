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

#include "navigatorgraphicsview.h"
#include "graphicsscene.h"
#include "graphicsview.h"

#include <QWheelEvent>

using namespace ScxmlEditor::Common;

NavigatorGraphicsView::NavigatorGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setInteractive(false);
    setViewportUpdateMode(FullViewportUpdate);
}

void NavigatorGraphicsView::setGraphicsScene(ScxmlEditor::PluginInterface::GraphicsScene *s)
{
    if (scene())
        scene()->disconnect(this);

    setScene(s);
    if (s)
        connect(s, &PluginInterface::GraphicsScene::sceneRectChanged, this, &NavigatorGraphicsView::updateView);
}

void NavigatorGraphicsView::setMainViewPolygon(const QPolygonF &pol)
{
    m_mainViewPolygon = mapFromScene(pol);
    updateView();
}

void NavigatorGraphicsView::updateView()
{
    fitInView(sceneRect()); //, Qt::KeepAspectRatio);
    update();
}

void NavigatorGraphicsView::paintEvent(QPaintEvent *e)
{
    QGraphicsView::paintEvent(e);

    QPainter p(viewport());
    p.save();
    p.setBrush(Qt::NoBrush);
    p.setPen(Qt::red);
    p.drawPolygon(m_mainViewPolygon);
    p.restore();
}

void NavigatorGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (Qt::ControlModifier & event->modifiers()) {
        if (event->delta() > 0)
            emit zoomIn();
        else
            emit zoomOut();

        emit moveMainViewTo(mapToScene(event->pos()));
    } else
        QGraphicsView::wheelEvent(event);
}

void NavigatorGraphicsView::mousePressEvent(QMouseEvent *event)
{
    m_mouseDown = true;
    emit moveMainViewTo(mapToScene(event->pos()));
    QGraphicsView::mousePressEvent(event);
}

void NavigatorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_mouseDown = false;
}

void NavigatorGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mouseDown)
        emit moveMainViewTo(mapToScene(event->pos()));
}
