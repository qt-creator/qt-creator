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

#include "movetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditorgraphicsview.h"

#include "resizehandleitem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>

namespace QmlDesigner {

MoveTool::MoveTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeIndicator(editorView->scene()->manipulatorLayerItem()),
    m_anchorIndicator(editorView->scene()->manipulatorLayerItem()),
    m_bindingIndicator(editorView->scene()->manipulatorLayerItem()),
    m_contentNotEditableIndicator(editorView->scene()->manipulatorLayerItem())
{
    m_selectionIndicator.setCursor(Qt::SizeAllCursor);
}

MoveTool::~MoveTool()
{

}

void MoveTool::clear()
{
    m_moveManipulator.clear();
    m_movingItems.clear();
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_anchorIndicator.clear();
    m_bindingIndicator.clear();
    m_contentNotEditableIndicator.clear();

    AbstractFormEditorTool::clear();
    view()->formEditorWidget()->graphicsView()->viewport()->unsetCursor();
}

void MoveTool::start()
{
    view()->formEditorWidget()->graphicsView()->viewport()->setCursor(Qt::SizeAllCursor);
}

void MoveTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (itemList.isEmpty())
            return;
        m_movingItems = movingItems(items());
        if (m_movingItems.isEmpty())
            return;

        m_moveManipulator.setItems(m_movingItems);
        m_moveManipulator.begin(event->scenePos());
    }

    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void MoveTool::mouseMoveEvent(const QList<QGraphicsItem*> &itemList,
                                           QGraphicsSceneMouseEvent *event)
{
    if (m_moveManipulator.isActive()) {
        if (m_movingItems.isEmpty())
            return;

        //    m_selectionIndicator.hide();
        m_resizeIndicator.hide();
        m_anchorIndicator.hide();
        m_bindingIndicator.hide();

        FormEditorItem *containerItem = containerFormEditorItem(itemList, m_movingItems);
        if (containerItem && view()->currentState().isBaseState()) {
            if (containerItem != m_movingItems.first()->parentItem()
                    && event->modifiers().testFlag(Qt::ShiftModifier)) {

                FormEditorItem *movingItem = m_movingItems.first();

                if (m_movingItems.count() > 1
                        || (movingItem->qmlItemNode().canBereparentedTo(containerItem->qmlItemNode())))
                        m_moveManipulator.reparentTo(containerItem, MoveManipulator::EnforceReparent);
            }
        }

        m_moveManipulator.update(event->scenePos(), generateUseSnapping(event->modifiers()));
    }
}

void MoveTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * event)
{
    if (itemList.isEmpty()) {
        view()->changeToSelectionTool();
        return;
    }

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle) {
        view()->changeToResizeTool();
        return;
    }

    if (view()->hasSingleSelectedModelNode() && selectedItemCursorInMovableArea(event->scenePos()))
        return;

    if (!topSelectedItemIsMovable(itemList)) {
        view()->changeToSelectionTool();
        return;
    }

    if (view()->hasSingleSelectedModelNode()) {
        view()->changeToSelectionTool();
        return;
    }

    if (event->modifiers().testFlag(Qt::ShiftModifier)
            || event->modifiers().testFlag(Qt::ControlModifier) ) {
        view()->changeToSelectionTool();
        return;
    }


    m_contentNotEditableIndicator.setItems(toFormEditorItemList(itemList));
}

void MoveTool::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_AltGr:
            event->setAccepted(false);
            return;
    }

    double moveStep = 1.0;

    if (event->modifiers().testFlag(Qt::ShiftModifier))
        moveStep = 10.0;

    if (!event->isAutoRepeat()) {
        QList<FormEditorItem*> movableItems(movingItems(items()));
        if (movableItems.isEmpty())
            return;

        m_moveManipulator.setItems(movableItems);
//        m_selectionIndicator.hide();
        m_resizeIndicator.hide();
        m_anchorIndicator.hide();
        m_bindingIndicator.hide();
        m_moveManipulator.beginRewriterTransaction();
    }

    switch (event->key()) {
        case Qt::Key_Left: m_moveManipulator.moveBy(-moveStep, 0.0); break;
        case Qt::Key_Right: m_moveManipulator.moveBy(moveStep, 0.0); break;
        case Qt::Key_Up: m_moveManipulator.moveBy(0.0, -moveStep); break;
        case Qt::Key_Down: m_moveManipulator.moveBy(0.0, moveStep); break;
    }

    if (event->key() == Qt::Key_Escape && !m_movingItems.isEmpty()) {
       event->accept();
       view()->changeToSelectionTool();
    }
}

