/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "movetool.h"

#include "resizehandleitem.h"
#include "qdeclarativedesignview.h"

#include <QApplication>
#include <QWheelEvent>
#include <QAction>
#include <QMouseEvent>
#include <QtDebug>

namespace QmlViewer {

MoveTool::MoveTool(QDeclarativeDesignView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moving(false),
    m_moveManipulator(editorView),
    m_selectionIndicator(editorView->manipulatorLayer()),
    m_resizeIndicator(editorView->manipulatorLayer())
{

}


MoveTool::~MoveTool()
{

}

void MoveTool::clear()
{
    view()->clearHighlightBoundingRect();
    view()->setCursor(Qt::SizeAllCursor);
    m_moveManipulator.clear();
    m_movingItems.clear();
    m_resizeIndicator.clear();
    m_selectionIndicator.clear();
}

void MoveTool::mousePressEvent(QMouseEvent *event)
{
    QList<QGraphicsItem*> itemList = view()->selectableItems(event->pos());

    if (event->buttons() & Qt::LeftButton) {
        m_moving = true;

        if (itemList.isEmpty()) {
            view()->changeTool(Constants::SelectionToolMode);
            return;
        }

        m_movingItems = movingItems(items());
        if (m_movingItems.isEmpty())
            return;

        m_moveManipulator.setItems(m_movingItems);

        m_moveManipulator.begin(event->pos());
    } else if (event->buttons() & Qt::RightButton) {
        view()->changeTool(Constants::SelectionToolMode);
    }

}

void MoveTool::mouseMoveEvent(QMouseEvent *event)
{
    if (m_movingItems.isEmpty())
        return;

    if (event->buttons() & Qt::LeftButton) {

        m_selectionIndicator.hide();
        m_resizeIndicator.hide();

//        QGraphicsItem *containerItem = containerQGraphicsItem(itemList, m_movingItems);
//        if (containerItem
//            && view()->currentState().isBaseState()) {
//            if (containerItem != m_movingItems.first()->parentItem()
//                && event->modifiers().testFlag(Qt::ShiftModifier)) {
//                m_moveManipulator.reparentTo(containerItem);
//            }
//        }
//        bool shouldSnapping = view()->widget()->snappingAction()->isChecked();
//        bool shouldSnappingAndAnchoring = view()->widget()->snappingAndAnchoringAction()->isChecked();
//        MoveManipulator::Snapping useSnapping = MoveManipulator::NoSnapping;
//        if (event->modifiers().testFlag(Qt::ControlModifier) != (shouldSnapping || shouldSnappingAndAnchoring)) {
//            if (shouldSnappingAndAnchoring)
//                useSnapping = MoveManipulator::UseSnappingAndAnchoring;
//            else
//                useSnapping = MoveManipulator::UseSnapping;
//        }

        m_moveManipulator.update(event->pos(), MoveManipulator::NoSnapping);
    }
}

void MoveTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_movingItems.isEmpty())
        return;

    if (m_moving) {
        QLineF moveVector(event->pos(), m_moveManipulator.beginPoint());
        if (moveVector.length() < QApplication::startDragDistance())
        {
            QPointF beginPoint(m_moveManipulator.beginPoint());

            m_moveManipulator.end(beginPoint);

            m_selectionIndicator.show();
            m_resizeIndicator.show();
            m_movingItems.clear();
            view()->changeTool(Constants::SelectionToolMode,
                               Constants::UseCursorPos);
        } else {
            m_moveManipulator.end(event->pos());

            m_selectionIndicator.show();
            m_resizeIndicator.show();
            m_movingItems.clear();
        }
        qDebug() << "releasing";
        view()->changeTool(Constants::ResizeToolMode);
    }

    m_moving = false;

    qDebug() << "released";
}


void MoveTool::hoverMoveEvent(QMouseEvent *event)
{
    QList<QGraphicsItem*> itemList = view()->items(event->pos());

    if (itemList.isEmpty()) {
        view()->changeTool(Constants::SelectionToolMode);
        return;
    }

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle) {
        view()->changeTool(Constants::ResizeToolMode);
        return;
    }

    QList<QGraphicsItem*> selectableItemList = view()->selectableItems(event->pos());
    if (!topSelectedItemIsMovable(selectableItemList)) {
        view()->changeTool(Constants::SelectionToolMode);
        return;
    }
}

