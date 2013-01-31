/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "movemanipulator.h"
#include "itemutilfunctions.h"
#include "layeritem.h"
#include "formeditoritem.h"
#include "formeditorscene.h"

#include <QPointF>
#include <QDebug>
#include <QColor>
#include <QPen>
#include <QApplication>

#include <limits>
#include <model.h>
#include <qmlanchors.h>
#include <nodemetainfo.h>
#include <variantproperty.h>
#include <nodeabstractproperty.h>


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


void MoveManipulator::synchronizeParent(const QList<FormEditorItem*> &itemList, const ModelNode &parentNode)
{
    bool snapperUpdated = false;

    foreach (FormEditorItem *item, itemList) {
        if (m_itemList.contains(item)) {
            QmlItemNode parentItemNode = QmlItemNode(parentNode);
            if (parentItemNode.isValid()) {
                if (snapperUpdated == false && m_snapper.containerFormEditorItem() != m_view->scene()->itemForQmlItemNode(parentItemNode)) {
                    m_snapper.setContainerFormEditorItem(m_view->scene()->itemForQmlItemNode(parentItemNode));
                    m_snapper.setTransformtionSpaceFormEditorItem(m_snapper.containerFormEditorItem());
                    m_snapper.updateSnappingLines(m_itemList);
                    updateHashes();
                    snapperUpdated = true;
                }
            }
        }
    }

    if (!parentNode.metaInfo().isSubclassOf("<cpp>.QDeclarativeBasePositioner", -1, -1))
        update(m_lastPosition, NoSnapping, UseBaseState);
}

void MoveManipulator::synchronizeInstanceParent(const QList<FormEditorItem*> &itemList)
{
    if (m_view->model() && !m_itemList.isEmpty() && m_itemList.first()->qmlItemNode().instanceParent().isValid())
        synchronizeParent(itemList, m_itemList.first()->qmlItemNode().instanceParent());
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
        if (item
            && item->qmlItemNode().isValid()
            && !item->qmlItemNode().canReparent())
            return false;
    }

    return true;
}

void MoveManipulator::begin(const QPointF &beginPoint)
{
    m_isActive = true;

    m_snapper.updateSnappingLines(m_itemList);


    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid())
            m_beginItemRectHash.insert(item, m_snapper.containerFormEditorItem()->mapRectFromItem(item, item->qmlItemNode().instanceBoundingRect()));
    }

    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid()) {
            QPointF positionInParentSpace(item->qmlItemNode().instancePosition());
            QPointF positionInScenesSpace = m_snapper.containerFormEditorItem()->mapToScene(positionInParentSpace);
            m_beginPositionInSceneSpaceHash.insert(item, positionInScenesSpace);
        }
    }

    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid()) {
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
    }

    m_beginPoint = beginPoint;

//    setOpacityForAllElements(0.62);

    m_rewriterTransaction = m_view->beginRewriterTransaction();
}





QPointF MoveManipulator::findSnappingOffset(const QHash<FormEditorItem*, QRectF> &boundingRectHash)
{
    QPointF offset;

    QMap<double, double> verticalOffsetMap;
    QMap<double, double> horizontalOffsetMap;

    QHashIterator<FormEditorItem*, QRectF> hashIterator(boundingRectHash);
    while (hashIterator.hasNext()) {
        hashIterator.next();
        FormEditorItem *formEditorItem = hashIterator.key();
        QRectF boundingRect = hashIterator.value();

        if (!formEditorItem || !formEditorItem->qmlItemNode().isValid())
            continue;

        if (!formEditorItem->qmlItemNode().hasBindingProperty("x")) {
            double verticalOffset = m_snapper.snappedVerticalOffset(boundingRect);
            if (verticalOffset < std::numeric_limits<double>::max())
                verticalOffsetMap.insert(qAbs(verticalOffset), verticalOffset);
        }

        if (!formEditorItem->qmlItemNode().hasBindingProperty("y")) {
            double horizontalOffset = m_snapper.snappedHorizontalOffset(boundingRect);
            if (horizontalOffset < std::numeric_limits<double>::max())
                horizontalOffsetMap.insert(qAbs(horizontalOffset), horizontalOffset);
        }
    }


    if (!verticalOffsetMap.isEmpty())
        offset.rx() = verticalOffsetMap.begin().value();


    if (!horizontalOffsetMap.isEmpty())
        offset.ry() = horizontalOffsetMap.begin().value();

    return offset;
}


