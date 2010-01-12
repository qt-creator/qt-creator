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

#include "selectiontool.h"
#include "formeditorscene.h"
#include "formeditorview.h"

#include "resizehandleitem.h"


#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QtDebug>
#include <QClipboard>

namespace QmlDesigner {

static const int s_startDragDistance = 20;
static const int s_startDragTime = 50;

SelectionTool::SelectionTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_rubberbandSelectionManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_singleSelectionManipulator(editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_resizeIndicator(editorView->scene()->manipulatorLayerItem())

{
//    view()->setCursor(Qt::CrossCursor);
}


SelectionTool::~SelectionTool()
{
}

void SelectionTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                    QGraphicsSceneMouseEvent *event)
{
    m_mousePressTimer.start();
    FormEditorItem* formEditorItem = topFormEditorItem(itemList);
    if (formEditorItem && !formEditorItem->qmlItemNode().hasChildren()) {
        m_singleSelectionManipulator.begin(event->scenePos());

        if (event->modifiers().testFlag(Qt::ControlModifier))
            m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
        else if (event->modifiers().testFlag(Qt::ShiftModifier))
            m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
        else
            m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);
    } else {
        if (event->modifiers().testFlag(Qt::AltModifier)) {
            m_singleSelectionManipulator.begin(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);

            m_singleSelectionManipulator.end(event->scenePos());
            view()->changeToMoveTool(event->scenePos());
        } else {
            m_rubberbandSelectionManipulator.begin(event->scenePos());
        }
    }
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
                        QGraphicsSceneMouseEvent * /*event*/)
{
    if (itemList.isEmpty())
        return;

    ResizeHandleItem* resizeHandle = ResizeHandleItem::fromGraphicsItem(itemList.first());
    if (resizeHandle) {
        view()->changeToResizeTool();
        return;
    }

    if (topSelectedItemIsMovable(itemList))
        view()->changeToMoveTool();
}

void SelectionTool::mouseReleaseEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                      QGraphicsSceneMouseEvent *event)
{
    if (m_singleSelectionManipulator.isActive()) {
        m_singleSelectionManipulator.end(event->scenePos());
    }
    else if (m_rubberbandSelectionManipulator.isActive()) {

        QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
        if (mouseMovementVector.toPoint().manhattanLength() < s_startDragDistance) {
            m_singleSelectionManipulator.begin(event->scenePos());

            if (event->modifiers().testFlag(Qt::ControlModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::RemoveFromSelection);
            else if (event->modifiers().testFlag(Qt::ShiftModifier))
                m_singleSelectionManipulator.select(SingleSelectionManipulator::AddToSelection);
            else
                m_singleSelectionManipulator.select(SingleSelectionManipulator::InvertSelection);

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

void SelectionTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                          QGraphicsSceneMouseEvent * /*event*/)
{

}

void SelectionTool::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            view()->changeToMoveTool();
            view()->currentTool()->keyPressEvent(event);
            break;
        case Qt::Key_C:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                copySelectedNodeToClipBoard();
                break;
            }
        case Qt::Key_V:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                pasteClipBoard();
                break;
            }
        case Qt::Key_X:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                cutSelectedNodeToClipBoard();
                break;
            }
    }
}

void SelectionTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{

}

void SelectionTool::copySelectedNodeToClipBoard()
{
    // QClipboard *clipboard = QApplication::clipboard();
}

void SelectionTool::cutSelectedNodeToClipBoard()
{
    // QClipboard *clipboard = QApplication::clipboard();
}

void SelectionTool::pasteClipBoard()
{
    // QClipboard *clipboard = QApplication::clipboard();
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
}

void SelectionTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.setItems(itemList);
    m_resizeIndicator.setItems(itemList);
}

void SelectionTool::formEditorItemsChanged(const QList<FormEditorItem*> &itemList)
{
    m_selectionIndicator.updateItems(itemList);
    m_resizeIndicator.updateItems(itemList);
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
