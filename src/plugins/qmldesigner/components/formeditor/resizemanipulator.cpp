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

#include "resizemanipulator.h"

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "qmlanchors.h"
#include <QtDebug>
#include <model.h>
#include "mathutils.h"

#include <limits>

namespace QmlDesigner {

ResizeManipulator::ResizeManipulator(LayerItem *layerItem, FormEditorView *view)
    : m_view(view),
    m_beginTopMargin(0.0),
    m_beginLeftMargin(0.0),
    m_beginRightMargin(0.0),
    m_beginBottomMargin(0.0),
    m_layerItem(layerItem),
    m_resizeHandle(0)
{
}

ResizeManipulator::~ResizeManipulator()
{
    deleteSnapLines();
}

void ResizeManipulator::setHandle(ResizeHandleItem *resizeHandle)
{
    Q_ASSERT(resizeHandle);
    m_resizeHandle = resizeHandle;
    m_resizeController = resizeHandle->resizeController();
    m_snapper.setContainerFormEditorItem(m_resizeController.formEditorItem()->parentItem());
    m_snapper.setTransformtionSpaceFormEditorItem(m_resizeController.formEditorItem());
    Q_ASSERT(m_resizeController.isValid());
}

void ResizeManipulator::removeHandle()
{
    m_resizeController = ResizeController();
    m_resizeHandle = 0;
}

void ResizeManipulator::begin(const QPointF &/*beginPoint*/)
{
    if (m_resizeController.isValid()) {
        m_beginBoundingRect = m_resizeController.formEditorItem()->qmlItemNode().instanceBoundingRect();
        m_beginToSceneTransform = m_resizeController.formEditorItem()->qmlItemNode().instanceSceneTransform();
        m_beginFromSceneTransform = m_beginToSceneTransform.inverted();
        m_beginToParentTransform = m_resizeController.formEditorItem()->qmlItemNode().instanceTransform();
        m_rewriterTransaction = m_view->beginRewriterTransaction();
        m_snapper.updateSnappingLines(m_resizeController.formEditorItem());
        m_beginBottomRightPoint = m_beginToParentTransform.map(m_resizeController.formEditorItem()->qmlItemNode().instanceBoundingRect().bottomRight());

        QmlAnchors anchors(m_resizeController.formEditorItem()->qmlItemNode().anchors());
        m_beginTopMargin = anchors.instanceMargin(AnchorLine::Top);
        m_beginLeftMargin = anchors.instanceMargin(AnchorLine::Left);
        m_beginRightMargin = anchors.instanceMargin(AnchorLine::Right);
        m_beginBottomMargin = anchors.instanceMargin(AnchorLine::Bottom);
    }
}

//static QSizeF mapSizeToParent(const QSizeF &size, QGraphicsItem *item)
//{
//    QPointF sizeAsPoint(size.width(), size.height());
//    sizeAsPoint = item->mapToParent(sizeAsPoint);
//    return QSizeF(sizeAsPoint.x(), sizeAsPoint.y());
//}

void ResizeManipulator::update(const QPointF& updatePoint, Snapping useSnapping)
{
    const double minimumWidth = 15.0;
    const double minimumHeight = 15.0;

    deleteSnapLines();

    if (m_resizeController.isValid()) {

        FormEditorItem *formEditorItem = m_resizeController.formEditorItem();
        FormEditorItem *containerItem = m_snapper.containerFormEditorItem();

        if (!containerItem)
            return;

        QPointF updatePointInLocalSpace = m_beginFromSceneTransform.map(updatePoint);
        QmlAnchors anchors(formEditorItem->qmlItemNode().anchors());

        QRectF boundingRect(m_beginBoundingRect);
        if (m_resizeHandle->isBottomRightHandle()) {
            boundingRect.setBottomRight(updatePointInLocalSpace);

            if (useSnapping == UseSnapping) {
                double rightOffset = m_snapper.snapRightOffset(boundingRect);
                if (rightOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= rightOffset;

                double bottomOffset = m_snapper.snapBottomOffset(boundingRect);
                if (bottomOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= bottomOffset;
            }
            boundingRect.setBottomRight(updatePointInLocalSpace);

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                boundingRect.setLeft(boundingRect.left() - (updatePointInLocalSpace.x() - m_beginBoundingRect.right()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                boundingRect.setTop(boundingRect.top() - (updatePointInLocalSpace.y() - m_beginBoundingRect.bottom()));
            }

            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);
            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());


            if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
               anchors.setMargin(AnchorLine::Bottom,
                       m_beginBottomMargin - (m_beginToParentTransform.map(boundingRect.bottomRight())  - m_beginBottomRightPoint).y());
            }

            if (anchors.instanceHasAnchor(AnchorLine::Right)) {
               anchors.setMargin(AnchorLine::Right,
                       m_beginRightMargin - (m_beginToParentTransform.map(boundingRect.bottomRight()) - m_beginBottomRightPoint).x());
            }
        } else if (m_resizeHandle->isTopLeftHandle()) {
            boundingRect.setTopLeft(updatePointInLocalSpace);

            if (useSnapping == UseSnapping) {
                double leftOffset = m_snapper.snapLeftOffset(boundingRect);
                if (leftOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= leftOffset;

                double topOffset = m_snapper.snapTopOffset(boundingRect);
                if (topOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= topOffset;
            }
            boundingRect.setTopLeft(updatePointInLocalSpace);

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                boundingRect.setRight(boundingRect.right() - (updatePointInLocalSpace.x() - m_beginBoundingRect.left()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                boundingRect.setBottom(boundingRect.bottom() - (updatePointInLocalSpace.y() - m_beginBoundingRect.top()));
            }


            if (boundingRect.width() < minimumWidth)
                boundingRect.setLeft(boundingRect.left() - minimumWidth + boundingRect.width());
            if (boundingRect.height() < minimumHeight)
                boundingRect.setTop(boundingRect.top() - minimumHeight + boundingRect.height());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPositionWithBorder(m_beginToParentTransform.map(boundingRect.topLeft()));


            if (anchors.instanceHasAnchor(AnchorLine::Top)) {
               anchors.setMargin(AnchorLine::Top,
                       m_beginTopMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).y() + m_beginToParentTransform.map(boundingRect.topLeft()).y()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::Left)) {
               anchors.setMargin(AnchorLine::Left,
                       m_beginLeftMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).x() + m_beginToParentTransform.map(boundingRect.topLeft()).x()));
            }

        } else if (m_resizeHandle->isTopRightHandle()) {
            boundingRect.setTopRight(updatePointInLocalSpace);

            if (useSnapping == UseSnapping) {
                double rightOffset = m_snapper.snapRightOffset(boundingRect);
                if (rightOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= rightOffset;

                double topOffset = m_snapper.snapTopOffset(boundingRect);
                if (topOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= topOffset;
            }
            boundingRect.setTopRight(updatePointInLocalSpace);

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                boundingRect.setLeft(boundingRect.left() - (updatePointInLocalSpace.x() - m_beginBoundingRect.right()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                boundingRect.setBottom(boundingRect.bottom() - (updatePointInLocalSpace.y() - m_beginBoundingRect.top()));
            }

            if (boundingRect.height() < minimumHeight)
                boundingRect.setTop(boundingRect.top() - minimumHeight + boundingRect.height());
            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPositionWithBorder(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLine::Top)) {
               anchors.setMargin(AnchorLine::Top,
                       m_beginTopMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).y() + m_beginToParentTransform.map(boundingRect.topLeft()).y()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::Right)) {
               anchors.setMargin(AnchorLine::Right,
                       m_beginRightMargin - (m_beginToParentTransform.map(boundingRect.bottomRight()) - m_beginBottomRightPoint).x());
            }
        } else if (m_resizeHandle->isBottomLeftHandle()) {
            boundingRect.setBottomLeft(updatePointInLocalSpace);

            if (useSnapping == UseSnapping) {
                double leftOffset = m_snapper.snapLeftOffset(boundingRect);
                if (leftOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= leftOffset;

                double bottomOffset = m_snapper.snapBottomOffset(boundingRect);
                if (bottomOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= bottomOffset;
            }

            boundingRect.setBottomLeft(updatePointInLocalSpace);

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                boundingRect.setRight(boundingRect.right() - (updatePointInLocalSpace.x() - m_beginBoundingRect.left()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                boundingRect.setTop(boundingRect.top() - (updatePointInLocalSpace.y() - m_beginBoundingRect.bottom()));
            }

            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);
            if (boundingRect.width() < minimumWidth)
                boundingRect.setLeft(boundingRect.left() - minimumWidth + boundingRect.width());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPositionWithBorder(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLine::Left)) {
               anchors.setMargin(AnchorLine::Left,
                       m_beginLeftMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).x() + m_beginToParentTransform.map(boundingRect.topLeft()).x()));
            }

            if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
               anchors.setMargin(AnchorLine::Bottom,
                       m_beginBottomMargin - (m_beginToParentTransform.map(boundingRect.bottomRight())  - m_beginBottomRightPoint).y());
            }
        } else if (m_resizeHandle->isBottomHandle()) {
            boundingRect.setBottom(updatePointInLocalSpace.y());

            if (useSnapping == UseSnapping) {
                double bottomOffset = m_snapper.snapBottomOffset(boundingRect);
                if (bottomOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= bottomOffset;
            }

            boundingRect.setBottom(updatePointInLocalSpace.y());

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                boundingRect.setTop(boundingRect.top() - (updatePointInLocalSpace.y() - m_beginBoundingRect.bottom()));
            }

            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());

            if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
               anchors.setMargin(AnchorLine::Bottom,
                       m_beginBottomMargin - (m_beginToParentTransform.map(boundingRect.bottomRight())  - m_beginBottomRightPoint).y());
            }
        } else if (m_resizeHandle->isTopHandle()) {
            boundingRect.setTop(updatePointInLocalSpace.y());

            if (useSnapping == UseSnapping) {
                double topOffset = m_snapper.snapTopOffset(boundingRect);
                if (topOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= topOffset;
            }

            boundingRect.setTop(updatePointInLocalSpace.y());

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                boundingRect.setBottom(boundingRect.bottom() - (updatePointInLocalSpace.y() - m_beginBoundingRect.top()));
            }

            if (boundingRect.height() < minimumHeight)
                boundingRect.setTop(boundingRect.top() - minimumHeight + boundingRect.height());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPositionWithBorder(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLine::Top)) {
               anchors.setMargin(AnchorLine::Top,
                       m_beginTopMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).y() + m_beginToParentTransform.map(boundingRect.topLeft()).y()));
            }
        } else if (m_resizeHandle->isRightHandle()) {
            boundingRect.setRight(updatePointInLocalSpace.x());

            if (useSnapping == UseSnapping) {
                double rightOffset = m_snapper.snapRightOffset(boundingRect);
                if (rightOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= rightOffset;
            }

            boundingRect.setRight(updatePointInLocalSpace.x());

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                boundingRect.setLeft(boundingRect.left() - (updatePointInLocalSpace.x() - m_beginBoundingRect.right()));
            }

            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());


            if (anchors.instanceHasAnchor(AnchorLine::Right)) {
               anchors.setMargin(AnchorLine::Right,
                       m_beginRightMargin - (m_beginToParentTransform.map(boundingRect.bottomRight()) - m_beginBottomRightPoint).x());
            }
        } else if (m_resizeHandle->isLeftHandle()) {
            boundingRect.setLeft(updatePointInLocalSpace.x());

            if (useSnapping == UseSnapping) {
                double leftOffset = m_snapper.snapLeftOffset(boundingRect);
                if (leftOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= leftOffset;
            }

            boundingRect.setLeft(updatePointInLocalSpace.x());

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                boundingRect.setRight(boundingRect.right() - (updatePointInLocalSpace.x() - m_beginBoundingRect.left()));
            }

            if (boundingRect.width() < minimumWidth)
                boundingRect.setLeft(boundingRect.left() - minimumWidth + boundingRect.width());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPositionWithBorder(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLine::Left)) {
               anchors.setMargin(AnchorLine::Left,
                       m_beginLeftMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).x() + m_beginToParentTransform.map(boundingRect.topLeft()).x()));
            }
        }

        if (useSnapping == UseSnapping)
            m_graphicsLineList = m_snapper.generateSnappingLines(boundingRect,
                                                                 m_layerItem.data(),
                                                                 m_beginToSceneTransform);
    }
}

