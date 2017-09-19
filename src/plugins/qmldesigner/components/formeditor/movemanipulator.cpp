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

#include "movemanipulator.h"
#include "layeritem.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "formeditorwidget.h"
#include "formeditorgraphicsview.h"

#include <qmlanchors.h>
#include <nodehints.h>
#include <nodemetainfo.h>
#include <variantproperty.h>
#include <nodeabstractproperty.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QPointF>

#include <limits>

static Q_LOGGING_CATEGORY(moveManipulatorInfo, "qtc.qmldesigner.formeditor");

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
                    snapperUpdated = true;
                }
            }
        }
    }
}

void MoveManipulator::synchronizeInstanceParent(const QList<FormEditorItem*> &itemList)
{
    if (m_view->model() && !m_itemList.isEmpty() && m_itemList.first()->qmlItemNode().hasInstanceParent())
        synchronizeParent(itemList, m_itemList.first()->qmlItemNode().instanceParent());
}

bool MoveManipulator::itemsCanReparented() const
{
    foreach (FormEditorItem* item, m_itemList) {
        if (item
            && item->qmlItemNode().isValid()
            && !item->qmlItemNode().instanceCanReparent())
            return false;
    }

    return true;
}

void MoveManipulator::setDirectUpdateInNodeInstances(bool directUpdate)
{
    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid())
            item->qmlItemNode().nodeInstance().setDirectUpdate(directUpdate);
    }
}

void MoveManipulator::begin(const QPointF &beginPoint)
{
    m_view->formEditorWidget()->graphicsView()->viewport()->setCursor(Qt::SizeAllCursor);
    m_isActive = true;

    m_snapper.updateSnappingLines(m_itemList);

    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid()) {
            QTransform fromItemToSceneTransform = item->qmlItemNode().instanceSceneTransform();
            m_beginItemRectInSceneSpaceHash.insert(item, fromItemToSceneTransform.mapRect(item->qmlItemNode().instanceBoundingRect()));
        }
    }

    QTransform fromContentItemToSceneTransform = m_snapper.containerFormEditorItem()->qmlItemNode().instanceSceneContentItemTransform();
    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid()) {
            QPointF positionInScenesSpace = fromContentItemToSceneTransform.map(item->qmlItemNode().instancePosition());
            m_beginPositionInSceneSpaceHash.insert(item, positionInScenesSpace);
        }
    }

    foreach (FormEditorItem* item, m_itemList) {
        if (item && item->qmlItemNode().isValid()) {
            QmlAnchors anchors(item->qmlItemNode().anchors());
            m_beginTopMarginHash.insert(item, anchors.instanceMargin(AnchorLineTop));
            m_beginLeftMarginHash.insert(item, anchors.instanceMargin(AnchorLineLeft));
            m_beginRightMarginHash.insert(item, anchors.instanceMargin(AnchorLineRight));
            m_beginBottomMarginHash.insert(item, anchors.instanceMargin(AnchorLineBottom));
            m_beginHorizontalCenterHash.insert(item, anchors.instanceMargin(AnchorLineHorizontalCenter));
            m_beginVerticalCenterHash.insert(item, anchors.instanceMargin(AnchorLineVerticalCenter));
        }
    }

    m_beginPoint = beginPoint;

    setDirectUpdateInNodeInstances(true);

    beginRewriterTransaction();
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



