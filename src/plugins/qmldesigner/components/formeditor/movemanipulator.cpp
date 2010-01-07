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

#include "movemanipulator.h"
#include "itemutilfunctions.h"
#include "layeritem.h"
#include "formeditoritem.h"
#include "formeditorscene.h"

#include <QPointF>
#include <QtDebug>
#include <QColor>
#include <QPen>
#include <QApplication>

#include <limits>
#include <model.h>
#include <qmlanchors.h>


namespace QmlDesigner {

MoveManipulator::MoveManipulator(LayerItem *layerItem, FormEditorView *view)
    : m_layerItem(layerItem),
    m_view(view),
    m_isActive(false)
{
}

MoveManipulator::~MoveManipulator()
{
    deleteSnapLines();
}

QPointF MoveManipulator::beginPoint() const
{
    return m_beginPoint;
}

void MoveManipulator::setItem(FormEditorItem* item)
{
    QList<FormEditorItem*> itemList;
    itemList.append(item);

    setItems(itemList);
}

void MoveManipulator::setItems(const QList<FormEditorItem*> &itemList)
{
    m_itemList = itemList;
    if (!m_itemList.isEmpty()) {
        if (m_itemList.first()->parentItem())
            m_snapper.setContainerFormEditorItem(m_itemList.first()->parentItem());
        else
            m_snapper.setContainerFormEditorItem(m_itemList.first());
        m_snapper.setTransformtionSpaceFormEditorItem(m_snapper.containerFormEditorItem());
    }
}

void MoveManipulator::updateHashes()
{
//    foreach (FormEditorItem* item, m_itemList)
//        m_beginItemRectHash[item] = item->mapRectToParent(item->qmlItemNode().instanceBoundingRect());

    foreach (FormEditorItem* item, m_itemList) {
        QPointF positionInParentSpace = m_snapper.containerFormEditorItem()->mapFromScene(m_beginPositionInSceneSpaceHash.value(item));
        m_beginItemRectHash[item].translate(positionInParentSpace - m_beginPositionHash.value(item));
        m_beginPositionHash.insert(item, positionInParentSpace);
    }
}

bool MoveManipulator::itemsCanReparented() const
{
    foreach (FormEditorItem* item, m_itemList) {
        if (!item->qmlItemNode().canReparent())
            return false;
    }

    return true;
}

void MoveManipulator::begin(const QPointF &beginPoint)
{
    m_isActive = true;

    m_snapper.updateSnappingLines(m_itemList);


    foreach (FormEditorItem* item, m_itemList)
        m_beginItemRectHash.insert(item, m_snapper.containerFormEditorItem()->mapRectFromItem(item, item->qmlItemNode().instanceBoundingRect()));

    foreach (FormEditorItem* item, m_itemList) {
        QPointF positionInParentSpace(item->qmlItemNode().instancePosition());
        QPointF positionInScenesSpace = m_snapper.containerFormEditorItem()->mapToScene(positionInParentSpace);
        m_beginPositionInSceneSpaceHash.insert(item, positionInScenesSpace);
    }

    foreach (FormEditorItem* item, m_itemList) {
        QPointF positionInParentSpace = m_snapper.containerFormEditorItem()->mapFromScene(m_beginPositionInSceneSpaceHash.value(item));
        m_beginPositionHash.insert(item, positionInParentSpace);

        QmlAnchors anchors(item->qmlItemNode().anchors());
        m_beginTopMarginHash.insert(item, anchors.instanceMargin(AnchorLine::Top));
        m_beginLeftMarginHash.insert(item, anchors.instanceMargin(AnchorLine::Left));
        m_beginRightMarginHash.insert(item, anchors.instanceMargin(AnchorLine::Right));
        m_beginBottomMarginHash.insert(item, anchors.instanceMargin(AnchorLine::Bottom));
        m_beginHorizontalCenterHash.insert(item, anchors.instanceMargin(AnchorLine::HorizontalCenter));
        m_beginVerticalCenterHash.insert(item, anchors.instanceMargin(AnchorLine::VerticalCenter));
    }

    m_beginPoint = beginPoint;

//    setOpacityForAllElements(0.62);

    m_rewriterTransaction = m_view->beginRewriterTransaction();
}





QPointF MoveManipulator::findSnappingOffset(const QList<QRectF> &boundingRectList)
{
    QPointF offset;

    QMap<double, double> verticalOffsetMap;
    foreach (const QRectF &boundingRect, boundingRectList) {
        double verticalOffset = m_snapper.snappedVerticalOffset(boundingRect);
        if (verticalOffset < std::numeric_limits<double>::max())
            verticalOffsetMap.insert(qAbs(verticalOffset), verticalOffset);
    }


    if (!verticalOffsetMap.isEmpty())
        offset.rx() = verticalOffsetMap.begin().value();



    QMap<double, double> horizontalOffsetMap;
    foreach (const QRectF &boundingRect, boundingRectList) {
        double horizontalOffset = m_snapper.snappedHorizontalOffset(boundingRect);
        if (horizontalOffset < std::numeric_limits<double>::max())
            horizontalOffsetMap.insert(qAbs(horizontalOffset), horizontalOffset);
    }


    if (!horizontalOffsetMap.isEmpty())
        offset.ry() = horizontalOffsetMap.begin().value();

    return offset;
}


void MoveManipulator::generateSnappingLines(const QList<QRectF> &boundingRectList)
{
    m_graphicsLineList = m_snapper.generateSnappingLines(boundingRectList,
                                                         m_layerItem.data(),
                                                         m_snapper.transformtionSpaceFormEditorItem()->sceneTransform());
}



QList<QRectF> MoveManipulator::tanslatedBoundingRects(const QList<QRectF> &boundingRectList, const QPointF& offsetVector)
{
    QList<QRectF> translatedBoundingRectList;
    foreach (const QRectF &boundingRect, boundingRectList)
        translatedBoundingRectList.append(boundingRect.translated(offsetVector));

    return translatedBoundingRectList;
}



/*
  /brief updates the position of the items.
*/
void MoveManipulator::update(const QPointF& updatePoint, Snapping useSnapping)
{
    deleteSnapLines(); //Since they position is changed and the item is moved the snapping lines are
                       //are obsolete. The new updated snapping lines (color and visibility) will be
                       //calculated in snapPoint() called in moveNode() later

    if (m_itemList.isEmpty()) {
        return;
    } else {
        QPointF updatePointInContainerSpace(m_snapper.containerFormEditorItem()->mapFromScene(updatePoint));
        QPointF beginPointInContainerSpace(m_snapper.containerFormEditorItem()->mapFromScene(m_beginPoint));

        QPointF offsetVector(updatePointInContainerSpace - beginPointInContainerSpace);
        if (useSnapping == UseSnapping) {
            offsetVector -= findSnappingOffset(tanslatedBoundingRects(m_beginItemRectHash.values(), offsetVector));
            generateSnappingLines(tanslatedBoundingRects(m_beginItemRectHash.values(), offsetVector));
        }

        foreach (FormEditorItem* item, m_itemList) {
            QPointF positionInContainerSpace(m_beginPositionHash.value(item) + offsetVector);
            QmlAnchors anchors(item->qmlItemNode().anchors());

            if (anchors.instanceHasAnchor(AnchorLine::Top)) {
               anchors.setMargin(AnchorLine::Top, m_beginTopMarginHash.value(item) + offsetVector.y());
            }

            if (anchors.instanceHasAnchor(AnchorLine::Left)) {
               anchors.setMargin(AnchorLine::Left, m_beginLeftMarginHash.value(item) + offsetVector.x());
            }

            if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
               anchors.setMargin(AnchorLine::Bottom, m_beginBottomMarginHash.value(item) - offsetVector.y());
            }

            if (anchors.instanceHasAnchor(AnchorLine::Right)) {
               anchors.setMargin(AnchorLine::Right, m_beginRightMarginHash.value(item) - offsetVector.x());
            }

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
               anchors.setMargin(AnchorLine::HorizontalCenter, m_beginHorizontalCenterHash.value(item) + offsetVector.x());
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
               anchors.setMargin(AnchorLine::VerticalCenter, m_beginVerticalCenterHash.value(item) + offsetVector.y());
            }

            item->qmlItemNode().setPosition(positionInContainerSpace);
        }


    }
}

