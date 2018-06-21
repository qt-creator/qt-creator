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

#include "snappinglinecreator.h"

#include "formeditoritem.h"
#include "formeditorview.h"

#include <QtDebug>

namespace QmlDesigner {

SnappingLineCreator::SnappingLineCreator(FormEditorItem *formEditorItem)
    : m_formEditorItem(formEditorItem),
    m_topOffset(formEditorItem->formEditorView()->spacing()),
    m_bottomOffset(formEditorItem->formEditorView()->spacing()),
    m_leftOffset(formEditorItem->formEditorView()->spacing()),
    m_rightOffset(formEditorItem->formEditorView()->spacing()),
    m_topPadding(formEditorItem->formEditorView()->containerPadding()),
    m_bottomPadding(formEditorItem->formEditorView()->containerPadding()),
    m_leftPadding(formEditorItem->formEditorView()->containerPadding()),
    m_rightPadding(formEditorItem->formEditorView()->containerPadding())
{
    Q_ASSERT(m_formEditorItem);
}

void SnappingLineCreator::clearLines()
{
    m_topLineMap.clear();
    m_bottomLineMap.clear();
    m_leftLineMap.clear();
    m_rightLineMap.clear();
    m_horizontalCenterLineMap.clear();
    m_verticalCenterLineMap.clear();

    m_topOffsetMap.clear();
    m_bottomOffsetMap.clear();
    m_leftOffsetMap.clear();
    m_rightOffsetMap.clear();

}

void SnappingLineCreator::addLines(const QRectF &rectInSceneSpace, FormEditorItem *item)
{
    const QPair<QRectF, FormEditorItem*> rectInSceneSpaceItemPair = {rectInSceneSpace, item};
    m_topLineMap.insert(rectInSceneSpace.top(), rectInSceneSpaceItemPair);
    m_bottomLineMap.insert(rectInSceneSpace.bottom(), rectInSceneSpaceItemPair);
    m_leftLineMap.insert(rectInSceneSpace.left(), rectInSceneSpaceItemPair);
    m_rightLineMap.insert(rectInSceneSpace.right(), rectInSceneSpaceItemPair);
    const QPointF centerPoint(rectInSceneSpace.center());
    m_horizontalCenterLineMap.insert(centerPoint.y(), rectInSceneSpaceItemPair);
    m_verticalCenterLineMap.insert(centerPoint.x(), rectInSceneSpaceItemPair);
}

void SnappingLineCreator::addOffsets(const QRectF &rectInSceneSpace, FormEditorItem *item)
{
    const QPair<QRectF, FormEditorItem*> rectInSceneSpaceItemPair = {rectInSceneSpace, item};
    m_topOffsetMap.insert(rectInSceneSpace.top() - m_topOffset, rectInSceneSpaceItemPair);
    m_bottomOffsetMap.insert(rectInSceneSpace.bottom() + m_bottomOffset, rectInSceneSpaceItemPair);
    m_leftOffsetMap.insert(rectInSceneSpace.left() - m_leftOffset, rectInSceneSpaceItemPair);
    m_rightOffsetMap.insert(rectInSceneSpace.right() + m_rightOffset, rectInSceneSpaceItemPair);
}

void SnappingLineCreator::generateLines(const QList<FormEditorItem*> &exceptionList,
                                        FormEditorItem *transformationSpaceItem)
{
    if (!m_formEditorItem->qmlItemNode().isValid())
        return;

    Q_ASSERT(transformationSpaceItem);
    {
        QRectF containerBoundingRectInTransformationSpace = m_formEditorItem->mapRectToItem(transformationSpaceItem,
                                                                                            m_formEditorItem->qmlItemNode().instanceBoundingRect());

        addLines(containerBoundingRectInTransformationSpace, m_formEditorItem);
        containerBoundingRectInTransformationSpace.adjust(m_leftPadding, m_topPadding, -m_rightPadding, -m_rightPadding);
        addLines(containerBoundingRectInTransformationSpace, m_formEditorItem);
    }

    foreach (FormEditorItem *item, m_formEditorItem->childFormEditorItems()) {

        if (!item || !item->qmlItemNode().isValid())
            continue;

        if (exceptionList.contains(item))
            continue;
        QRectF boundingRectInContainerSpace;

        boundingRectInContainerSpace = item->mapRectToItem(transformationSpaceItem, item->qmlItemNode().instanceBoundingRect());

        boundingRectInContainerSpace = boundingRectInContainerSpace.toRect(); // round to integer

        addLines(boundingRectInContainerSpace, item);
        addOffsets(boundingRectInContainerSpace, item);
    }
}

void SnappingLineCreator::setContainerPaddingByGloablPadding(double containerPadding)
{
    m_topPadding = containerPadding;
    m_bottomPadding = containerPadding;
    m_leftPadding = containerPadding;
    m_rightPadding = containerPadding;
}

void SnappingLineCreator::setContainerPaddingByContentItem(const QRectF &contentRectangle, const QRectF &itemBoundingRectangle)
{
    m_topPadding = contentRectangle.top() - itemBoundingRectangle.top();
    m_bottomPadding = itemBoundingRectangle.bottom() - contentRectangle.bottom();
    m_leftPadding = contentRectangle.left() - itemBoundingRectangle.left();
    m_rightPadding = itemBoundingRectangle.right() - contentRectangle.right();
}

void SnappingLineCreator::setSpacing(double spacing)
{
    m_topOffset = spacing;
    m_bottomOffset = spacing;
    m_leftOffset = spacing;
    m_rightOffset = spacing;
}

void SnappingLineCreator::update(const QList<FormEditorItem*> &exceptionList,
                                 FormEditorItem *transformationSpaceItem,
                                 FormEditorItem *containerFormEditorItem)
{
    clearLines();
    setContainerPaddingItem(containerFormEditorItem);
    setSpacing(m_formEditorItem->formEditorView()->spacing());
    generateLines(exceptionList, transformationSpaceItem);
}

SnapLineMap SnappingLineCreator::topLines() const
{
    return m_topLineMap;
}

SnapLineMap SnappingLineCreator::bottomLines() const
{
    return m_bottomLineMap;
}

SnapLineMap SnappingLineCreator::leftLines() const
{
    return m_leftLineMap;
}

SnapLineMap SnappingLineCreator::rightLines() const
{
    return m_rightLineMap;
}

SnapLineMap  SnappingLineCreator::horizontalCenterLines() const
{
    return m_horizontalCenterLineMap;
}

SnapLineMap  SnappingLineCreator::verticalCenterLines() const
{
    return m_verticalCenterLineMap;
}

SnapLineMap SnappingLineCreator::topOffsets() const
{
    return m_topOffsetMap;
}

SnapLineMap SnappingLineCreator::bottomOffsets() const
{
    return m_bottomOffsetMap;
}

SnapLineMap SnappingLineCreator::leftOffsets() const
{
    return m_leftOffsetMap;
}

SnapLineMap SnappingLineCreator::rightOffsets() const
{
    return m_rightOffsetMap;
}

void SnappingLineCreator::setContainerPaddingItem(FormEditorItem *transformationSpaceItem)
{
    QmlItemNode containerItemNode = transformationSpaceItem->qmlItemNode();
    QRectF contentRect = containerItemNode.instanceContentItemBoundingRect();

    if (contentRect.isValid())
        setContainerPaddingByContentItem(contentRect, containerItemNode.instanceBoundingRect());
    else
        setContainerPaddingByGloablPadding(m_formEditorItem->formEditorView()->containerPadding());
}

}