void MoveManipulator::generateSnappingLines(const QHash<FormEditorItem*, QRectF> &boundingRectHash)
{
    m_graphicsLineList = m_snapper.generateSnappingLines(boundingRectHash.values(),
                                                         m_layerItem.data(),
                                                         m_snapper.transformtionSpaceFormEditorItem()->sceneTransform());
}



QHash<FormEditorItem*, QRectF> MoveManipulator::tanslatedBoundingRects(const QHash<FormEditorItem*, QRectF> &boundingRectHash, const QPointF& offsetVector)
{
    QHash<FormEditorItem*, QRectF> translatedBoundingRectHash;

    QHashIterator<FormEditorItem*, QRectF> hashIterator(boundingRectHash);
    while (hashIterator.hasNext()) {
        QPointF alignedOffset(offsetVector);
        hashIterator.next();
        FormEditorItem *formEditorItem = hashIterator.key();
        QRectF boundingRect = hashIterator.value();

        if (!formEditorItem || !formEditorItem->qmlItemNode().isValid())
            continue;

        if (formEditorItem->qmlItemNode().hasBindingProperty("x"))
            alignedOffset.setX(0);
        if (formEditorItem->qmlItemNode().hasBindingProperty("y"))
            alignedOffset.setY(0);
        translatedBoundingRectHash.insert(formEditorItem, boundingRect.translated(offsetVector));
    }

    return translatedBoundingRectHash;
}