void MoveManipulator::clear()
{
    deleteSnapLines();
    m_beginItemRectHash.clear();
    m_beginPositionHash.clear();
    m_beginPositionInSceneSpaceHash.clear();
    m_itemList.clear();

    m_rewriterTransaction.commit();

    m_beginTopMarginHash.clear();
    m_beginLeftMarginHash.clear();
    m_beginRightMarginHash.clear();
    m_beginBottomMarginHash.clear();
    m_beginHorizontalCenterHash.clear();
    m_beginVerticalCenterHash.clear();
}

void MoveManipulator::reparentTo(FormEditorItem *newParent)
{
    deleteSnapLines();

    if (!newParent)
        return;

    if (!itemsCanReparented())
        return;

    foreach (FormEditorItem* item, m_itemList) {
        QmlItemNode parent(newParent->qmlItemNode());
        if (parent.isValid()) {
            item->qmlItemNode().setParentProperty(parent.nodeAbstractProperty("data"));
        }
    }

    if (m_view->model()) {
        m_snapper.setContainerFormEditorItem(newParent);
        m_snapper.setTransformtionSpaceFormEditorItem(m_snapper.containerFormEditorItem());
        m_snapper.updateSnappingLines(m_itemList);
        updateHashes();
    }
}


void MoveManipulator::end(const QPointF &/*endPoint*/)
{
    m_isActive = false;
    deleteSnapLines();
//    setOpacityForAllElements(1.0);
    clear();
}

