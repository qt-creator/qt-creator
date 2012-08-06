/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "formeditorgraphicsview.h"

#include <QWheelEvent>
#include <QApplication>
#include <QDebug>

#include <qmlanchors.h>

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
    QGraphicsView::mouseMoveEvent(event);

//    if (rect().contains(event->pos())) {
//        QGraphicsView::mouseMoveEvent(event);
//    } else {
//        QPoint position = event->pos();
//        QPoint topLeft = rect().topLeft();
//        QPoint bottomRight = rect().bottomRight();
//        position.rx() = qMax(topLeft.x(), qMin(position.x(), bottomRight.x()));
//        position.ry() = qMax(topLeft.y(), qMin(position.y(), bottomRight.y()));
//        QMouseEvent *mouseEvent = QMouseEvent::createExtendedMouseEvent(event->type(), position, mapToGlobal(position), event->button(), event->buttons(), event->modifiers());

//        QGraphicsView::mouseMoveEvent(mouseEvent);
//        delete mouseEvent;
//    }

//    // Keeps the feedback bubble within screen boundraries
//    int tx = qMin(width() - 114,  qMax(16, event->pos().x() + 50));
//    int ty = qMin(height() - 45, qMax(10, event->pos().y() - 70));
//    m_feedbackOriginPoint = QPoint(tx, ty);
}

void FormEditorGraphicsView::keyPressEvent(QKeyEvent *event)
{
    m_feedbackOriginPoint = QPoint();
    QGraphicsView::keyPressEvent(event);
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

void FormEditorGraphicsView::mousePressEvent(QMouseEvent *mouseEvent)
{
    QGraphicsView::mousePressEvent(mouseEvent);
}

void FormEditorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
//    if (rect().contains(event->pos())) {
//        QGraphicsView::mouseReleaseEvent(event);
//    } else {
//        QPoint position = event->pos();
//        QPoint topLeft = rect().topLeft();
//        QPoint bottomRight = rect().bottomRight();
//        position.rx() = qMax(topLeft.x(), qMin(position.x(), bottomRight.x()));
//        position.ry() = qMax(topLeft.y(), qMin(position.y(), bottomRight.y()));
//        QMouseEvent *mouseEvent = QMouseEvent::createExtendedMouseEvent(event->type(), position, mapToGlobal(position), event->button(), event->buttons(), event->modifiers());

//        QGraphicsView::mouseReleaseEvent(mouseEvent);
//        delete mouseEvent;
//    }

//    m_feedbackOriginPoint = QPoint();
}

void FormEditorGraphicsView::leaveEvent(QEvent *event)
{
     m_feedbackOriginPoint = QPoint();
     QGraphicsView::leaveEvent(event);
 }

//static QPixmap createBubblePixmap()
//{
//    QPixmap pixmap(124, 48);
//    pixmap.fill(Qt::transparent);
//    QPainter pmPainter(&pixmap);
//    pmPainter.setRenderHint(QPainter::Antialiasing);
//    pmPainter.setOpacity(0.85);
//    pmPainter.translate(0.5, 0.5);
//    pmPainter.setPen(Qt::NoPen);
//    pmPainter.setBrush(QColor(0, 0, 0, 40));
//    pmPainter.drawRoundedRect(QRect(0, 0, 124, 48), 8, 8);
//    QLinearGradient gradient(QPoint(0, 0), QPoint(0, 44));
//    gradient.setColorAt(0.0, QColor(70, 70, 70));
//    gradient.setColorAt(1.0, QColor(10, 10, 10));
//    pmPainter.setBrush(gradient);
//    pmPainter.setPen(QColor(60, 60, 60));
//    pmPainter.drawRoundedRect(QRect(2, 1, 120, 45), 5, 5);
//    pmPainter.setBrush(Qt::NoBrush);
//    pmPainter.setPen(QColor(255, 255, 255, 140));
//    pmPainter.drawRoundedRect(QRect(3, 2, 118, 43), 5, 5);
//    pmPainter.end();
//    return pixmap;
//}

