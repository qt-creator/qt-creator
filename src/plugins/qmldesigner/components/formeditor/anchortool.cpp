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

#include "anchortool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include <qmlitemnode.h>
#include <qmlanchors.h>
#include "anchorlinehandleitem.h"

#include <QGraphicsSceneMouseEvent>

#include <QDebug>

namespace QmlDesigner {

AnchorTool::AnchorTool(FormEditorView* editorView)
   : AbstractFormEditorTool(editorView),
     m_anchorLineIndicator(editorView->scene()->manipulatorLayerItem()),
     m_anchorIndicator(editorView->scene()->manipulatorLayerItem()),
     m_anchorManipulator(editorView),
     m_lastAnchorLineHandleItem(0)
{
    m_hoverTimeLine.setDuration(200);
    connect(&m_hoverTimeLine, SIGNAL(finished()), SLOT(checkIfStillHovering()));
}

AnchorTool::~AnchorTool()
{

}

void AnchorTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                     QGraphicsSceneMouseEvent *)
{
    AnchorLineHandleItem *anchorLineHandleItem = topAnchorLineHandleItem(itemList);
    if (anchorLineHandleItem) {
        m_anchorManipulator.begin(anchorLineHandleItem->anchorLineController().formEditorItem(),
                                  anchorLineHandleItem->anchorLineType());
    }

    m_anchorLineIndicator.clear();

}

bool areAchorable(FormEditorItem *firstItem, FormEditorItem *secondItem)
{
    return firstItem->qmlItemNode().anchors().canAnchor(secondItem->qmlItemNode());
}

void AnchorTool::mouseMoveEvent(const QList<QGraphicsItem*> &itemList,
                    QGraphicsSceneMouseEvent *event)
{
    if (m_anchorManipulator.isActive()) {
        FormEditorItem *targetItem = 0;
        AnchorLineHandleItem *anchorLineHandleItem = topAnchorLineHandleItem(itemList);
        if (anchorLineHandleItem && areAchorable(m_anchorManipulator.beginFormEditorItem(), anchorLineHandleItem->anchorLineController().formEditorItem())) {
            targetItem = anchorLineHandleItem->anchorLineController().formEditorItem();
        } else {
            FormEditorItem *topFormEditItem = topFormEditorItemWithRootItem(itemList);
            if (topFormEditItem && areAchorable(m_anchorManipulator.beginFormEditorItem(), topFormEditItem)) {
                targetItem = topFormEditItem;
            } else {
                m_anchorLineIndicator.hide();
                m_anchorIndicator.updateTargetPoint(m_anchorManipulator.beginFormEditorItem(), m_anchorManipulator.beginAnchorLine(), event->scenePos());
            }
        }

        if (targetItem) {
            targetItem->qmlItemNode().selectNode();
            m_anchorLineIndicator.setItem(targetItem);
            m_anchorLineIndicator.show(m_anchorManipulator.beginFormEditorItem()->qmlItemNode().anchors().possibleAnchorLines(m_anchorManipulator.beginAnchorLine(), targetItem->qmlItemNode()));
            m_anchorIndicator.updateTargetPoint(m_anchorManipulator.beginFormEditorItem(), m_anchorManipulator.beginAnchorLine(), event->scenePos());
            targetItem->qmlItemNode().selectNode();
        }
    }

}

void AnchorTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                       QGraphicsSceneMouseEvent *)
{
    if (m_anchorManipulator.isActive()) {
        AnchorLineHandleItem *anchorLineHandleItem = topAnchorLineHandleItem(itemList);
        if (anchorLineHandleItem) {
            m_anchorManipulator.addAnchor(anchorLineHandleItem->anchorLineController().formEditorItem(),
                                    anchorLineHandleItem->anchorLineType());
        } else {
            m_anchorManipulator.removeAnchor();
        }


    }

    FormEditorItem *topFormEditItem = topFormEditorItem(itemList);
    if (topFormEditItem)
        topFormEditItem->qmlItemNode().selectNode();

    m_anchorManipulator.clear();
    m_anchorLineIndicator.clear();
}

void AnchorTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/,
                           QGraphicsSceneMouseEvent * /*event*/)
{
}

void AnchorTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                    QGraphicsSceneMouseEvent *event)
{
    m_anchorLineIndicator.clearHighlight();
    m_anchorIndicator.clearHighlight();
    m_lastMousePosition = event->scenePos();
    FormEditorItem *topFormEditItem = 0;
    AnchorLineHandleItem *anchorLineHandleItem = topAnchorLineHandleItem(itemList);

    if (anchorLineHandleItem) {
        anchorLineHandleItem->setHiglighted(true);
        m_anchorIndicator.highlight(anchorLineHandleItem->anchorLineController().formEditorItem(),
                                    anchorLineHandleItem->anchorLineType());
        topFormEditItem = anchorLineHandleItem->anchorLineController().formEditorItem();
        if (m_hoverTimeLine.state() == QTimeLine::NotRunning) {
            m_lastAnchorLineHandleItem = anchorLineHandleItem;
            m_hoverTimeLine.start();
        }
    } else {
        topFormEditItem = topFormEditorItem(itemList);
    }

    if (topFormEditItem) {
        m_anchorLineIndicator.setItem(topFormEditItem);
        m_anchorLineIndicator.show(AnchorLine::AllMask);
        topFormEditItem->qmlItemNode().selectNode();
    } else {

        m_anchorLineIndicator.clear();
    }
}

void AnchorTool::checkIfStillHovering()
{
    AnchorLineHandleItem *anchorLineHandleItem = topAnchorLineHandleItem(scene()->items(m_lastMousePosition));

    if (anchorLineHandleItem && anchorLineHandleItem == m_lastAnchorLineHandleItem) {
        FormEditorItem *sourceFormEditItem = anchorLineHandleItem->anchorLineController().formEditorItem();
        QmlAnchors anchors(sourceFormEditItem->qmlItemNode().anchors());
        if (anchors.instanceHasAnchor(anchorLineHandleItem->anchorLine())) {
            QmlItemNode targetNode(anchors.instanceAnchor(anchorLineHandleItem->anchorLine()).qmlItemNode());
            FormEditorItem *targetFormEditorItem = scene()->itemForQmlItemNode(targetNode);
            targetFormEditorItem->showAttention();
        }
    }
}

void AnchorTool::keyPressEvent(QKeyEvent *)
{
}

void AnchorTool::keyReleaseEvent(QKeyEvent *)
{
}

void AnchorTool::dragLeaveEvent(QGraphicsSceneDragDropEvent *)
{

}

void AnchorTool::dragMoveEvent(QGraphicsSceneDragDropEvent *)
{

}

void AnchorTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItems)
{
    QList<FormEditorItem*> newItemList = items().toSet().subtract(removedItems.toSet()).toList();
    setItems(newItemList);
    m_anchorIndicator.setItems(newItemList);
    m_anchorLineIndicator.clear();
}

void AnchorTool::selectedItemsChanged(const QList<FormEditorItem*> &/*itemList*/)
{
    m_anchorIndicator.setItems(view()->scene()->allFormEditorItems());
    m_anchorIndicator.show();
}

void AnchorTool::clear()
{
    m_anchorLineIndicator.clear();
    m_anchorIndicator.clear();
}

void AnchorTool::formEditorItemsChanged(const QList<FormEditorItem*> &)
{
    m_anchorLineIndicator.updateItems(view()->scene()->allFormEditorItems());
    m_anchorIndicator.updateItems(view()->scene()->allFormEditorItems());
}

void AnchorTool::instancesCompleted(const QList<FormEditorItem*> &/*itemList*/)
{
}

void AnchorTool::instancesParentChanged(const QList<FormEditorItem *> &/*itemList*/)
{
}

AnchorLineHandleItem* AnchorTool::topAnchorLineHandleItem(const QList<QGraphicsItem*> & itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        AnchorLineHandleItem *anchorLineItem = AnchorLineHandleItem::fromGraphicsItem(item);
        if (anchorLineItem)
            return anchorLineItem;
    }

    return 0;
}
}
