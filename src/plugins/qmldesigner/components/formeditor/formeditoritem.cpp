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

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "formeditornodeinstanceview.h"
#include "selectiontool.h"

#include <modelnode.h>
#include <nodemetainfo.h>
#include <widgetqueryview.h>

#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsView>
#include <QTimeLine>

#include <cmath>

#include <invalidmodelnodeexception.h>
#include <invalidnodestateexception.h>

namespace QmlDesigner {



FormEditorScene *FormEditorItem::scene() const {
    return qobject_cast<FormEditorScene*>(QGraphicsItem::scene());
}

FormEditorItem::FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene)
    : QGraphicsObject(scene->formLayerItem()),
    m_snappingLineCreator(this),
    m_qmlItemNode(qmlItemNode),
    m_borderWidth(1.0),
    m_opacity(0.6),
    m_highlightBoundingRect(false)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setup();
}

void FormEditorItem::setup()
{
    if (qmlItemNode().hasInstanceParent())
        setParentItem(scene()->itemForQmlItemNode(qmlItemNode().instanceParent().toQmlItemNode()));

    if (QGraphicsItem::parentItem() == scene()->formLayerItem())
        m_borderWidth = 0.0;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    updateGeometry();
    updateVisibilty();
}

QRectF FormEditorItem::boundingRect() const
{
    return m_boundingRect;
}

void FormEditorItem::updateGeometry()
{
    prepareGeometryChange();
    m_boundingRect = qmlItemNode().instanceBoundingRect().adjusted(0, 0, 1., 1.);
    setTransform(qmlItemNode().instanceTransform());
    setTransform(m_attentionTransform, true);
    //the property for zValue is called z in QGraphicsObject
    Q_ASSERT(qmlItemNode().instanceValue("z").isValid());
    setZValue(qmlItemNode().instanceValue("z").toDouble());
}

void FormEditorItem::updateVisibilty()
{
//    setVisible(nodeInstance().isVisible());
//    setOpacity(nodeInstance().opacity());
}

void FormEditorItem::showAttention()
{
    if (m_attentionTimeLine.isNull()) {
        m_attentionTimeLine = new QTimeLine(500, this);
        m_attentionTimeLine->setCurveShape(QTimeLine::SineCurve);
        connect(m_attentionTimeLine.data(), SIGNAL(valueChanged(qreal)), SLOT(changeAttention(qreal)));
        connect(m_attentionTimeLine.data(), SIGNAL(finished()), m_attentionTimeLine.data(), SLOT(deleteLater()));

        m_attentionTimeLine->start();
    }
}

void FormEditorItem::changeAttention(qreal value)
{
    if (QGraphicsItem::parentItem() == scene()->formLayerItem()) {
        setAttentionHighlight(value);
    } else {
        setAttentionHighlight(value);
        setAttentionScale(value);
    }
}

FormEditorView *FormEditorItem::formEditorView() const
{
    return scene()->editorView();
}

void FormEditorItem::setAttentionScale(double sinusScale)
{
    if (!qFuzzyIsNull(sinusScale)) {
        double scale = std::sqrt(sinusScale);
        m_attentionTransform.reset();
        QPointF centerPoint(qmlItemNode().instanceBoundingRect().center());
        m_attentionTransform.translate(centerPoint.x(), centerPoint.y());
        m_attentionTransform.scale(scale * 0.15 + 1.0, scale * 0.15 + 1.0);
        m_attentionTransform.translate(-centerPoint.x(), -centerPoint.y());
        m_inverseAttentionTransform = m_attentionTransform.inverted();
        prepareGeometryChange();
        setTransform(qmlItemNode().instanceTransform());
        setTransform(m_attentionTransform, true);
    } else {
        m_attentionTransform.reset();
        prepareGeometryChange();
        setTransform(qmlItemNode().instanceTransform());
    }
}

void FormEditorItem::setAttentionHighlight(double value)
{
    m_opacity = 0.6 + value;
    if (QGraphicsItem::parentItem() == scene()->formLayerItem())
        m_borderWidth = value * 4;
    else
        m_borderWidth = 1. + value * 3;

    update();
}

void FormEditorItem::setHighlightBoundingRect(bool highlight)
{
    if (m_highlightBoundingRect != highlight) {
        m_highlightBoundingRect = highlight;
        update();
    }
}

FormEditorItem::~FormEditorItem()
{
   scene()->removeItemFromHash(this);
}

/* \brief returns the parent item skipping all proxyItem*/
FormEditorItem *FormEditorItem::parentItem() const
{
    return qgraphicsitem_cast<FormEditorItem*> (QGraphicsItem::parentItem());
}

