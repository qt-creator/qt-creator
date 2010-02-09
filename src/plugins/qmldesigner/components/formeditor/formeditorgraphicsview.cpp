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

#include <qmlanchors.h>

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

void FormEditorGraphicsView::drawForeground(QPainter *painter, const QRectF &/*rect*/ )
{
    if (!m_feedbackNode.isValid())
        return;

    painter->save();

    painter->translate(mapToScene(QPoint(3, 3)));

    QColor changeColor(QColor(Qt::green).lighter(170));

    QFont font;
    font.setPixelSize(12);
    painter->setFont(font);

    painter->save();
    painter->setOpacity(0.7);

    QLinearGradient gradient(QPoint(0, 0), QPoint(100, 0));
    gradient.setColorAt(0.0, Qt::darkGray);
    gradient.setColorAt(1.0, Qt::gray);
    painter->setBrush(gradient);

    painter->setPen(Qt::black);
    int height = 40;
    if (m_feedbackNode.instanceHasAnchors())
        height += 36;
    painter->drawRoundedRect(QRect(-1, -1, 100, height), 5, 5);
    painter->restore();


    if (m_feedbackNode.instanceHasAnchors())
    if (m_beginXHasExpression) {
        if(m_feedbackNode.hasBindingProperty("x"))
            painter->setPen(Qt::blue);
        else
            painter->setPen(Qt::red);
    } else {
        if (m_beginX != m_feedbackNode.instanceValue("x"))
            painter->setPen(changeColor);
        else
            painter->setPen(Qt::black);
    }

    painter->drawText(QPoint(2.0, 12.0), QString("x:"));
    painter->drawText(QPoint(14.0, 12.0), m_feedbackNode.instanceValue("x").toString());


    if (m_beginYHasExpression) {
        if(m_feedbackNode.hasBindingProperty("y"))
            painter->setPen(Qt::blue);
        else
            painter->setPen(Qt::red);
    } else {
        if (m_beginY != m_feedbackNode.instanceValue("y"))
            painter->setPen(changeColor);
        else
            painter->setPen(Qt::black);
    }

    painter->drawText(QPoint(50.0, 12.0), QString("y:"));
    painter->drawText(QPoint(60.0, 12.0), m_feedbackNode.instanceValue("y").toString());


    if (m_beginWidthHasExpression) {
        if(m_feedbackNode.hasBindingProperty("width"))
            painter->setPen(Qt::blue);
        else
            painter->setPen(Qt::red);
    } else {
        if (m_beginWidth != m_feedbackNode.instanceValue("width"))
            painter->setPen(changeColor);
        else
            painter->setPen(Qt::black);
    }

    painter->drawText(QPoint(2.0, 24.0), QString("w:"));
    painter->drawText(QPoint(14.0, 24.0), m_feedbackNode.instanceValue("width").toString());


    if (m_beginHeightHasExpression) {
        if(m_feedbackNode.hasBindingProperty("height"))
            painter->setPen(Qt::blue);
        else
            painter->setPen(Qt::red);
    } else {
        if (m_beginHeight != m_feedbackNode.instanceValue("height"))
            painter->setPen(changeColor);
        else
            painter->setPen(Qt::black);
    }

    painter->drawText(QPoint(50.0, 24.0), QString("h:"));
    painter->drawText(QPoint(60.0, 24.0),m_feedbackNode.instanceValue("height").toString());

    if (m_parentNode == m_feedbackNode.instanceParent()) {

        if (!m_feedbackNode.canReparent()) {
            painter->setPen(Qt::red);
            painter->drawText(QPoint(2.0, 36.0), QString("Cannot reparent"));
        }
    } else {
        painter->setPen(Qt::yellow);
        painter->drawText(QPoint(2.0, 36.0), QString("Parent changed"));

    }


    if (m_feedbackNode.instanceHasAnchors()) {
        if (m_beginTopMargin != m_feedbackNode.instanceValue("anchors.leftMargin"))
            painter->setPen(changeColor);
        else if (m_feedbackNode.anchors().instanceHasAnchor(AnchorLine::Left))
            painter->setPen(Qt::yellow);
        else
            painter->setPen(Qt::black);

        painter->drawText(QPoint(2.0, 48.0), QString("l:"));
        painter->drawText(QPoint(14.0, 48.0), m_feedbackNode.instanceValue("anchors.leftMargin").toString());


        if (m_beginRightMargin != m_feedbackNode.instanceValue("anchors.rightMargin"))
            painter->setPen(changeColor);
        else if (m_feedbackNode.anchors().instanceHasAnchor(AnchorLine::Right))
            painter->setPen(Qt::yellow);
        else
            painter->setPen(Qt::black);

        painter->drawText(QPoint(50.0, 48.0), QString("r:"));
        painter->drawText(QPoint(60.0, 48.0), m_feedbackNode.instanceValue("anchors.rightMargin").toString());


        if (m_beginTopMargin != m_feedbackNode.instanceValue("anchors.topMargin"))
            painter->setPen(changeColor);
        else if (m_feedbackNode.anchors().instanceHasAnchor(AnchorLine::Top))
            painter->setPen(Qt::yellow);
        else
            painter->setPen(Qt::black);

        painter->drawText(QPoint(2.0, 60.0), QString("t:"));
        painter->drawText(QPoint(14.0, 60.0), m_feedbackNode.instanceValue("anchors.topMargin").toString());


        if (m_beginBottomMargin != m_feedbackNode.instanceValue("anchors.bottomMargin"))
            painter->setPen(changeColor);
        else if (m_feedbackNode.anchors().instanceHasAnchor(AnchorLine::Bottom))
            painter->setPen(Qt::yellow);
        else
            painter->setPen(Qt::black);

        painter->drawText(QPoint(50.0, 60.0), QString("h:"));
        painter->drawText(QPoint(60.0, 60.0), m_feedbackNode.instanceValue("anchors.bottomMargin").toString());

        if (m_beginTopMargin != m_feedbackNode.instanceValue("anchors.horizontalCenterOffset"))
            painter->setPen(changeColor);
        else if (m_feedbackNode.anchors().instanceHasAnchor(AnchorLine::HorizontalCenter))
            painter->setPen(Qt::yellow);
        else
            painter->setPen(Qt::black);
        painter->drawText(QPoint(2.0, 72.0), QString("h:"));
        painter->drawText(QPoint(14.0, 72.0), m_feedbackNode.instanceValue("anchors.horizontalCenterOffset").toString());


        if (m_beginBottomMargin != m_feedbackNode.instanceValue("anchors.verticalCenterOffset"))
            painter->setPen(changeColor);
        else if (m_feedbackNode.anchors().instanceHasAnchor(AnchorLine::VerticalCenter))
            painter->setPen(Qt::yellow);
        else
            painter->setPen(Qt::black);

        painter->drawText(QPoint(50.0, 72.0), QString("v:"));
        painter->drawText(QPoint(60.0, 72.0), m_feedbackNode.instanceValue("anchors.verticalCenterOffset").toString());
    }

    painter->restore();
}

