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

#include "resizetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"

#include "resizehandleitem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>

namespace QmlDesigner {

ResizeTool::ResizeTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeManipulator(editorView->scene()->manipulatorLayerItem(), editorView)
{
}


ResizeTool::~ResizeTool()
{
}

void ResizeTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (itemList.isEmpty())
            return;

        ResizeHandleItem *resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
        if (resizeHandle && resizeHandle->resizeController().isValid()) {
            m_resizeManipulator.setHandle(resizeHandle);
            m_resizeManipulator.begin(event->scenePos());
            m_resizeIndicator.hide();
        }
    }

    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void ResizeTool::mouseMoveEvent(const QList<QGraphicsItem*> &,
                                           QGraphicsSceneMouseEvent *event)
{
    if (m_resizeManipulator.isActive()) {
        bool shouldSnapping = view()->formEditorWidget()->snappingAction()->isChecked();
        bool shouldSnappingAndAnchoring = view()->formEditorWidget()->snappingAndAnchoringAction()->isChecked();

        ResizeManipulator::Snapping useSnapping = ResizeManipulator::NoSnapping;
        if (event->modifiers().testFlag(Qt::ControlModifier) != (shouldSnapping || shouldSnappingAndAnchoring)) {
            if (shouldSnappingAndAnchoring)
                useSnapping = ResizeManipulator::UseSnappingAndAnchoring;
            else
                useSnapping = ResizeManipulator::UseSnapping;
        }

        m_resizeManipulator.update(event->scenePos(), useSnapping);
    }
}

void ResizeTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * /*event*/)
{
    if (itemList.isEmpty()) {
       view()->changeToSelectionTool();
       return;
    }

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle && resizeHandle->resizeController().isValid()) {
        m_resizeManipulator.setHandle(resizeHandle);
    } else {
        view()->changeToSelectionTool();
        return;
    }
}

void ResizeTool::dragLeaveEvent(QGraphicsSceneDragDropEvent * /*event*/)
{

}

void ResizeTool::dragMoveEvent(QGraphicsSceneDragDropEvent * /*event*/)
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
        m_resizeManipulator.end();
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

//     if (!keyEvent->isAutoRepeat())
//         m_resizeManipulator.clear();
}

void ResizeTool::itemsAboutToRemoved(const QList<FormEditorItem*> & /*itemList*/)
{

}

void ResizeTool::selectedItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
    m_selectionIndicator.setItems(items());
    m_resizeIndicator.setItems(items());
}

void ResizeTool::clear()
{
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_resizeManipulator.clear();
}

void ResizeTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.updateItems(itemList);
    m_resizeIndicator.updateItems(itemList);
}

void ResizeTool::instancesCompleted(const QList<FormEditorItem*> &/*itemList*/)
{
}


void ResizeTool::instancesParentChanged(const QList<FormEditorItem *> &/*itemList*/)
{

}

}
