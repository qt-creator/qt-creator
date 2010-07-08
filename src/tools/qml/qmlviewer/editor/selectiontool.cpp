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

#include "selectiontool.h"

//#include "resizehandleitem.h"
#include "qdeclarativedesignview.h"

#include <private/qdeclarativeitem_p.h>
#include <private/qdeclarativecontext_p.h>
#include <QDeclarativeEngine>

#include <QApplication>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QClipboard>
#include <QMenu>
#include <QAction>
#include <QDeclarativeItem>
#include <QGraphicsObject>

#include <QDebug>

namespace QmlViewer {

SelectionTool::SelectionTool(QDeclarativeDesignView *editorView)
    : AbstractFormEditorTool(editorView),
    m_rubberbandSelectionMode(false),
    m_rubberbandSelectionManipulator(editorView->manipulatorLayer(), editorView),
    m_singleSelectionManipulator(editorView),
    m_selectionIndicator(editorView->manipulatorLayer()),
    //m_resizeIndicator(editorView->manipulatorLayer()),
    m_selectOnlyContentItems(true)
{

}

SelectionTool::~SelectionTool()
{
}

void SelectionTool::setRubberbandSelectionMode(bool value)
{
    m_rubberbandSelectionMode = value;
}

SingleSelectionManipulator::SelectionType SelectionTool::getSelectionType(Qt::KeyboardModifiers modifiers)
{
    SingleSelectionManipulator::SelectionType selectionType = SingleSelectionManipulator::ReplaceSelection;
    if (modifiers.testFlag(Qt::ControlModifier)) {
        selectionType = SingleSelectionManipulator::RemoveFromSelection;
    } else if (modifiers.testFlag(Qt::ShiftModifier)) {
        selectionType = SingleSelectionManipulator::AddToSelection;
    }
    return selectionType;
}

bool SelectionTool::alreadySelected(const QList<QGraphicsItem*> &itemList) const
{
    const QList<QGraphicsItem*> selectedItems = view()->selectedItems();

    if (selectedItems.isEmpty())
        return false;

    foreach(QGraphicsItem *item, itemList) {
        if (selectedItems.contains(item)) {
            return true;
        }
    }

    return false;
}

void SelectionTool::mousePressEvent(QMouseEvent *event)
{
    QList<QGraphicsItem*> itemList = view()->selectableItems(event->pos());
    SingleSelectionManipulator::SelectionType selectionType = getSelectionType(event->modifiers());

    if (event->buttons() & Qt::LeftButton) {
        m_mousePressTimer.start();

        if (m_rubberbandSelectionMode) {
            m_rubberbandSelectionManipulator.begin(event->pos());
        } else {

            if (itemList.isEmpty()) {
                view()->setSelectedItems(itemList);
                return;
            }

            if ((selectionType == SingleSelectionManipulator::InvertSelection
                 || selectionType == SingleSelectionManipulator::ReplaceSelection)
                && alreadySelected(itemList))
            {
                //view()->changeToMoveTool(event->pos());
                return;
            }

            QGraphicsItem* item = itemList.first();

            if (item->children().isEmpty()) {
                m_singleSelectionManipulator.begin(event->pos());
                m_singleSelectionManipulator.select(selectionType, m_selectOnlyContentItems);
            } else {
                m_mousePressTimer.start();

                if (itemList.isEmpty()) {
                    view()->setSelectedItems(itemList);
                    return;
                }

                if (item->children().isEmpty()) {
                    m_singleSelectionManipulator.begin(event->pos());
                    m_singleSelectionManipulator.select(selectionType, m_selectOnlyContentItems);
                } else {
                    m_singleSelectionManipulator.begin(event->pos());
                    m_singleSelectionManipulator.select(selectionType, m_selectOnlyContentItems);
                    m_singleSelectionManipulator.end(event->pos());
                    //view()->changeToMoveTool(event->pos());
                }

                m_singleSelectionManipulator.begin(event->pos());
                m_singleSelectionManipulator.select(selectionType, m_selectOnlyContentItems);
                m_singleSelectionManipulator.end(event->pos());
                //view()->changeToMoveTool(event->pos());

            }
        }

    } else if (event->buttons() & Qt::RightButton) {
        createContextMenu(itemList, event->globalPos());
    }
}

void SelectionTool::createContextMenu(QList<QGraphicsItem*> itemList, QPoint globalPos)
{
    if (!view()->mouseInsideContextItem())
        return;

    QMenu contextMenu;
    connect(&contextMenu, SIGNAL(hovered(QAction*)), this, SLOT(contextMenuElementHovered(QAction*)));

//    QList<QGraphicsItem*> sortedList;
//    for(int i = itemList.length(); i>= 0; i--) {
//    }

    m_contextMenuItemList = itemList;

    contextMenu.addAction("Items");
    contextMenu.addSeparator();
    int shortcutKey = Qt::Key_1;
    bool addKeySequence = true;
    int i = 0;

    foreach(QGraphicsItem * const item, itemList) {
        QString itemTitle = titleForItem(item);
        QAction *elementAction = contextMenu.addAction(itemTitle, this, SLOT(contextMenuElementSelected()));

        if (view()->selectedItems().contains(item)) {
            QFont boldFont = elementAction->font();
            boldFont.setBold(true);
            elementAction->setFont(boldFont);
        }

        elementAction->setData(i);
        if (addKeySequence)
            elementAction->setShortcut(QKeySequence(shortcutKey));

        shortcutKey++;
        if (shortcutKey > Qt::Key_9)
            addKeySequence = false;

        ++i;
    }
    // add root item separately
    QString itemTitle = QString(tr("%1 (root)")).arg(titleForItem(view()->currentRootItem()));
    contextMenu.addAction(itemTitle, this, SLOT(contextMenuElementSelected()));
    m_contextMenuItemList.append(view()->currentRootItem());

    contextMenu.exec(globalPos);
    m_contextMenuItemList.clear();
}

void SelectionTool::contextMenuElementSelected()
{
    QAction *senderAction = static_cast<QAction*>(sender());
    int itemListIndex = senderAction->data().toInt();
    if (itemListIndex >= 0 && itemListIndex < m_contextMenuItemList.length()) {

        QPointF updatePt(0, 0);
        m_singleSelectionManipulator.begin(updatePt);
        m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection,
                                            QList<QGraphicsItem*>() << m_contextMenuItemList.at(itemListIndex),
                                            false);
        m_singleSelectionManipulator.end(updatePt);
    }
}

