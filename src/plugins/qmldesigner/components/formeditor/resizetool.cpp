/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "resizetool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"

#include "resizehandleitem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QtDebug>

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
    if (itemList.isEmpty())
        return;

    ResizeHandleItem *resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle) {
        m_resizeManipulator.setHandle(resizeHandle);
        m_resizeManipulator.begin(event->scenePos());
        m_selectionIndicator.hide();
        m_resizeIndicator.hide();
    }
}

void ResizeTool::mouseMoveEvent(const QList<QGraphicsItem*> &,
                                           QGraphicsSceneMouseEvent *event)
{
    bool shouldSnapping = view()->widget()->snappingAction()->isChecked();
    bool shouldSnappingAndAnchoring = view()->widget()->snappingAndAnchoringAction()->isChecked();

    ResizeManipulator::Snapping useSnapping = ResizeManipulator::NoSnapping;
    if (event->modifiers().testFlag(Qt::ControlModifier) != (shouldSnapping || shouldSnappingAndAnchoring)) {
        if (shouldSnappingAndAnchoring)
            useSnapping = ResizeManipulator::UseSnappingAndAnchoring;
        else
            useSnapping = ResizeManipulator::UseSnapping;
    }

    m_resizeManipulator.update(event->scenePos(), useSnapping);
}

void ResizeTool::hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent * /*event*/)
{
    if (itemList.isEmpty())
        return;
    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle) {
        m_resizeManipulator.setHandle(resizeHandle);
    } else {
        view()->changeToSelectionTool();
        return;
    }
}

void ResizeTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                              QGraphicsSceneMouseEvent * /*event*/)
{
    if (itemList.isEmpty())
        return;

    m_selectionIndicator.show();
    m_resizeIndicator.show();
    m_resizeManipulator.end();
}

void ResizeTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                              QGraphicsSceneMouseEvent * /*event*/)
{
}

void ResizeTool::keyPressEvent(QKeyEvent * event)
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

    switch(event->key()) {
        case Qt::Key_Left: m_resizeManipulator.moveBy(-moveStep, 0.0); break;
        case Qt::Key_Right: m_resizeManipulator.moveBy(moveStep, 0.0); break;
        case Qt::Key_Up: m_resizeManipulator.moveBy(0.0, -moveStep); break;
        case Qt::Key_Down: m_resizeManipulator.moveBy(0.0, moveStep); break;
    }

}

void ResizeTool::keyReleaseEvent(QKeyEvent * keyEvent)
{
     switch(keyEvent->key()) {
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

}
