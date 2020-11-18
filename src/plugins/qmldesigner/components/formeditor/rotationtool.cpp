/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "rotationtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"

#include "rotationhandleitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>

namespace QmlDesigner {

RotationTool::RotationTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView)
    , m_selectionIndicator(editorView->scene()->manipulatorLayerItem())
    , m_rotationIndicator(editorView->scene()->manipulatorLayerItem())
    , m_anchorIndicator(editorView->scene()->manipulatorLayerItem())
    , m_rotationManipulator(editorView->scene()->manipulatorLayerItem(), editorView)
{
}

RotationTool::~RotationTool() = default;


void RotationTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (itemList.isEmpty())
            return;

        RotationHandleItem *rotationHandle = RotationHandleItem::fromGraphicsItem(itemList.constFirst());
        if (rotationHandle && rotationHandle->rotationController().isValid()) {
            m_rotationManipulator.setHandle(rotationHandle);
            m_rotationManipulator.begin(event->scenePos());
            m_rotationIndicator.hide();
            m_anchorIndicator.hide();
        }
    }

    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void RotationTool::mouseMoveEvent(const QList<QGraphicsItem*> &,
                                           QGraphicsSceneMouseEvent *event)
{
    if (m_rotationManipulator.isActive())
        m_rotationManipulator.update(event->scenePos(), event->modifiers());
}

void RotationTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * /*event*/)
{
    if (itemList.isEmpty()) {
       view()->changeToSelectionTool();
       return;
    }

    RotationHandleItem* rotationHandle = RotationHandleItem::fromGraphicsItem(itemList.constFirst());
    if (rotationHandle && rotationHandle->rotationController().isValid()) {
        m_rotationManipulator.setHandle(rotationHandle);
    } else {
        view()->changeToSelectionTool();
        return;
    }
}

void RotationTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void RotationTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}


void RotationTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                              QGraphicsSceneMouseEvent *event)
{
    if (m_rotationManipulator.isActive()) {
        if (itemList.isEmpty())
            return;

        m_selectionIndicator.show();
        m_rotationIndicator.show();
        m_anchorIndicator.show();
        m_rotationManipulator.end();
    }

    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}

void RotationTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                              QGraphicsSceneMouseEvent * /*event*/)
{
}

void RotationTool::keyPressEvent(QKeyEvent * event)
{
    switch (event->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_AltGr:
            event->setAccepted(false);
            return;
    }
}

void RotationTool::keyReleaseEvent(QKeyEvent * keyEvent)
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

void RotationTool::itemsAboutToRemoved(const QList<FormEditorItem*> & /*itemList*/)
{

}

void RotationTool::selectedItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
    m_selectionIndicator.setItems(items());
    m_rotationIndicator.setItems(items());
    m_anchorIndicator.setItems(items());
}

void RotationTool::clear()
{
    m_selectionIndicator.clear();
    m_rotationIndicator.clear();
    m_anchorIndicator.clear();
    m_rotationManipulator.clear();
}

void RotationTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    const QList<FormEditorItem*> selectedItemList = filterSelectedModelNodes(itemList);

    m_selectionIndicator.updateItems(selectedItemList);
    m_rotationIndicator.updateItems(selectedItemList);
    m_anchorIndicator.updateItems(selectedItemList);
}

void RotationTool::instancesCompleted(const QList<FormEditorItem*> &/*itemList*/)
{
}

void RotationTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
}

void RotationTool::focusLost()
{
}


void RotationTool::instancesParentChanged(const QList<FormEditorItem *> &/*itemList*/)
{

}

} //namespace QmlDesigner