void SelectionTool::contextMenuElementHovered(QAction *action)
{
    int itemListIndex = action->data().toInt();
    if (itemListIndex >= 0 && itemListIndex < m_contextMenuItemList.length()) {
        view()->highlightBoundingRect(m_contextMenuItemList.at(itemListIndex));
    }
}

void SelectionTool::mouseMoveEvent(QMouseEvent *event)
{
    if (m_singleSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_singleSelectionManipulator.beginPoint() - event->pos();

        if ((mouseMovementVector.toPoint().manhattanLength() > Constants::DragStartDistance)
            && (m_mousePressTimer.elapsed() > Constants::DragStartTime))
        {
            m_singleSelectionManipulator.end(event->pos());
            //view()->changeToMoveTool(m_singleSelectionManipulator.beginPoint());
            return;
        }
    } else if (m_rubberbandSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->pos();

        if ((mouseMovementVector.toPoint().manhattanLength() > Constants::DragStartDistance)
            && (m_mousePressTimer.elapsed() > Constants::DragStartTime)) {
            m_rubberbandSelectionManipulator.update(event->pos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::AddToSelection);
            else
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::ReplaceSelection);
        }
    }
}

void SelectionTool::hoverMoveEvent(QMouseEvent * event)
{
    QList<QGraphicsItem*> itemList = view()->items(event->pos());
    if (!itemList.isEmpty() && !m_rubberbandSelectionMode) {

//        foreach(QGraphicsItem *item, itemList) {
//            if (item->type() == Constants::ResizeHandleItemType) {
//                ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(item);
//                if (resizeHandle)
//                    view()->changeTool(Constants::ResizeToolMode);
//                return;
//            }
//        }
//        if (topSelectedItemIsMovable(itemList))
//            view()->changeTool(Constants::MoveToolMode);
    }

    QList<QGraphicsItem*> selectableItemList = view()->selectableItems(event->pos());
    if (!selectableItemList.isEmpty()) {
        QGraphicsItem *topSelectableItem = 0;
        foreach(QGraphicsItem* item, selectableItemList)
        {
            if (item
                && item->type() != Constants::EditorItemType
                /*&& !item->qmlItemNode().isRootNode()
                && (QGraphicsItem->qmlItemNode().hasShowContent() || !m_selectOnlyContentItems)*/)
            {
                topSelectableItem = item;
                break;
            }
        }

        view()->highlightBoundingRect(topSelectableItem);
    }
}

void SelectionTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_singleSelectionManipulator.isActive()) {
        m_singleSelectionManipulator.end(event->pos());
    }
    else if (m_rubberbandSelectionManipulator.isActive()) {

        QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->pos();
        if (mouseMovementVector.toPoint().manhattanLength() < Constants::DragStartDistance) {
            m_singleSelectionManipulator.begin(event->pos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection, m_selectOnlyContentItems);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection, m_selectOnlyContentItems);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection, m_selectOnlyContentItems);

            m_singleSelectionManipulator.end(event->pos());
        } else {
            m_rubberbandSelectionManipulator.update(event->pos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::AddToSelection);
            else
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::ReplaceSelection);

            m_rubberbandSelectionManipulator.end();
        }
    }
}

void SelectionTool::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{

}

void SelectionTool::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            view()->changeTool(Constants::MoveToolMode);
            view()->currentTool()->keyPressEvent(event);
            break;
    }
}

void SelectionTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{

}

void SelectionTool::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() == Qt::Horizontal || m_rubberbandSelectionMode)
        return;

    QList<QGraphicsItem*> itemList = view()->selectableItems(event->pos());

    int selectedIdx = 0;
    if (!view()->selectedItems().isEmpty()) {
        selectedIdx = itemList.indexOf(view()->selectedItems().first());
        if (selectedIdx >= 0) {
            if (event->delta() > 0) {
                selectedIdx++;
                if (selectedIdx == itemList.length())
                    selectedIdx = 0;
            } else if (event->delta() < 0) {
                selectedIdx--;
                if (selectedIdx == -1)
                    selectedIdx = itemList.length() - 1;
            }
        } else {
            selectedIdx = 0;
        }
    }

    QPointF updatePt(0, 0);
    m_singleSelectionManipulator.begin(updatePt);
    m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection,
                                        QList<QGraphicsItem*>() << itemList.at(selectedIdx),
                                        false);
    m_singleSelectionManipulator.end(updatePt);

}

void SelectionTool::setSelectOnlyContentItems(bool selectOnlyContentItems)
{
    m_selectOnlyContentItems = selectOnlyContentItems;
}

void SelectionTool::itemsAboutToRemoved(const QList<QGraphicsItem*> &/*itemList*/)
{

}

void SelectionTool::clear()
{
    view()->setCursor(Qt::ArrowCursor);
    m_rubberbandSelectionManipulator.clear(),
    m_singleSelectionManipulator.clear();
    m_selectionIndicator.clear();
    //m_resizeIndicator.clear();
}

void SelectionTool::selectedItemsChanged(const QList<QGraphicsItem*> &itemList)
{
    m_selectionIndicator.setItems(toGraphicsObjectList(itemList));
    //m_resizeIndicator.setItems(toGraphicsObjectList(itemList));
}

void SelectionTool::graphicsObjectsChanged(const QList<QGraphicsObject*> &itemList)
{
    m_selectionIndicator.updateItems(itemList);
    //m_resizeIndicator.updateItems(itemList);
}

void SelectionTool::selectUnderPoint(QMouseEvent *event)
{
    m_singleSelectionManipulator.begin(event->pos());

    if (event->modifiers().testFlag(Qt::ControlModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection, m_selectOnlyContentItems);
    else if (event->modifiers().testFlag(Qt::ShiftModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection, m_selectOnlyContentItems);
    else
        m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection, m_selectOnlyContentItems);

    m_singleSelectionManipulator.end(event->pos());
}

}
