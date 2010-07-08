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

#include "resizetool.h"
#include "resizehandleitem.h"
#include "qdeclarativedesignview.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QKeyEvent>
#include <QGraphicsObject>
#include <QWheelEvent>

#include <QtDebug>

namespace QmlViewer {

ResizeTool::ResizeTool(QDeclarativeDesignView *view)
    : AbstractFormEditorTool(view),
    m_selectionIndicator(view->manipulatorLayer()),
    m_resizeManipulator(new ResizeManipulator(view->manipulatorLayer(), view)),
    m_resizeIndicator(view->manipulatorLayer())
{
}


ResizeTool::~ResizeTool()
{
    delete m_resizeManipulator;
}

void ResizeTool::mousePressEvent(QMouseEvent *event)
{
    QList<QGraphicsItem*> itemList = view()->items(event->pos());

    if (itemList.isEmpty())
        return;

    ResizeHandleItem *resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());

    if (resizeHandle)
        qDebug() << resizeHandle->resizeController();

    if (resizeHandle
        && resizeHandle->resizeController()
        && resizeHandle->resizeController()->isValid())
    {
        m_resizeManipulator->setHandle(resizeHandle);
        m_resizeManipulator->begin(view()->mapToScene(event->pos()));
        m_resizeIndicator.hide();
    } else {
        view()->setCursor(Qt::ArrowCursor);
    }
}

void ResizeTool::mouseMoveEvent(QMouseEvent *event)
{

    bool shouldSnapping = false; //view()->widget()->snappingAction()->isChecked();
    bool shouldSnappingAndAnchoring = false; //view()->widget()->snappingAndAnchoringAction()->isChecked();

    ResizeManipulator::Snapping useSnapping = ResizeManipulator::NoSnapping;
    if (event->modifiers().testFlag(Qt::ControlModifier) != (shouldSnapping || shouldSnappingAndAnchoring)) {
        if (shouldSnappingAndAnchoring)
            useSnapping = ResizeManipulator::UseSnappingAndAnchoring;
        else
            useSnapping = ResizeManipulator::UseSnapping;
    }
    m_resizeManipulator->update(view()->mapToScene(event->pos()), useSnapping);
}

void ResizeTool::hoverMoveEvent(QMouseEvent *event)
{
    QList<QGraphicsItem*> itemList = view()->items(event->pos());

    if (itemList.isEmpty())
        return;

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle && resizeHandle->resizeController()->isValid()) {
        m_resizeManipulator->setHandle(resizeHandle);
    } else {
        qDebug() << "resize -> selection tool";
        view()->changeTool(Constants::SelectionToolMode);
        return;
    }
}

void ResizeTool::mouseReleaseEvent(QMouseEvent *event)
{
    QList<QGraphicsItem*> itemList = view()->selectableItems(event->pos());
    if (itemList.isEmpty())
        return;

    view()->setCursor(Qt::ArrowCursor);
    m_selectionIndicator.show();
    m_resizeIndicator.show();
    m_resizeManipulator->end();
}

void ResizeTool::mouseDoubleClickEvent(QMouseEvent * /*event*/)
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

//    switch(event->key()) {
//        case Qt::Key_Left: m_resizeManipulator->moveBy(-moveStep, 0.0); break;
//        case Qt::Key_Right: m_resizeManipulator->moveBy(moveStep, 0.0); break;
//        case Qt::Key_Up: m_resizeManipulator->moveBy(0.0, -moveStep); break;
//        case Qt::Key_Down: m_resizeManipulator->moveBy(0.0, moveStep); break;
//    }

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

     if (!keyEvent->isAutoRepeat())
         m_resizeManipulator->clear();
}

void ResizeTool::wheelEvent(QWheelEvent */*event*/)
{

}

void ResizeTool::itemsAboutToRemoved(const QList<QGraphicsItem*> & /*itemList*/)
{

}

void ResizeTool::selectedItemsChanged(const QList<QGraphicsItem*> & itemList)
{
    m_selectionIndicator.setItems(toGraphicsObjectList(itemList));

    m_resizeIndicator.setItems(toGraphicsObjectList(itemList));
}

void ResizeTool::clear()
{
    view()->clearHighlightBoundingRect();
    view()->setCursor(Qt::ArrowCursor);
    m_selectionIndicator.clear();
    m_resizeIndicator.clear();
    m_resizeManipulator->clear();
}

void ResizeTool::graphicsObjectsChanged(const QList<QGraphicsObject*> &itemList)
{
    m_selectionIndicator.updateItems(itemList);
    m_resizeIndicator.updateItems(itemList);
}

}