FormEditorItem* FormEditorItem::fromQGraphicsItem(QGraphicsItem *graphicsItem)
{
    return qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
}

//static QRectF alignedRect(const QRectF &rect)
//{
//    QRectF alignedRect(rect);
//    alignedRect.setTop(std::floor(rect.top()) + 0.5);
//    alignedRect.setBottom(std::floor(rect.bottom()) + 0.5);
//    alignedRect.setLeft(std::floor(rect.left()) + 0.5);
//    alignedRect.setRight(std::floor(rect.right()) + 0.5);
//
//    return alignedRect;
//}

void FormEditorItem::paintBoundingRect(QPainter *painter) const
{
    if (!boundingRect().isValid()
        || (QGraphicsItem::parentItem() == scene()->formLayerItem() && qFuzzyIsNull(m_borderWidth)))
          return;

    QPen pen;
    pen.setJoinStyle(Qt::MiterJoin);

    switch(scene()->paintMode()) {
        case FormEditorScene::AnchorMode: {
                pen.setColor(Qt::black);
                pen.setWidth(m_borderWidth);
            }
            break;
        case FormEditorScene::NormalMode: {
                QColor frameColor("#AAAAAA");

                if (qmlItemNode().anchors().instanceHasAnchors())
                        frameColor = QColor("#ffff00");

                if (scene()->showBoundingRects()) {
                    if (m_highlightBoundingRect)
                        pen.setColor(frameColor);
                    else
                        pen.setColor(frameColor.darker(150));
                } else {
                    if (m_highlightBoundingRect)
                        pen.setColor(frameColor);
                    else
                        pen.setColor(Qt::transparent);
                }
            }
            break;
    }

    painter->setPen(pen);
//    int offset =  m_borderWidth / 2;

    painter->drawRect(boundingRect().adjusted(0., 0., -1., -1.));
}

void FormEditorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!qmlItemNode().isValid())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    switch(scene()->paintMode()) {
        case FormEditorScene::AnchorMode:
            painter->setOpacity(m_opacity);
            break;
        case FormEditorScene::NormalMode:
            painter->setOpacity(qmlItemNode().instanceValue("opacity").toDouble());
            break;
    }

    qmlItemNode().paintInstance(painter);

    painter->setRenderHint(QPainter::Antialiasing, false);

    if (!qmlItemNode().isRootModelNode())
        paintBoundingRect(painter);

    painter->restore();
}

AbstractFormEditorTool* FormEditorItem::tool() const
{
    return static_cast<FormEditorScene*>(scene())->currentTool();
}

SnapLineMap FormEditorItem::topSnappingLines() const
{
    return m_snappingLineCreator.topLines();
}

SnapLineMap FormEditorItem::bottomSnappingLines() const
{
    return m_snappingLineCreator.bottomLines();
}

SnapLineMap FormEditorItem::leftSnappingLines() const
{
    return m_snappingLineCreator.leftLines();
}

SnapLineMap FormEditorItem::rightSnappingLines() const
{
    return m_snappingLineCreator.rightLines();
}

SnapLineMap FormEditorItem::horizontalCenterSnappingLines() const
{
    return m_snappingLineCreator.horizontalCenterLines();
}

SnapLineMap FormEditorItem::verticalCenterSnappingLines() const
{
    return m_snappingLineCreator.verticalCenterLines();
}

SnapLineMap FormEditorItem::topSnappingOffsets() const
{
    return m_snappingLineCreator.topOffsets();
}

SnapLineMap FormEditorItem::bottomSnappingOffsets() const
{
    return m_snappingLineCreator.bottomOffsets();
}

SnapLineMap FormEditorItem::leftSnappingOffsets() const
{
    return m_snappingLineCreator.leftOffsets();
}

SnapLineMap FormEditorItem::rightSnappingOffsets() const
{
    return m_snappingLineCreator.rightOffsets();
}


void FormEditorItem::updateSnappingLines(const QList<FormEditorItem*> &exceptionList,
                                         FormEditorItem *transformationSpaceItem)
{
    m_snappingLineCreator.update(exceptionList, transformationSpaceItem);
}


QList<FormEditorItem*> FormEditorItem::childFormEditorItems() const
{
    QList<FormEditorItem*> formEditorItemList;

    foreach (QGraphicsItem *item, childItems()) {
        FormEditorItem *formEditorItem = fromQGraphicsItem(item);
        if (formEditorItem)
            formEditorItemList.append(formEditorItem);
    }

    return formEditorItemList;
}

bool FormEditorItem::isContainer() const
{
    return qmlItemNode().modelNode().metaInfo().isContainer();
}

QmlItemNode FormEditorItem::qmlItemNode() const
{
    return m_qmlItemNode;
}

}