QHash<FormEditorItem*, QRectF> MoveManipulator::tanslatedBoundingRects(const QHash<FormEditorItem*, QRectF> &boundingRectHash,
                                                                       const QPointF& offsetVector,
                                                                       const QTransform &transform)
{
    QHash<FormEditorItem*, QRectF> translatedBoundingRectHash;

    QHashIterator<FormEditorItem*, QRectF> hashIterator(boundingRectHash);
    while (hashIterator.hasNext()) {
        QPointF alignedOffset(offsetVector);
        hashIterator.next();
        FormEditorItem *formEditorItem = hashIterator.key();
        QRectF boundingRect = transform.mapRect(hashIterator.value());

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
void MoveManipulator::update(const QPointF& updatePoint, Snapper::Snapping useSnapping, State stateToBeManipulated)
{
    m_lastPosition = updatePoint;
    deleteSnapLines(); //Since they position is changed and the item is moved the snapping lines are
                       //are obsolete. The new updated snapping lines (color and visibility) will be
                       //calculated in snapPoint() called in moveNode() later

    if (m_itemList.isEmpty()) {
        return;
    } else {
        QTransform fromSceneToContentItemTransform = m_snapper.containerFormEditorItem()->qmlItemNode().instanceSceneContentItemTransform().inverted();
        QTransform fromSceneToItemTransform = m_snapper.containerFormEditorItem()->qmlItemNode().instanceSceneTransform().inverted();

        QPointF updatePointInContainerSpace = fromSceneToItemTransform.map(updatePoint);
        QPointF beginPointInContainerSpace = fromSceneToItemTransform.map(m_beginPoint);

        QPointF offsetVector = updatePointInContainerSpace - beginPointInContainerSpace;

        if (useSnapping == Snapper::UseSnapping || useSnapping == Snapper::UseSnappingAndAnchoring) {
            offsetVector -= findSnappingOffset(tanslatedBoundingRects(m_beginItemRectInSceneSpaceHash, offsetVector, fromSceneToItemTransform));
            generateSnappingLines(tanslatedBoundingRects(m_beginItemRectInSceneSpaceHash, offsetVector, fromSceneToItemTransform));
        }

        foreach (FormEditorItem* item, m_itemList) {
            QPointF positionInContainerSpace(fromSceneToContentItemTransform.map(m_beginPositionInSceneSpaceHash.value(item)) + offsetVector);

            if (!item || !item->qmlItemNode().isValid())
                continue;

            // don't support anchors for base state because it is not needed by the droptool
            if (stateToBeManipulated == UseCurrentState) {
                QmlAnchors anchors(item->qmlItemNode().anchors());

                if (anchors.instanceHasAnchor(AnchorLineTop))
                    anchors.setMargin(AnchorLineTop, m_beginTopMarginHash.value(item) + offsetVector.y());

                if (anchors.instanceHasAnchor(AnchorLineLeft))
                    anchors.setMargin(AnchorLineLeft, m_beginLeftMarginHash.value(item) + offsetVector.x());

                if (anchors.instanceHasAnchor(AnchorLineBottom))
                    anchors.setMargin(AnchorLineBottom, m_beginBottomMarginHash.value(item) - offsetVector.y());

                if (anchors.instanceHasAnchor(AnchorLineRight))
                    anchors.setMargin(AnchorLineRight, m_beginRightMarginHash.value(item) - offsetVector.x());

                if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                    anchors.setMargin(AnchorLineHorizontalCenter, m_beginHorizontalCenterHash.value(item) + offsetVector.x());

                if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                    anchors.setMargin(AnchorLineVerticalCenter, m_beginVerticalCenterHash.value(item) + offsetVector.y());

                item->qmlItemNode().setPosition(positionInContainerSpace);
            } else {
                item->qmlItemNode().setPostionInBaseState(positionInContainerSpace);
            }
        }
    }
}

void MoveManipulator::clear()
{
    deleteSnapLines();
    m_beginItemRectInSceneSpaceHash.clear();
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

void MoveManipulator::reparentTo(FormEditorItem *newParent, ReparentFlag flag)
{
    deleteSnapLines();

    if (!newParent)
        return;

    if (!itemsCanReparented())
        return;

    qCInfo(moveManipulatorInfo()) << Q_FUNC_INFO << newParent->qmlItemNode();

    if (!newParent->qmlItemNode().modelNode().metaInfo().isLayoutable()
            && newParent->qmlItemNode().modelNode().hasParentProperty()) {
        ModelNode grandParent = newParent->qmlItemNode().modelNode().parentProperty().parentModelNode();
        if (grandParent.metaInfo().isLayoutable()
                && !NodeHints::fromModelNode(grandParent).isStackedContainer()
                && flag == DoNotEnforceReparent)
            newParent = m_view.data()->scene()->itemForQmlItemNode(QmlItemNode(grandParent));
    }

    QVector<ModelNode> nodeReparentVector;
    NodeAbstractProperty parentProperty;

    QmlItemNode parentItemNode(newParent->qmlItemNode());
    if (parentItemNode.isValid()) {
        if (parentItemNode.hasDefaultPropertyName())
            parentProperty = parentItemNode.defaultNodeAbstractProperty();
        else
            parentProperty = parentItemNode.nodeAbstractProperty("data");

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

void MoveManipulator::end()
{
    m_view->formEditorWidget()->graphicsView()->viewport()->unsetCursor();
    setDirectUpdateInNodeInstances(false);
    m_isActive = false;
    deleteSnapLines();
    clear();
}



void MoveManipulator::end(Snapper::Snapping useSnapping)
{
    if (useSnapping == Snapper::UseSnappingAndAnchoring) {
        foreach (FormEditorItem *formEditorItem, m_itemList)
            m_snapper.adjustAnchoringOfItem(formEditorItem);
    }

    end();
}

void MoveManipulator::moveBy(double deltaX, double deltaY)
{
    foreach (FormEditorItem* item, m_itemList) {
        if (!item || !item->qmlItemNode().isValid())
            continue;

        QmlAnchors anchors(item->qmlItemNode().anchors());

        if (anchors.instanceHasAnchor(AnchorLineTop))
            anchors.setMargin(AnchorLineTop, anchors.instanceMargin(AnchorLineTop) + deltaY);

        if (anchors.instanceHasAnchor(AnchorLineLeft))
            anchors.setMargin(AnchorLineLeft, anchors.instanceMargin(AnchorLineLeft) + deltaX);

        if (anchors.instanceHasAnchor(AnchorLineBottom))
            anchors.setMargin(AnchorLineBottom, anchors.instanceMargin(AnchorLineBottom) - deltaY);

        if (anchors.instanceHasAnchor(AnchorLineRight))
            anchors.setMargin(AnchorLineRight, anchors.instanceMargin(AnchorLineRight) - deltaX);

        if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
            anchors.setMargin(AnchorLineHorizontalCenter, anchors.instanceMargin(AnchorLineHorizontalCenter) + deltaX);

        if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
            anchors.setMargin(AnchorLineVerticalCenter, anchors.instanceMargin(AnchorLineVerticalCenter) + deltaY);

        item->qmlItemNode().setPosition(QPointF(item->qmlItemNode().instanceValue("x").toDouble() + deltaX,
                                                  item->qmlItemNode().instanceValue("y").toDouble() + deltaY));
    }
}

void MoveManipulator::beginRewriterTransaction()
{
    m_rewriterTransaction = m_view->beginRewriterTransaction(QByteArrayLiteral("MoveManipulator::beginRewriterTransaction"));
    m_rewriterTransaction.ignoreSemanticChecks();
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

}