void ResizeManipulator::end()
{
    m_rewriterTransaction.commit();
    clear();
    removeHandle();
}

void ResizeManipulator::moveBy(double deltaX, double deltaY)
{
    if (resizeHandle()) {
        QmlItemNode qmlItemNode(m_resizeController.formEditorItem()->qmlItemNode());
        QmlAnchors anchors(qmlItemNode.anchors());

        if (m_resizeController.isLeftHandle(resizeHandle())
            || m_resizeController.isTopLeftHandle(resizeHandle())
            || m_resizeController.isBottomLeftHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("x", round((qmlItemNode.instanceValue("x").toDouble() + deltaX), 4));
            qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() - deltaX, 4));


            if (anchors.instanceHasAnchor(AnchorLine::Left)) {
               anchors.setMargin(AnchorLine::Left, anchors.instanceMargin(AnchorLine::Left) + deltaX);
            }

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() - (deltaX * 2), 4));
            }
        }

        if (m_resizeController.isRightHandle(resizeHandle())
            || m_resizeController.isTopRightHandle(resizeHandle())
            || m_resizeController.isBottomRightHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() + deltaX, 4));

            if (anchors.instanceHasAnchor(AnchorLine::Right)) {
               anchors.setMargin(AnchorLine::Right, round(anchors.instanceMargin(AnchorLine::Right) - deltaX, 4));
            }

            if (anchors.instanceHasAnchor(AnchorLine::HorizontalCenter)) {
                qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() + (deltaX * 2), 4));
            }
        }


        if (m_resizeController.isTopHandle(resizeHandle())
            || m_resizeController.isTopLeftHandle(resizeHandle())
            || m_resizeController.isTopRightHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("y", round(qmlItemNode.instanceValue("y").toDouble() + deltaY, 4));
            qmlItemNode.setVariantProperty("height", round(qmlItemNode.instanceValue("height").toDouble() - deltaY, 4));

            if (anchors.instanceHasAnchor(AnchorLine::Top)) {
               anchors.setMargin(AnchorLine::Top, anchors.instanceMargin(AnchorLine::Top) + deltaY);
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                qmlItemNode.setVariantProperty("height", round(qmlItemNode.instanceValue("height").toDouble() - (deltaY * 2), 4));
            }
        }

        if (m_resizeController.isBottomHandle(resizeHandle())
            || m_resizeController.isBottomLeftHandle(resizeHandle())
            || m_resizeController.isBottomRightHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("height",  round(qmlItemNode.instanceValue("height").toDouble() + deltaY, 4));

            if (anchors.instanceHasAnchor(AnchorLine::Bottom)) {
               anchors.setMargin(AnchorLine::Bottom, anchors.instanceMargin(AnchorLine::Bottom) - deltaY);
            }

            if (anchors.instanceHasAnchor(AnchorLine::VerticalCenter)) {
                qmlItemNode.setVariantProperty("height",  round(qmlItemNode.instanceValue("height").toDouble() + (deltaY * 2), 4));
            }
        }

    }
}

bool ResizeManipulator::isInvalidSize(const QSizeF & size)
{
    if (size.width() < 15 || size.height() < 15)
        return true;

    return false;
}

void ResizeManipulator::deleteSnapLines()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_graphicsLineList)
            m_layerItem->scene()->removeItem(item);
    }

    m_graphicsLineList.clear();
    m_view->scene()->update();
}

ResizeHandleItem *ResizeManipulator::resizeHandle()
{
    return m_resizeHandle;
}

void ResizeManipulator::clear()
{
    m_rewriterTransaction.commit();

    deleteSnapLines();
    m_beginBoundingRect = QRectF();
    m_beginFromSceneTransform = QTransform();
    m_beginToSceneTransform = QTransform();
    m_beginToParentTransform = QTransform();
    m_beginTopMargin = 0.0;
    m_beginLeftMargin = 0.0;
    m_beginRightMargin = 0.0;
    m_beginBottomMargin = 0.0;
    removeHandle();
}

}