void FormEditorGraphicsView::drawForeground(QPainter * /*painter*/, const QRectF & /*rect*/ )
{
//    if (!m_feedbackNode.isValid())
//        return;

//    if (m_feedbackOriginPoint.isNull())
//        return;

//    painter->save();
//    painter->resetTransform();
//    painter->translate(m_feedbackOriginPoint);

//    QColor defaultColor(Qt::white);
//    QColor changeColor("#9999ff");

//    QFont font;
//    font.setFamily("Helvetica");
//    font.setPixelSize(12);
//    painter->setFont(font);

//    if (m_bubblePixmap.isNull())
//        m_bubblePixmap = createBubblePixmap();
//    painter->drawPixmap(-13, -7, m_bubblePixmap);

//    if (m_beginXHasExpression) {
//        if(m_feedbackNode.hasBindingProperty("x"))
//            painter->setPen(defaultColor);
//        else
//            painter->setPen(Qt::red);
//    } else {
//        if (m_beginX != m_feedbackNode.instanceValue("x"))
//            painter->setPen(changeColor);
//        else
//            painter->setPen(defaultColor);
//    }

//    painter->drawText(QPoint(8.0, 13.0), QString("x:"));
//    painter->drawText(QPoint(22.0, 13.0), m_feedbackNode.instanceValue("x").toString());


//    if (m_beginYHasExpression) {
//        if(m_feedbackNode.hasBindingProperty("y"))
//            painter->setPen(defaultColor);
//        else
//            painter->setPen(Qt::red);
//    } else {
//        if (m_beginY != m_feedbackNode.instanceValue("y"))
//            painter->setPen(changeColor);
//        else
//            painter->setPen(defaultColor);
//    }

//    painter->drawText(QPoint(60.0, 13.0), QString("y:"));
//    painter->drawText(QPoint(72.0, 13.0), m_feedbackNode.instanceValue("y").toString());


//    if (m_beginWidthHasExpression) {
//        if(m_feedbackNode.hasBindingProperty("width"))
//            painter->setPen(defaultColor);
//        else
//            painter->setPen(Qt::red);
//    } else {
//        if (m_beginWidth != m_feedbackNode.instanceValue("width"))
//            painter->setPen(changeColor);
//        else
//            painter->setPen(defaultColor);
//    }

//    painter->drawText(QPoint(8.0, 29.0), QString("w:"));
//    painter->drawText(QPoint(22.0, 29.0), m_feedbackNode.instanceValue("width").toString());


//    if (m_beginHeightHasExpression) {
//        if(m_feedbackNode.hasBindingProperty("height"))
//            painter->setPen(defaultColor);
//        else
//            painter->setPen(Qt::red);
//    } else {
//        if (m_beginHeight != m_feedbackNode.instanceValue("height"))
//            painter->setPen(changeColor);
//        else
//            painter->setPen(defaultColor);
//    }

//    painter->drawText(QPoint(60.0, 29.0), QString("h:"));
//    painter->drawText(QPoint(72.0, 29.0), m_feedbackNode.instanceValue("height").toString());

//    if (m_parentNode != m_feedbackNode.instanceParent()) {
//        painter->setPen(changeColor);
//        painter->drawText(QPoint(2.0, 39.0), QString("Parent changed"));

//    }

//    painter->restore();
}

void FormEditorGraphicsView::setFeedbackNode(const QmlItemNode & /*node*/)
{
//    if (node == m_feedbackNode)
//        return;

//    m_feedbackNode = node;

//    if (m_feedbackNode.isValid()) {
//        m_beginX = m_feedbackNode.instanceValue("x");
//        m_beginY = m_feedbackNode.instanceValue("y");
//        m_beginWidth = m_feedbackNode.instanceValue("width");
//        m_beginHeight = m_feedbackNode.instanceValue("height");
//        m_parentNode = m_feedbackNode.instanceParent();
//        m_beginLeftMargin = m_feedbackNode.instanceValue("anchors.leftMargin");
//        m_beginRightMargin = m_feedbackNode.instanceValue("anchors.rightMargin");
//        m_beginTopMargin = m_feedbackNode.instanceValue("anchors.topMargin");
//        m_beginBottomMargin = m_feedbackNode.instanceValue("anchors.bottomMargin");
//        m_beginXHasExpression = m_feedbackNode.hasBindingProperty("x");
//        m_beginYHasExpression = m_feedbackNode.hasBindingProperty("y");
//        m_beginWidthHasExpression = m_feedbackNode.hasBindingProperty("width");
//        m_beginHeightHasExpression = m_feedbackNode.hasBindingProperty("height");
//    } else {
//        m_beginX = QVariant();
//        m_beginY = QVariant();
//        m_beginWidth = QVariant();
//        m_beginHeight = QVariant();
//        m_parentNode = QmlObjectNode();
//        m_beginLeftMargin = QVariant();
//        m_beginRightMargin = QVariant();
//        m_beginTopMargin = QVariant();
//        m_beginBottomMargin = QVariant();
//        m_beginXHasExpression = false;
//        m_beginYHasExpression = false;
//        m_beginWidthHasExpression = false;
//        m_beginHeightHasExpression = false;
//    }
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
