// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "movetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditorgraphicsview.h"

#include "resizehandleitem.h"
#include "rotationhandleitem.h"

#include <utils/algorithm.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>

namespace QmlDesigner {

using FormEditorTracing::category;

MoveTool::MoveTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView)
    , m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView)
    , m_selectionIndicator(editorView->scene()->manipulatorLayerItem())
    , m_resizeIndicator(editorView->scene()->manipulatorLayerItem())
    , m_rotationIndicator(editorView->scene()->manipulatorLayerItem())
    , m_anchorIndicator(editorView->scene()->manipulatorLayerItem())
    , m_bindingIndicator(editorView->scene()->manipulatorLayerItem())

{
    NanotraceHR::Tracer tracer{"move tool constructor", category()};

    m_selectionIndicator.setCursor(Qt::SizeAllCursor);
}

MoveTool::~MoveTool() = default;

void MoveTool::clear()
{
    NanotraceHR::Tracer tracer{"move tool clear", category()};

    m_moveManipulator.clear();
    m_movingItems.clear();
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_rotationIndicator.clear();
    m_anchorIndicator.clear();
    m_bindingIndicator.clear();

    AbstractFormEditorTool::clear();
    if (view()->formEditorWidget()->graphicsView())
        view()->formEditorWidget()->graphicsView()->viewport()->unsetCursor();
}

void MoveTool::start()
{
    NanotraceHR::Tracer tracer{"move tool start", category()};

    view()->formEditorWidget()->graphicsView()->viewport()->setCursor(Qt::SizeAllCursor);
}

void MoveTool::mousePressEvent(const QList<QGraphicsItem *> &itemList, QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"move tool mouse press event", category()};

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

void MoveTool::mouseMoveEvent(const QList<QGraphicsItem *> &itemList, QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"move tool mouse move event", category()};

    if (m_moveManipulator.isActive()) {
        if (m_movingItems.isEmpty())
            return;

        m_selectionIndicator.hide();
        m_resizeIndicator.hide();
        m_rotationIndicator.hide();
        m_anchorIndicator.hide();
        m_bindingIndicator.hide();

        FormEditorItem *containerItem = containerFormEditorItem(itemList, m_movingItems);
        if (containerItem && QmlModelState::isBaseState(view()->currentStateNode())) {
            if (containerItem != m_movingItems.constFirst()->parentItem()
                    && event->modifiers().testFlag(Qt::ControlModifier)
                    && event->modifiers().testFlag(Qt::ShiftModifier)) {

                const FormEditorItem *movingItem = m_movingItems.constFirst();

                if (m_movingItems.size() > 1
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
    NanotraceHR::Tracer tracer{"move tool hover move event", category()};

    if (itemList.isEmpty()) {
        view()->changeToSelectionTool();
        return;
    }

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.constFirst());
    if (resizeHandle) {
        view()->changeToResizeTool();
        return;
    }

    RotationHandleItem* rotationHandle = RotationHandleItem::fromGraphicsItem(itemList.constFirst());
    if (rotationHandle) {
        view()->changeToRotationTool();
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
}

void MoveTool::keyPressEvent(QKeyEvent *event)
{
    NanotraceHR::Tracer tracer{"move tool key press event", category()};

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
        m_rotationIndicator.hide();
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
    NanotraceHR::Tracer tracer{"move tool key release event", category()};

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
        m_rotationIndicator.show();
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
    NanotraceHR::Tracer tracer{"move tool mouse release event", category()};

    if (m_moveManipulator.isActive()) {
        if (m_movingItems.isEmpty())
            return;

        m_moveManipulator.end(generateUseSnapping(event->modifiers()));

        m_selectionIndicator.show();
        m_resizeIndicator.show();
        m_rotationIndicator.show();
        m_anchorIndicator.show();
        m_bindingIndicator.show();
        m_movingItems.clear();
    }

    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);

    view()->changeToSelectionTool();
}

void MoveTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"move tool mouse double click event", category()};

    AbstractFormEditorTool::mouseDoubleClickEvent(itemList, event);
}

void MoveTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    NanotraceHR::Tracer tracer{"move tool items about to be removed", category()};

    for (FormEditorItem *removedItem : removedItemList)
        m_movingItems.removeOne(removedItem);
}

void MoveTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"move tool selected items changed", category()};

    m_selectionIndicator.setItems(movingItems(itemList));
    m_resizeIndicator.setItems(itemList);
    m_rotationIndicator.setItems(itemList);
    m_anchorIndicator.setItems(itemList);
    m_bindingIndicator.setItems(itemList);
    updateMoveManipulator();
}

void MoveTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  MoveTool::instancesParentChanged(const QList<FormEditorItem *> &itemList)
{
    NanotraceHR::Tracer tracer{"move tool instances parent changed", category()};

    m_moveManipulator.synchronizeInstanceParent(itemList);
}

void MoveTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

bool MoveTool::haveSameParent(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"move tool have same parent", category()};

    if (itemList.isEmpty())
        return false;

    QGraphicsItem *firstParent = itemList.constFirst()->parentItem();
    for (FormEditorItem* item : itemList)
    {
        if (firstParent != item->parentItem())
            return false;
    }

    return true;
}

bool MoveTool::isAncestorOfAllItems(FormEditorItem* maybeAncestorItem,
                                    const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"move tool is ancestor of all items", category()};

    for (FormEditorItem *item : itemList) {
        if (!maybeAncestorItem->isAncestorOf(item) && item != maybeAncestorItem)
            return false;
    }

    return true;
}


FormEditorItem* MoveTool::ancestorIfOtherItemsAreChild(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"move tool ancestor if other items are child", category()};

    if (itemList.isEmpty())
        return nullptr;


    for (FormEditorItem* item : itemList)
    {
        if (isAncestorOfAllItems(item, itemList))
            return item;
    }

    return nullptr;
}

void MoveTool::updateMoveManipulator()
{
    NanotraceHR::Tracer tracer{"move tool update move manipulator", category()};

    if (m_moveManipulator.isActive())
        return;
}

void MoveTool::beginWithPoint(const QPointF &beginPoint)
{
    NanotraceHR::Tracer tracer{"move tool begin with point", category()};

    m_movingItems = movingItems(items());
    if (m_movingItems.isEmpty())
        return;

    m_moveManipulator.setItems(m_movingItems);
    m_moveManipulator.begin(beginPoint);
}

static QList<FormEditorItem *> movableItems(const QList<FormEditorItem *> &itemList)
{
    return Utils::filtered(itemList, [](FormEditorItem *item) {
        const QmlItemNode node = item->qmlItemNode();
        return node.isValid()
            && node.instanceIsMovable()
            && node.modelIsMovable()
            && !node.instanceIsInLayoutable();
    });
}

QList<FormEditorItem*> MoveTool::movingItems(const QList<FormEditorItem*> &selectedItemList)
{
    NanotraceHR::Tracer tracer{"move tool moving items", category()};

    QList<FormEditorItem *> filteredItemList = movableItems(selectedItemList);

    FormEditorItem* ancestorItem = ancestorIfOtherItemsAreChild(filteredItemList);

    if (ancestorItem != nullptr && ancestorItem->qmlItemNode().isRootNode()) {
//        view()->changeToSelectionTool();
        return {};
    }

    if (ancestorItem != nullptr && ancestorItem->parentItem() != nullptr)  {
        QList<FormEditorItem*> ancestorItemList;
        ancestorItemList.append(ancestorItem);
        return ancestorItemList;
    }

    if (!haveSameParent(filteredItemList)) {
//        view()->changeToSelectionTool();
        return {};
    }

    return filteredItemList;
}

void MoveTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    NanotraceHR::Tracer tracer{"move tool form editor items changed", category()};

    const QList<FormEditorItem*> selectedItemList = filterSelectedModelNodes(itemList);

    m_selectionIndicator.updateItems(selectedItemList);
    m_resizeIndicator.updateItems(selectedItemList);
    m_rotationIndicator.updateItems(selectedItemList);
    m_anchorIndicator.updateItems(selectedItemList);
    m_bindingIndicator.updateItems(selectedItemList);
}

void MoveTool::focusLost()
{
}

}
