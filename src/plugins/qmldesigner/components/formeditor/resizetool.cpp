// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resizetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"

#include "resizehandleitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>

namespace QmlDesigner {

ResizeTool::ResizeTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView)
    , m_selectionIndicator(editorView->scene()->manipulatorLayerItem())
    , m_resizeIndicator(editorView->scene()->manipulatorLayerItem())
    , m_anchorIndicator(editorView->scene()->manipulatorLayerItem())
    , m_rotationIndicator(editorView->scene()->manipulatorLayerItem())
    , m_resizeManipulator(editorView->scene()->manipulatorLayerItem(), editorView)
{
}

ResizeTool::~ResizeTool() = default;

void ResizeTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (itemList.isEmpty())
            return;

        ResizeHandleItem *resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.constFirst());
        if (resizeHandle && resizeHandle->resizeController().isValid()) {
            m_resizeManipulator.setHandle(resizeHandle);
            m_resizeManipulator.begin(event->scenePos());
            m_resizeIndicator.hide();
            m_anchorIndicator.hide();
            m_rotationIndicator.hide();
        }
    }

    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void ResizeTool::mouseMoveEvent(const QList<QGraphicsItem*> &,
                                           QGraphicsSceneMouseEvent *event)
{
    if (m_resizeManipulator.isActive())
        m_resizeManipulator.update(event->scenePos(), generateUseSnapping(event->modifiers()),
                                   event->modifiers());
}

void ResizeTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * /*event*/)
{
    if (itemList.isEmpty()) {
       view()->changeToSelectionTool();
       return;
    }

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.constFirst());
    if (resizeHandle && resizeHandle->resizeController().isValid()) {
        m_resizeManipulator.setHandle(resizeHandle);
    } else {
        view()->changeToSelectionTool();
        return;
    }
}

void ResizeTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void ResizeTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}


void ResizeTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                              QGraphicsSceneMouseEvent *event)
{
    if (m_resizeManipulator.isActive()) {
        if (itemList.isEmpty())
            return;

        m_selectionIndicator.show();
        m_resizeIndicator.show();
        m_anchorIndicator.show();
        m_rotationIndicator.show();
        m_resizeManipulator.end(generateUseSnapping(event->modifiers()));
    }

    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}

void ResizeTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                              QGraphicsSceneMouseEvent * /*event*/)
{
}

void ResizeTool::keyPressEvent(QKeyEvent * event)
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

    switch (event->key()) {
        case Qt::Key_Left: m_resizeManipulator.moveBy(-moveStep, 0.0); break;
        case Qt::Key_Right: m_resizeManipulator.moveBy(moveStep, 0.0); break;
        case Qt::Key_Up: m_resizeManipulator.moveBy(0.0, -moveStep); break;
        case Qt::Key_Down: m_resizeManipulator.moveBy(0.0, moveStep); break;
    }

}

void ResizeTool::keyReleaseEvent(QKeyEvent * keyEvent)
{
     switch (keyEvent->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_AltGr:
            keyEvent->setAccepted(false);
            return;
    }
}

void ResizeTool::itemsAboutToRemoved(const QList<FormEditorItem*> & /*itemList*/)
{

}

void ResizeTool::selectedItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
    m_selectionIndicator.setItems(items());
    m_resizeIndicator.setItems(items());
    m_anchorIndicator.setItems(items());
    m_rotationIndicator.setItems(items());
}

void ResizeTool::clear()
{
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_anchorIndicator.clear();
    m_rotationIndicator.clear();
    m_resizeManipulator.clear();
}

void ResizeTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    const QList<FormEditorItem*> selectedItemList = filterSelectedModelNodes(itemList);

    m_selectionIndicator.updateItems(selectedItemList);
    m_resizeIndicator.updateItems(selectedItemList);
    m_anchorIndicator.updateItems(selectedItemList);
    m_rotationIndicator.updateItems(selectedItemList);
}

void ResizeTool::instancesCompleted(const QList<FormEditorItem*> &/*itemList*/)
{
}

void ResizeTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void ResizeTool::focusLost()
{
}


void ResizeTool::instancesParentChanged(const QList<FormEditorItem *> &/*itemList*/)
{

}

}