void MoveManipulator::moveBy(double deltaX, double deltaY)
{
    foreach (FormEditorItem* item, m_itemList) {
        QmlAnchors anchors(item->qmlItemNode().anchors());

        if (anchors.instanceHasAnchor(AnchorLine::Top)) {
            anchors.setMargin(AnchorLine::Top, anchors.instanceMargin(AnchorLine::Top) - deltaY);
        }

        if (anchors.instanceHasAnchor(AnchorLine::Left)) {
            anchors.setMargin(AnchorLine::Left, anchors.instanceMargin(AnchorLine::Left) + deltaX);
        }

        if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
            anchors.setMargin(AnchorLine::Bottom, anchors.instanceMargin(AnchorLine::Bottom) + deltaY);
        }

        if (anchors.instanceHasAnchor(AnchorLine::Right)) {
            anchors.setMargin(AnchorLine::Right, anchors.instanceMargin(AnchorLine::Right) - deltaX);
        }

        if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
            anchors.setMargin(AnchorLine::HorizontalCenter, anchors.instanceMargin(AnchorLine::HorizontalCenter) + deltaX);
        }

        if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
            anchors.setMargin(AnchorLine::VerticalCenter, anchors.instanceMargin(AnchorLine::VerticalCenter) + deltaY);
        }

        item->qmlItemNode().setPosition(QPointF(item->qmlItemNode().instanceValue("x").toDouble() + deltaX,
                                      item->qmlItemNode().instanceValue("y").toDouble() + deltaY));
    }
}

void MoveManipulator::setOpacityForAllElements(qreal opacity)
{
    foreach (FormEditorItem* item, m_itemList)
        item->setOpacity(opacity);
}

void MoveManipulator::deleteSnapLines()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_graphicsLineList)
            m_layerItem->scene()->removeItem(item);
    }
    m_graphicsLineList.clear();
    m_view->scene()->update();
}

bool MoveManipulator::isActive() const
{
    return m_isActive;
}

}
