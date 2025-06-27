// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectiontool.h"
#include "formeditorgraphicsview.h"
#include "formeditorscene.h"
#include "formeditortracing.h"
#include "formeditorview.h"
#include "formeditorwidget.h"

#include "resizehandleitem.h"
#include "rotationhandleitem.h"

#include <nodemetainfo.h>

#include <utils/algorithm.h>

#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace QmlDesigner {

using FormEditorTracing::category;

const int s_startDragDistance = 20;
const int s_startDragTime = 50;

SelectionTool::SelectionTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView)
    , m_rubberbandSelectionManipulator(editorView->scene()->manipulatorLayerItem(), editorView)
    , m_singleSelectionManipulator(editorView)
    , m_selectionIndicator(editorView->scene()->manipulatorLayerItem())
    , m_resizeIndicator(editorView->scene()->manipulatorLayerItem())
    , m_rotationIndicator(editorView->scene()->manipulatorLayerItem())
    , m_anchorIndicator(editorView->scene()->manipulatorLayerItem())
    , m_bindingIndicator(editorView->scene()->manipulatorLayerItem())
{
    NanotraceHR::Tracer tracer{"selection tool constructor", category()};

    m_selectionIndicator.setCursor(Qt::ArrowCursor);
}

SelectionTool::~SelectionTool() = default;

void SelectionTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                    QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"selection tool mouse press event", category()};

    if (event->button() == Qt::LeftButton) {
        m_mousePressTimer.start();
        FormEditorItem* formEditorItem = nearestFormEditorItem(event->scenePos(), itemList);

        if (formEditorItem)
            m_itemSelectedAndMovable = toQmlItemNodeList(view()->selectedModelNodes()).contains(formEditorItem->qmlItemNode())
                    && view()->hasSingleSelectedModelNode() && !formEditorItem->qmlItemNode().isRootNode();
        else
            m_itemSelectedAndMovable = false;

        if (formEditorItem && m_itemSelectedAndMovable
                && formEditorItem->qmlItemNode().isValid()) {
            m_singleSelectionManipulator.begin(event->scenePos());


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
    NanotraceHR::Tracer tracer{"selection tool mouse move event", category()};

    if (m_singleSelectionManipulator.isActive()) {
        QPointF mouseMovementVector = m_singleSelectionManipulator.beginPoint() - event->scenePos();
        if ((mouseMovementVector.toPoint().manhattanLength() > s_startDragDistance)
            && (m_mousePressTimer.elapsed() > s_startDragTime)) {
            m_singleSelectionManipulator.end(event->scenePos());
            if (m_itemSelectedAndMovable)
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
    NanotraceHR::Tracer tracer{"selection tool hover move event", category()};

    if (!itemList.isEmpty()) {
        ResizeHandleItem *resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.constFirst());
        if (resizeHandle) {
            view()->changeToResizeTool();
            return;
        }

        RotationHandleItem* rotationHandle = RotationHandleItem::fromGraphicsItem(itemList.constFirst());
        if (rotationHandle) {
            view()->changeToRotationTool();
            return;
        }

        if ((topSelectedItemIsMovable(itemList) && !view()->hasSingleSelectedModelNode())
                || (selectedItemCursorInMovableArea(event->scenePos())
                && !event->modifiers().testFlag(Qt::ControlModifier)
                && !event->modifiers().testFlag(Qt::ShiftModifier))) {
            view()->changeToMoveTool();
            return;
        }
    }

    FormEditorItem *topSelectableItem = nearestFormEditorItem(event->scenePos(), itemList);


    if (topSelectableItem && toQmlItemNodeList(view()->selectedModelNodes()).contains(topSelectableItem->qmlItemNode())
            && topSelectedItemIsMovable({topSelectableItem})) {
        view()->formEditorWidget()->graphicsView()->viewport()->setCursor(Qt::SizeAllCursor);
    } else {
        view()->formEditorWidget()->graphicsView()->viewport()->unsetCursor();
    }

    scene()->highlightBoundingRect(topSelectableItem);
}

void SelectionTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                      QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"selection tool mouse release event", category()};

    if (event->button() == Qt::LeftButton) {
        if (m_singleSelectionManipulator.isActive()) {
            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection);
            m_singleSelectionManipulator.end(event->scenePos());
        } else if (m_rubberbandSelectionManipulator.isActive()) {

            QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
            if (mouseMovementVector.toPoint().manhattanLength() < s_startDragDistance) {
                m_singleSelectionManipulator.begin(event->scenePos());

                if (event->modifiers().testFlag(Qt::ControlModifier))
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);
                else if (event->modifiers().testFlag(Qt::ShiftModifier))
                    m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);
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
    NanotraceHR::Tracer tracer{"selection tool mouse double click event", category()};

    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void SelectionTool::keyPressEvent(QKeyEvent *event)
{
    NanotraceHR::Tracer tracer{"selection tool key press event", category()};

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

void SelectionTool::itemsAboutToRemoved(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"selection tool items about to removed", category()};

    const QList<FormEditorItem *> current = items();

    QList<FormEditorItem *> remaining = Utils::filtered(current, [&itemList](FormEditorItem *item) {
        return !itemList.contains(item);
    });

    if (!remaining.isEmpty())
        setItems(remaining);
    else
        clear();
}

void SelectionTool::clear()
{
    NanotraceHR::Tracer tracer{"selection tool clear", category()};

    m_rubberbandSelectionManipulator.clear(), m_singleSelectionManipulator.clear();
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_rotationIndicator.clear();
    m_anchorIndicator.clear();
    m_bindingIndicator.clear();

    AbstractFormEditorTool::clear();
}

void SelectionTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"selection tool selected items changed", category()};

    m_selectionIndicator.setItems(itemList);
    m_resizeIndicator.setItems(itemList);
    m_rotationIndicator.setItems(itemList);
    m_anchorIndicator.setItems(itemList);
    m_bindingIndicator.setItems(itemList);
}

void SelectionTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"selection tool form editor items changed", category()};

    const QList<FormEditorItem*> selectedItemList = filterSelectedModelNodes(itemList);

    m_selectionIndicator.updateItems(selectedItemList);
    m_resizeIndicator.updateItems(selectedItemList);
    m_rotationIndicator.updateItems(selectedItemList);
    m_anchorIndicator.updateItems(selectedItemList);
    m_bindingIndicator.updateItems(selectedItemList);
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
    NanotraceHR::Tracer tracer{"selection tool select under point", category()};

    m_singleSelectionManipulator.begin(event->scenePos());

    if (event->modifiers().testFlag(Qt::ControlModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
    else if (event->modifiers().testFlag(Qt::ShiftModifier))
        m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
    else
        m_singleSelectionManipulator.select(SingleSelectionManipulator::ReplaceSelection);

    m_singleSelectionManipulator.end(event->scenePos());
}

void SelectionTool::focusLost()
{
}

}
