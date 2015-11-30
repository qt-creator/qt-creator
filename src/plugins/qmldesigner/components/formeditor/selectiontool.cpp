/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "selectiontool.h"
#include "formeditorscene.h"
#include "formeditorview.h"

#include "resizehandleitem.h"

#include <nodemetainfo.h>

#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace QmlDesigner {

static const int s_startDragDistance = 20;
static const int s_startDragTime = 50;

SelectionTool::SelectionTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_rubberbandSelectionManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_singleSelectionManipulator(editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeIndicator(editorView->scene()->manipulatorLayerItem()),
    m_anchorIndicator(editorView->scene()->manipulatorLayerItem()),
    m_bindingIndicator(editorView->scene()->manipulatorLayerItem()),
    m_contentNotEditableIndicator(editorView->scene()->manipulatorLayerItem())
{
    m_selectionIndicator.setCursor(Qt::ArrowCursor);
}


SelectionTool::~SelectionTool()
{
}

void SelectionTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                    QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressTimer.start();
        FormEditorItem* formEditorItem = nearestFormEditorItem(event->scenePos(), itemList);
        if (formEditorItem
                && formEditorItem->qmlItemNode().isValid()
                && !formEditorItem->qmlItemNode().hasChildren()) {
            m_singleSelectionManipulator.begin(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection);
        } else {
            if (event->modifiers().testFlag(Qt::AltModifier)) {
                m_singleSelectionManipulator.begin(event->scenePos());

                if (event->modifiers().testFlag(Qt::ControlModifier))
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
                else if (event->modifiers().testFlag(Qt::ShiftModifier))
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
                else
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection);

                m_singleSelectionManipulator.end(event->scenePos());
                view()->changeToMoveTool(event->scenePos());
            } else {
                m_rubberbandSelectionManipulator.begin(event->scenePos());
            }
        }
    }

    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void SelectionTool::mouseMoveEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                   QGraphicsSceneMouseEvent *event)
{
    if (m_singleSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_singleSelectionManipulator.beginPoint() - event->scenePos();
        if ((mouseMovementVector.toPoint().manhattanLength() > s_startDragDistance)
            && (m_mousePressTimer.elapsed() > s_startDragTime)) {
            m_singleSelectionManipulator.end(event->scenePos());
            view()->changeToMoveTool(m_singleSelectionManipulator.beginPoint());
            return;
        }
    } else if (m_rubberbandSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
        if ((mouseMovementVector.toPoint().manhattanLength() > s_startDragDistance)
            && (m_mousePressTimer.elapsed() > s_startDragTime)) {
            m_rubberbandSelectionManipulator.update(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::AddToSelection);
            else
                m_rubberbandSelectionManipulator.select(RubberBandSelectionManipulator::ReplaceSelection);
        }
    }
}

void SelectionTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * event)
{
    if (!itemList.isEmpty()) {

        ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
        if (resizeHandle) {
            view()->changeToResizeTool();
            return;
        }

        if (topSelectedItemIsMovable(itemList)) {
            view()->changeToMoveTool();
            return;
        }
    }

    FormEditorItem *topSelectableItem = nearestFormEditorItem(event->scenePos(), itemList);

    scene()->highlightBoundingRect(topSelectableItem);

    m_contentNotEditableIndicator.setItems(toFormEditorItemList(itemList));
}

void SelectionTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                      QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_singleSelectionManipulator.isActive()) {
            m_singleSelectionManipulator.end(event->scenePos());
        } else if (m_rubberbandSelectionManipulator.isActive()) {

            QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
            if (mouseMovementVector.toPoint().manhattanLength() < s_startDragDistance) {
                m_singleSelectionManipulator.begin(event->scenePos());

                if (event->modifiers().testFlag(Qt::ControlModifier))
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
                else if (event->modifiers().testFlag(Qt::ShiftModifier))
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
                else
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection);

                m_singleSelectionManipulator.end(event->scenePos());
            } else {
                m_rubberbandSelectionManipulator.update(event->scenePos());

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

    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}

void SelectionTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent * event)
{
    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void SelectionTool::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            if (view()->changeToMoveTool())
                view()->currentTool()->keyPressEvent(event);
            break;
    }
}

void SelectionTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{

}

void SelectionTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void SelectionTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{
}

void SelectionTool::itemsAboutToRemoved(const QList<FormEditorItem*> &/*itemList*/)
{

}

//QVariant SelectionTool::itemChange(const QList<QGraphicsItem*> &itemList,
//                                           QGraphicsItem::GraphicsItemChange change,
//                                           const QVariant &value )
//{
//    qDebug() << Q_FUNC_INFO;
//    return QVariant();
//}

//void SelectionTool::update()
//{
//
//}


void SelectionTool::clear()
{
    m_rubberbandSelectionManipulator.clear(),
    m_singleSelectionManipulator.clear();
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_anchorIndicator.clear();
    m_bindingIndicator.clear();
    m_contentNotEditableIndicator.clear();

    AbstractFormEditorTool::clear();
}

void SelectionTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.setItems(itemList);
    m_resizeIndicator.setItems(itemList);
    m_anchorIndicator.setItems(itemList);
    m_bindingIndicator.setItems(itemList);
}

void SelectionTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    const QList<FormEditorItem*> selectedItemList = filterSelectedModelNodes(itemList);

    m_selectionIndicator.updateItems(selectedItemList);
    m_resizeIndicator.updateItems(selectedItemList);
    m_anchorIndicator.updateItems(selectedItemList);
    m_bindingIndicator.updateItems(selectedItemList);
    m_contentNotEditableIndicator.updateItems(selectedItemList);
}

void SelectionTool::instancesCompleted(const QList<FormEditorItem*> &/*itemList*/)
{
}

void SelectionTool::instancesParentChanged(const QList<FormEditorItem *> &/*itemList*/)
{

}

void SelectionTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void SelectionTool::selectUnderPoint(QGraphicsSceneMouseEvent *event)
{
    m_singleSelectionManipulator.begin(event->scenePos());

    if (event->modifiers().testFlag(Qt::ControlModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
    else if (event->modifiers().testFlag(Qt::ShiftModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
    else
        m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection);

    m_singleSelectionManipulator.end(event->scenePos());
}

}
