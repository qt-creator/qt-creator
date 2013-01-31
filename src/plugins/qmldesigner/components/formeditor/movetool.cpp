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

#include "movetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"

#include "resizehandleitem.h"

#include "nodemetainfo.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>

namespace QmlDesigner {

MoveTool::MoveTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeIndicator(editorView->scene()->manipulatorLayerItem())
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

    AbstractFormEditorTool::clear();
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

        FormEditorItem *containerItem = containerFormEditorItem(itemList, m_movingItems);
        if (containerItem
                && view()->currentState().isBaseState()) {
            if (containerItem != m_movingItems.first()->parentItem()
                    && event->modifiers().testFlag(Qt::ShiftModifier)) {
                m_moveManipulator.reparentTo(containerItem);
            }
        }

        bool shouldSnapping = view()->formEditorWidget()->snappingAction()->isChecked();
        bool shouldSnappingAndAnchoring = view()->formEditorWidget()->snappingAndAnchoringAction()->isChecked();

        MoveManipulator::Snapping useSnapping = MoveManipulator::NoSnapping;
        if (event->modifiers().testFlag(Qt::ControlModifier) != (shouldSnapping || shouldSnappingAndAnchoring)) {
            if (shouldSnappingAndAnchoring)
                useSnapping = MoveManipulator::UseSnappingAndAnchoring;
            else
                useSnapping = MoveManipulator::UseSnapping;
        }

        m_moveManipulator.update(event->scenePos(), useSnapping);
    }
}

void MoveTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * /*event*/)
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

    if (!topSelectedItemIsMovable(itemList)) {
        view()->changeToSelectionTool();
        return;
    }
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
        m_moveManipulator.beginRewriterTransaction();
        m_moveManipulator.clear();
//        m_selectionIndicator.show();
        m_resizeIndicator.show();
    }
}

void  MoveTool::dragLeaveEvent(QGraphicsSceneDragDropEvent * /*event*/)
{

}

void  MoveTool::dragMoveEvent(QGraphicsSceneDragDropEvent * /*event*/)
{

}

void MoveTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    if (m_moveManipulator.isActive()) {
        if (m_movingItems.isEmpty())
            return;

        QLineF moveVector(event->scenePos(), m_moveManipulator.beginPoint());
        if (moveVector.length() < QApplication::startDragDistance())
        {
            QPointF beginPoint(m_moveManipulator.beginPoint());

            m_moveManipulator.end(beginPoint);

            //        m_selectionIndicator.show();
            m_resizeIndicator.show();
            m_movingItems.clear();

            view()->changeToSelectionTool(event);
        } else {
            m_moveManipulator.end(event->scenePos());

            m_selectionIndicator.show();
            m_resizeIndicator.show();
            m_movingItems.clear();
        }
    }

    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
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
    updateMoveManipulator();
}

void MoveTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  MoveTool::instancesParentChanged(const QList<FormEditorItem *> &itemList)
{
    m_moveManipulator.synchronizeInstanceParent(itemList);
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

static bool isNotAncestorOfItemInList(FormEditorItem *formEditorItem, const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (item
            && item->qmlItemNode().isValid()
            && item->qmlItemNode().isAncestorOf(formEditorItem->qmlItemNode()))
            return false;
    }

    return true;
}

FormEditorItem* MoveTool::containerFormEditorItem(const QList<QGraphicsItem*> &itemUnderMouseList,
                                        const QList<FormEditorItem*> &selectedItemList)
{
    Q_ASSERT(!selectedItemList.isEmpty());

    foreach (QGraphicsItem* item, itemUnderMouseList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
           && !selectedItemList.contains(formEditorItem)
           && isNotAncestorOfItemInList(formEditorItem, selectedItemList))
                return formEditorItem;

    }

    return 0;
}

QList<FormEditorItem*> movalbeItems(const QList<FormEditorItem*> &itemList)
{
    QList<FormEditorItem*> filteredItemList(itemList);

    QMutableListIterator<FormEditorItem*> listIterator(filteredItemList);
    while (listIterator.hasNext()) {
        FormEditorItem *item = listIterator.next();
        if (!item->qmlItemNode().isValid() || !item->qmlItemNode().instanceIsMovable() || item->qmlItemNode().instanceIsInPositioner())
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
    m_selectionIndicator.updateItems(itemList);
    m_resizeIndicator.updateItems(itemList);
}

}