void MoveTool::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
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
        QList<QGraphicsItem*> movableItems(movingItems(items()));
        if (movableItems.isEmpty())
            return;

        m_moveManipulator.setItems(movableItems);
        m_selectionIndicator.hide();
        m_resizeIndicator.hide();
    }

    switch(event->key()) {
        case Qt::Key_Left: m_moveManipulator.moveBy(-moveStep, 0.0); break;
        case Qt::Key_Right: m_moveManipulator.moveBy(moveStep, 0.0); break;
        case Qt::Key_Up: m_moveManipulator.moveBy(0.0, -moveStep); break;
        case Qt::Key_Down: m_moveManipulator.moveBy(0.0, moveStep); break;
    }


}

void MoveTool::keyReleaseEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_AltGr:
            keyEvent->setAccepted(false);
            return;
    }

    if (!keyEvent->isAutoRepeat()) {
        m_moveManipulator.clear();
        m_selectionIndicator.show();
        m_resizeIndicator.show();
    }
}

void MoveTool::wheelEvent(QWheelEvent */*event*/)
{

}

void MoveTool::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{

}

void MoveTool::itemsAboutToRemoved(const QList<QGraphicsItem*> &removedItemList)
{
    foreach (QGraphicsItem* removedItem, removedItemList)
        m_movingItems.removeOne(removedItem);
}

void MoveTool::selectedItemsChanged(const QList<QGraphicsItem*> &itemList)
{
    m_selectionIndicator.setItems(toGraphicsObjectList(itemList));
    m_resizeIndicator.setItems(toGraphicsObjectList(itemList));
    updateMoveManipulator();
}

bool MoveTool::haveSameParent(const QList<QGraphicsItem*> &itemList)
{
    if (itemList.isEmpty())
        return false;

    QGraphicsItem *firstParent = itemList.first()->parentItem();
    foreach (QGraphicsItem* item, itemList)
    {
        if (firstParent != item->parentItem())
            return false;
    }

    return true;
}

bool MoveTool::isAncestorOfAllItems(QGraphicsItem* maybeAncestorItem,
                                    const QList<QGraphicsItem*> &itemList)
{
    foreach (QGraphicsItem* item, itemList)
    {
        if (!maybeAncestorItem->isAncestorOf(item) && item != maybeAncestorItem)
            return false;
    }

    return true;
}


QGraphicsItem* MoveTool::ancestorIfOtherItemsAreChild(const QList<QGraphicsItem*> &itemList)
{
    if (itemList.isEmpty())
        return 0;


    foreach (QGraphicsItem* item, itemList)
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

    m_moving = true;
    m_moveManipulator.setItems(m_movingItems);
    m_moveManipulator.begin(beginPoint);
}

//static bool isNotAncestorOfItemInList(QGraphicsItem *graphicsItem, const QList<QGraphicsItem*> &itemList)
//{
//    foreach (QGraphicsItem *item, itemList) {
//        if (item->qmlItemNode().isAncestorOf(graphicsItem->qmlItemNode()))
//            return false;
//    }

//    return true;
//}

//QGraphicsItem* MoveTool::containerQGraphicsItem(const QList<QGraphicsItem*> &itemUnderMouseList,
//                                        const QList<QGraphicsItem*> &selectedItemList)
//{
//    Q_ASSERT(!selectedItemList.isEmpty());

//    foreach (QGraphicsItem* item, itemUnderMouseList) {
//        QGraphicsItem *QGraphicsItem = QGraphicsItem::fromQGraphicsItem(item);
//        if (QGraphicsItem
//           && !selectedItemList.contains(QGraphicsItem)
//           && isNotAncestorOfItemInList(QGraphicsItem, selectedItemList))
//                return QGraphicsItem;

//    }

//    return 0;
//}


QList<QGraphicsItem*> MoveTool::movingItems(const QList<QGraphicsItem*> &selectedItemList)
{
    QGraphicsItem* ancestorItem = ancestorIfOtherItemsAreChild(selectedItemList);

//    if (ancestorItem != 0 && ancestorItem->qmlItemNode().isRootNode()) {
//        view()->changeTool(QDeclarativeDesignView::SelectionToolMode);
//        return QList<QGraphicsItem*>();
//    }

    if (ancestorItem != 0 && ancestorItem->parentItem() != 0)  {
        QList<QGraphicsItem*> ancestorItemList;
        ancestorItemList.append(ancestorItem);
        return ancestorItemList;
    }

    if (!haveSameParent(selectedItemList)) {
        view()->changeTool(Constants::SelectionToolMode);
        return QList<QGraphicsItem*>();
    }

    return selectedItemList;
}

void MoveTool::graphicsObjectsChanged(const QList<QGraphicsObject*> &itemList)
{
    m_selectionIndicator.updateItems(itemList);
    m_resizeIndicator.updateItems(itemList);
}

}
