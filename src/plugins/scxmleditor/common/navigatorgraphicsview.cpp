// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        if (event->angleDelta().y() > 0)
            emit zoomIn();
        else
            emit zoomOut();

        emit moveMainViewTo(mapToScene(event->position().toPoint()));
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
