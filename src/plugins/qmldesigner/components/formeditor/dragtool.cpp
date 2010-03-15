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

#include "dragtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "modelutilities.h"
#include "itemutilfunctions.h"
#include <customdraganddrop.h>
#include <metainfo.h>

#include "resizehandleitem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QtDebug>

namespace QmlDesigner {

DragTool::DragTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem())
{
//    view()->setCursor(Qt::SizeAllCursor);
}


DragTool::~DragTool()
{

}

void DragTool::clear()
{
    m_moveManipulator.clear();
    m_selectionIndicator.clear();
    m_movingItem.clear();
}

void DragTool::mousePressEvent(const QList<QGraphicsItem*> &,
                                            QGraphicsSceneMouseEvent *)
{

}

void DragTool::mouseMoveEvent(const QList<QGraphicsItem*> &,
                                           QGraphicsSceneMouseEvent *)
{

}

void DragTool::hoverMoveEvent(const QList<QGraphicsItem*> &,
                        QGraphicsSceneMouseEvent * /*event*/)
{

}

void DragTool::keyPressEvent(QKeyEvent *)
{

}

void DragTool::keyReleaseEvent(QKeyEvent *)
{

}


void DragTool::mouseReleaseEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                 QGraphicsSceneMouseEvent *)
{

}

void DragTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/,
                                              QGraphicsSceneMouseEvent * /*event*/)
{

}

void DragTool::itemsAboutToRemoved(const QList<FormEditorItem*> & /* removedItemList */)
{

}

void DragTool::selectedItemsChanged(const QList<FormEditorItem*> &)
{

}



void DragTool::updateMoveManipulator()
{
    if (m_moveManipulator.isActive())
        return;
}

void DragTool::beginWithPoint(const QPointF &beginPoint)
{
    m_movingItem = scene()->itemForQmlItemNode(m_dragNode);

    m_moveManipulator.setItem(m_movingItem.data());
    m_moveManipulator.begin(beginPoint);

}

void DragTool::createQmlItemNode(const ItemLibraryInfo &itemLibraryRepresentation, QmlItemNode parentNode, QPointF scenePos)
{
    QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::hide();

    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF pos = parentItem->mapFromScene(scenePos);

    m_dragNode = view()->createQmlItemNode(itemLibraryRepresentation, pos, parentNode);

    Q_ASSERT(m_dragNode.modelNode().isValid());
    Q_ASSERT(m_dragNode.isValid());

    QList<QmlItemNode> nodeList;
    nodeList.append(m_dragNode);
    view()->setSelectedQmlItemNodes(nodeList);
    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));
}

void DragTool::createQmlItemNodeFromImage(const QString &imageName, QmlItemNode parentNode, QPointF scenePos)
{
    QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::hide();

    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF pos = parentItem->mapFromScene(scenePos);

    m_dragNode = view()->createQmlItemNodeFromImage(imageName, pos, parentNode);

    QList<QmlItemNode> nodeList;
    nodeList.append(m_dragNode);
    view()->setSelectedQmlItemNodes(nodeList);
    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));
}

FormEditorItem* DragTool::calculateContainer(const QPointF &point, FormEditorItem * currentItem)
{
    QList<QGraphicsItem *> list = scene()->items(point);
    foreach (QGraphicsItem *item, list) {
         FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
         if (formEditorItem && formEditorItem != currentItem && formEditorItem->isContainer())
             return formEditorItem;
    }

    if (scene()->rootFormEditorItem()->boundingRect().adjusted(-100, -100, 100, 100).contains(point))
        return scene()->rootFormEditorItem();
    return 0;
}


void DragTool::formEditorItemsChanged(const QList<FormEditorItem*> & itemList)
{
    if (m_movingItem && itemList.contains(m_movingItem.data())) {
        QList<FormEditorItem*> updateItemList;
        updateItemList.append(m_movingItem.data());
        m_selectionIndicator.updateItems(updateItemList);
    }
}


void DragTool::dropEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        end(event->scenePos());
        //Q_ASSERT(m_token.isValid());
        m_rewriterTransaction.commit();
        m_dragNode = ModelNode();
        view()->changeToSelectionTool();
    }
}

void DragTool::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
        event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        if (!m_rewriterTransaction.isValid()) {
            m_rewriterTransaction = view()->beginRewriterTransaction();
        }
    }
}

void DragTool::dragLeaveEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        if (m_dragNode.isValid())
            m_dragNode.destroy();
        end(event->scenePos());
        m_rewriterTransaction.commit();
        QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::show();
        QList<QmlItemNode> nodeList;
        view()->setSelectedQmlItemNodes(nodeList);
        view()->changeToSelectionTool();
    }
}

static ItemLibraryInfo ItemLibraryInfoFromData(const QByteArray &data)
{
    QDataStream stream(data);

    ItemLibraryInfo itemLibraryInfo;
    stream >> itemLibraryInfo;

    return itemLibraryInfo;
}

void DragTool::dragMoveEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        QPointF scenePos = event->scenePos();
        if  (m_dragNode.isValid()) {

            FormEditorItem *parentItem = calculateContainer(event->scenePos() - QPoint(2, 2));
            if (!parentItem) {      //if there is no parent any more - the use left the scene
                end(event->scenePos());
                m_dragNode.destroy(); //delete the node then
                QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::show();
                return;
            }
            //move
            move(event->scenePos());
        } else {
            //create new node  if container

            FormEditorItem *parentItem = calculateContainer(event->scenePos());
            if (!parentItem)
                return;
            QmlItemNode parentNode; //get possible container parentNode
            if (parentItem)
                parentNode = parentItem->qmlItemNode();

            if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo")) {
                Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo").isEmpty());
                ItemLibraryInfo ItemLibraryInfo = ItemLibraryInfoFromData(event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo"));
                createQmlItemNode(ItemLibraryInfo, parentNode, event->scenePos());
            } else if (event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
                Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.libraryresource").isEmpty());
                QString imageName = QString::fromLatin1((event->mimeData()->data("application/vnd.bauhaus.libraryresource")));
                createQmlItemNodeFromImage(imageName, parentNode, event->scenePos());
            } else Q_ASSERT(false);
            beginWithPoint(event->scenePos());
        }
    }
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
    }
}

void  DragTool::end(QPointF scenePos)
{
    m_moveManipulator.end(scenePos);
    clear();
}

void  DragTool::move(QPointF scenePos)
{
    if (!m_movingItem)
        return;

    FormEditorItem *containerItem = calculateContainer(scenePos - QPoint(2, 2), m_movingItem.data());
    if (containerItem &&
       containerItem != m_movingItem->parentItem() &&
       view()->currentState().isBaseState()) {

        m_moveManipulator.reparentTo(containerItem);
    }

    //MoveManipulator::Snapping useSnapping = MoveManipulator::NoSnapping;
    MoveManipulator::Snapping useSnapping = MoveManipulator::UseSnapping;
   /* if (event->modifiers().testFlag(Qt::ControlModifier) != view()->isSnapButtonChecked())
        useSnapping = MoveManipulator::UseSnapping;*/

    m_moveManipulator.update(scenePos, useSnapping);
}


}