void FormEditorGraphicsView::setFeedbackNode(const QmlItemNode &node)
{
    if (node == m_feedbackNode)
        return;

    m_feedbackNode = node;

    if (m_feedbackNode.isValid()) {
        m_beginX = m_feedbackNode.instanceValue("x");
        m_beginY = m_feedbackNode.instanceValue("y");
        m_beginWidth = m_feedbackNode.instanceValue("width");
        m_beginHeight = m_feedbackNode.instanceValue("height");
        m_parentNode = m_feedbackNode.instanceParent();
        m_beginLeftMargin = m_feedbackNode.instanceValue("anchors.leftMargin");
        m_beginRightMargin = m_feedbackNode.instanceValue("anchors.rightMargin");
        m_beginTopMargin = m_feedbackNode.instanceValue("anchors.topMargin");
        m_beginBottomMargin = m_feedbackNode.instanceValue("anchors.bottomMargin");
        m_beginXHasExpression = m_feedbackNode.hasBindingProperty("x");
        m_beginYHasExpression = m_feedbackNode.hasBindingProperty("y");
        m_beginWidthHasExpression = m_feedbackNode.hasBindingProperty("width");
        m_beginHeightHasExpression = m_feedbackNode.hasBindingProperty("height");
    } else {
        m_beginX = QVariant();
        m_beginY = QVariant();
        m_beginWidth = QVariant();
        m_beginHeight = QVariant();
        m_parentNode = QmlObjectNode();
        m_beginLeftMargin = QVariant();
        m_beginRightMargin = QVariant();
        m_beginTopMargin = QVariant();
        m_beginBottomMargin = QVariant();
        m_beginXHasExpression = false;
        m_beginYHasExpression = false;
        m_beginWidthHasExpression = false;
        m_beginHeightHasExpression = false;
    }
}

void FormEditorGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setBrushOrigin(0, 0);
    painter->fillRect(rect.intersected(sceneRect()), backgroundBrush());
    // paint rect around editable area
    painter->setPen(Qt::black);
    QRectF frameRect = sceneRect().adjusted(0, 0, 0, 0);
    painter->drawRect(frameRect);
    painter->restore();
}

} // namespace QmlDesigner