/*
  /brief updates the position of the items.
*/
void MoveManipulator::update(const QPointF& updatePoint, Snapping useSnapping, State stateToBeManipulated)
{
    m_lastPosition = updatePoint;
    deleteSnapLines(); //Since they position is changed and the item is moved the snapping lines are
                       //are obsolete. The new updated snapping lines (color and visibility) will be
                       //calculated in snapPoint() called in moveNode() later

    if (m_itemList.isEmpty()) {
        return;
    } else {
        QPointF updatePointInContainerSpace(m_snapper.containerFormEditorItem()->mapFromScene(updatePoint));
        QPointF beginPointInContainerSpace(m_snapper.containerFormEditorItem()->mapFromScene(m_beginPoint));

        QPointF offsetVector(updatePointInContainerSpace - beginPointInContainerSpace);

        if (useSnapping == UseSnappingAndAnchoring)
        {

        }

        if (useSnapping == UseSnapping || useSnapping == UseSnappingAndAnchoring) {
            offsetVector -= findSnappingOffset(tanslatedBoundingRects(m_beginItemRectHash, offsetVector));
            generateSnappingLines(tanslatedBoundingRects(m_beginItemRectHash, offsetVector));
        }

        foreach (FormEditorItem* item, m_itemList) {
            QPointF positionInContainerSpace(m_beginPositionHash.value(item) + offsetVector);

            if (!item || !item->qmlItemNode().isValid())
                continue;

            // don't support anchors for base state because it is not needed by the droptool
            if (stateToBeManipulated == UseActualState) {
                QmlAnchors anchors(item->qmlItemNode().anchors());

                if (anchors.instanceHasAnchor(AnchorLine::Top))
                    anchors.setMargin(AnchorLine::Top, m_beginTopMarginHash.value(item) + offsetVector.y());

                if (anchors.instanceHasAnchor(AnchorLine::Left))
                    anchors.setMargin(AnchorLine::Left, m_beginLeftMarginHash.value(item) + offsetVector.x());

                if (anchors.instanceHasAnchor(AnchorLine::Bottom))
                    anchors.setMargin(AnchorLine::Bottom, m_beginBottomMarginHash.value(item) - offsetVector.y());

                if (anchors.instanceHasAnchor(AnchorLine::Right))
                    anchors.setMargin(AnchorLine::Right, m_beginRightMarginHash.value(item) - offsetVector.x());

                if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter))
                    anchors.setMargin(AnchorLine::HorizontalCenter, m_beginHorizontalCenterHash.value(item) + offsetVector.x());

                if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter))
                    anchors.setMargin(AnchorLine::VerticalCenter, m_beginVerticalCenterHash.value(item) + offsetVector.y());

                setPosition(item->qmlItemNode(), positionInContainerSpace);
            } else {
                item->qmlItemNode().modelNode().variantProperty("x").setValue(qRound(positionInContainerSpace.x()));
                item->qmlItemNode().modelNode().variantProperty("y").setValue(qRound(positionInContainerSpace.y()));
            }
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
    m_lastPosition = QPointF();

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

    if (!newParent->qmlItemNode().modelNode().metaInfo().isPositioner()
            && newParent->qmlItemNode().modelNode().hasParentProperty()) {
        ModelNode grandParent = newParent->qmlItemNode().modelNode().parentProperty().parentModelNode();
        if (grandParent.metaInfo().isPositioner())
            newParent = m_view.data()->scene()->itemForQmlItemNode(QmlItemNode(grandParent));
    }



    QVector<ModelNode> nodeReparentVector;
    NodeAbstractProperty parentProperty;

    QmlItemNode parent(newParent->qmlItemNode());
    if (parent.isValid()) {
        if (parent.hasDefaultProperty())
            parentProperty = parent.nodeAbstractProperty(parent.defaultProperty());
        else
            parentProperty = parent.nodeAbstractProperty("data");

        foreach (FormEditorItem* item, m_itemList) {
            if (!item || !item->qmlItemNode().isValid())
                continue;

            if (parentProperty != item->qmlItemNode().modelNode().parentProperty())
                nodeReparentVector.append(item->qmlItemNode().modelNode());

        }

        foreach (const ModelNode &nodeToReparented, nodeReparentVector)
            parentProperty.reparentHere(nodeToReparented);

        synchronizeParent(m_itemList, parentProperty.parentModelNode());
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
        if (!item || !item->qmlItemNode().isValid())
            continue;

        QmlAnchors anchors(item->qmlItemNode().anchors());

        if (anchors.instanceHasAnchor(AnchorLine::Top))
            anchors.setMargin(AnchorLine::Top, anchors.instanceMargin(AnchorLine::Top) + deltaY);

        if (anchors.instanceHasAnchor(AnchorLine::Left))
            anchors.setMargin(AnchorLine::Left, anchors.instanceMargin(AnchorLine::Left) + deltaX);

        if (anchors.instanceHasAnchor(AnchorLine::Bottom))
            anchors.setMargin(AnchorLine::Bottom, anchors.instanceMargin(AnchorLine::Bottom) - deltaY);

        if (anchors.instanceHasAnchor(AnchorLine::Right))
            anchors.setMargin(AnchorLine::Right, anchors.instanceMargin(AnchorLine::Right) - deltaX);

        if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter))
            anchors.setMargin(AnchorLine::HorizontalCenter, anchors.instanceMargin(AnchorLine::HorizontalCenter) + deltaX);

        if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter))
            anchors.setMargin(AnchorLine::VerticalCenter, anchors.instanceMargin(AnchorLine::VerticalCenter) + deltaY);

        setPosition(item->qmlItemNode(), QPointF(item->qmlItemNode().instanceValue("x").toDouble() + deltaX,
                                                  item->qmlItemNode().instanceValue("y").toDouble() + deltaY));
    }
}

void MoveManipulator::beginRewriterTransaction()
{
    m_rewriterTransaction = m_view->beginRewriterTransaction();
}

void MoveManipulator::endRewriterTransaction()
{
    m_rewriterTransaction.commit();
}

void MoveManipulator::setOpacityForAllElements(qreal opacity)
{
    foreach (FormEditorItem* item, m_itemList)
        item->setOpacity(opacity);
}

void MoveManipulator::deleteSnapLines()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_graphicsLineList) {
            m_layerItem->scene()->removeItem(item);
            delete item;
        }
    }
    m_graphicsLineList.clear();
    m_view->scene()->update();
}

bool MoveManipulator::isActive() const
{
    return m_isActive;
}

void MoveManipulator::setPosition(QmlItemNode itemNode, const QPointF &position)
{
    if (!itemNode.hasBindingProperty("x"))
        itemNode.setVariantProperty("x", qRound(position.x()));

    if (!itemNode.hasBindingProperty("y"))
        itemNode.setVariantProperty("y", qRound(position.y()));
}

}
