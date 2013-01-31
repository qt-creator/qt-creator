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

#include "snappinglinecreator.h"

#include <QGraphicsItem>
#include "onedimensionalcluster.h"
#include "formeditoritem.h"
#include "formeditorview.h"

namespace QmlDesigner {

SnappingLineCreator::SnappingLineCreator(FormEditorItem *formEditorItem)
    : m_formEditorItem(formEditorItem),
    m_topOffset(formEditorItem->formEditorView()->spacing()),
    m_bottomOffset(formEditorItem->formEditorView()->spacing()),
    m_leftOffset(formEditorItem->formEditorView()->spacing()),
    m_rightOffset(formEditorItem->formEditorView()->spacing()),
    m_topMargin(formEditorItem->formEditorView()->margins()),
    m_bottomMargin(formEditorItem->formEditorView()->margins()),
    m_leftMargin(formEditorItem->formEditorView()->margins()),
    m_rightMargin(formEditorItem->formEditorView()->margins())
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
    m_topLineMap.insert(rectInSceneSpace.top(), qMakePair(rectInSceneSpace, item));
    m_bottomLineMap.insert(rectInSceneSpace.bottom(), qMakePair(rectInSceneSpace, item));
    m_leftLineMap.insert(rectInSceneSpace.left(), qMakePair(rectInSceneSpace, item));
    m_rightLineMap.insert(rectInSceneSpace.right(), qMakePair(rectInSceneSpace, item));
    QPointF centerPoint(rectInSceneSpace.center());
    m_horizontalCenterLineMap.insert(centerPoint.y(), qMakePair(rectInSceneSpace, item));
    m_verticalCenterLineMap.insert(centerPoint.x(), qMakePair(rectInSceneSpace, item));
}

void SnappingLineCreator::addOffsets(const QRectF &rectInSceneSpace, FormEditorItem *item)
{
    m_topOffsetMap.insert(rectInSceneSpace.top() - m_topOffset, qMakePair(rectInSceneSpace, item));
    m_bottomOffsetMap.insert(rectInSceneSpace.bottom() + m_bottomOffset, qMakePair(rectInSceneSpace, item));
    m_leftOffsetMap.insert(rectInSceneSpace.left() - m_leftOffset, qMakePair(rectInSceneSpace, item));
    m_rightOffsetMap.insert(rectInSceneSpace.right() + m_rightOffset, qMakePair(rectInSceneSpace, item));
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
        containerBoundingRectInTransformationSpace.adjust(m_leftMargin, m_topMargin, -m_rightMargin, -m_rightMargin);
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

void SnappingLineCreator::setMargins(double margin)
{
    m_topMargin = margin;
    m_bottomMargin = margin;
    m_leftMargin = margin;
    m_rightMargin = margin;
}

void SnappingLineCreator::setSpacing(double spacing)
{
    m_topOffset = spacing;
    m_bottomOffset = spacing;
    m_leftOffset = spacing;
    m_rightOffset = spacing;
}

void SnappingLineCreator::update(const QList<FormEditorItem*> &exceptionList,
                                 FormEditorItem *transformationSpaceItem)
{
    clearLines();
    setMargins(m_formEditorItem->formEditorView()->margins());
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

}