void MoveTool::keyReleaseEvent(QKeyEvent *keyEvent)
{
    switch (keyEvent->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_AltGr:
            keyEvent->setAccepted(false);
            return;
    }

    if (!keyEvent->isAutoRepeat()) {
        m_moveManipulator.clear();
//        m_selectionIndicator.show();
        m_resizeIndicator.show();
        m_anchorIndicator.show();
        m_bindingIndicator.show();
    }
}

void  MoveTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void  MoveTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void MoveTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    if (m_moveManipulator.isActive()) {
        if (m_movingItems.isEmpty())
            return;

        m_moveManipulator.end(generateUseSnapping(event->modifiers()));

        m_selectionIndicator.show();
        m_resizeIndicator.show();
        m_anchorIndicator.show();
        m_bindingIndicator.show();
        m_movingItems.clear();
    }

    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);

    view()->changeToSelectionTool();
}

void MoveTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void MoveTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    foreach (FormEditorItem* removedItem, removedItemList)
        m_movingItems.removeOne(removedItem);
}

void MoveTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.setItems(movingItems(itemList));
    m_resizeIndicator.setItems(itemList);
    m_anchorIndicator.setItems(itemList);
    m_bindingIndicator.setItems(itemList);
    updateMoveManipulator();
}

void MoveTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  MoveTool::instancesParentChanged(const QList<FormEditorItem *> &itemList)
{
    m_moveManipulator.synchronizeInstanceParent(itemList);
}

void MoveTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

bool MoveTool::haveSameParent(const QList<FormEditorItem*> &itemList)
{
    if (itemList.isEmpty())
        return false;

    QGraphicsItem *firstParent = itemList.first()->parentItem();
    foreach (FormEditorItem* item, itemList)
    {
        if (firstParent != item->parentItem())
            return false;
    }

    return true;
}

bool MoveTool::isAncestorOfAllItems(FormEditorItem* maybeAncestorItem,
                                    const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem* item, itemList)
    {
        if (!maybeAncestorItem->isAncestorOf(item) && item != maybeAncestorItem)
            return false;
    }

    return true;
}


FormEditorItem* MoveTool::ancestorIfOtherItemsAreChild(const QList<FormEditorItem*> &itemList)
{
    if (itemList.isEmpty())
        return 0;


    foreach (FormEditorItem* item, itemList)
    {
        if (isAncestorOfAllItems(item, itemList))
            return item;
    }

    return 0;
}

void MoveTool::updateMoveManipulator()
{
    if (m_moveManipulator.isActive())
        return;
}

void MoveTool::beginWithPoint(const QPointF &beginPoint)
{
    m_movingItems = movingItems(items());
    if (m_movingItems.isEmpty())
        return;

    m_moveManipulator.setItems(m_movingItems);
    m_moveManipulator.begin(beginPoint);
}



QList<FormEditorItem*> movalbeItems(const QList<FormEditorItem*> &itemList)
{
    QList<FormEditorItem*> filteredItemList(itemList);

    QMutableListIterator<FormEditorItem*> listIterator(filteredItemList);
    while (listIterator.hasNext()) {
        FormEditorItem *item = listIterator.next();
        if (!item->qmlItemNode().isValid()
                || !item->qmlItemNode().instanceIsMovable()
                || !item->qmlItemNode().modelIsMovable()
                || item->qmlItemNode().instanceIsInLayoutable())
            listIterator.remove();
    }

    return filteredItemList;
}

QList<FormEditorItem*> MoveTool::movingItems(const QList<FormEditorItem*> &selectedItemList)
{
    QList<FormEditorItem*> filteredItemList = movalbeItems(selectedItemList);

    FormEditorItem* ancestorItem = ancestorIfOtherItemsAreChild(filteredItemList);

    if (ancestorItem != 0 && ancestorItem->qmlItemNode().isRootNode()) {
//        view()->changeToSelectionTool();
        return QList<FormEditorItem*>();
    }


    if (ancestorItem != 0 && ancestorItem->parentItem() != 0)  {
        QList<FormEditorItem*> ancestorItemList;
        ancestorItemList.append(ancestorItem);
        return ancestorItemList;
    }

    if (!haveSameParent(filteredItemList)) {
//        view()->changeToSelectionTool();
        return QList<FormEditorItem*>();
    }

    return filteredItemList;
}

void MoveTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    const QList<FormEditorItem*> selectedItemList = filterSelectedModelNodes(itemList);

    m_selectionIndicator.updateItems(selectedItemList);
    m_resizeIndicator.updateItems(selectedItemList);
    m_anchorIndicator.updateItems(selectedItemList);
    m_bindingIndicator.updateItems(selectedItemList);
    m_contentNotEditableIndicator.updateItems(selectedItemList);
}

void MoveTool::focusLost()
{
}

}
