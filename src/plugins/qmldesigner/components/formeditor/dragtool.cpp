/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "dragtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "itemutilfunctions.h"
#include <customdraganddrop.h>
#include <metainfo.h>
#include <rewritingexception.h>

#include "resizehandleitem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QtDebug>
#include <QMessageBox>
#include <QTimer>

namespace QmlDesigner {

namespace Internal {
void TimerHandler::clearMoveDelay()
{
    m_dragTool->clearMoveDelay();
}

}

DragTool::DragTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_timerHandler(new Internal::TimerHandler(this)),
    m_blockMove(false)
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

void DragTool::createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry, QmlItemNode parentNode, QPointF scenePos)
{
    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF pos = parentItem->mapFromScene(scenePos);

    m_dragNode = view()->createQmlItemNode(itemLibraryEntry, pos, parentNode);

    Q_ASSERT(m_dragNode.modelNode().isValid());

    QList<QmlItemNode> nodeList;
    nodeList.append(m_dragNode);    
    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));

    QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::hide();
}

void DragTool::createQmlItemNodeFromImage(const QString &imageName, QmlItemNode parentNode, QPointF scenePos)
{
    if (!parentNode.isValid())
        return;

    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF pos = parentItem->mapFromScene(scenePos);

    m_dragNode = view()->createQmlItemNodeFromImage(imageName, pos, parentNode);

    QList<QmlItemNode> nodeList;
    nodeList.append(m_dragNode);
    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));

    QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::hide();
}

FormEditorItem* DragTool::calculateContainer(const QPointF &point, FormEditorItem * currentItem)
{
    QList<QGraphicsItem *> list = scene()->items(point);
    foreach (QGraphicsItem *item, list) {
         FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
         if (formEditorItem && formEditorItem != currentItem && formEditorItem->isContainer())
             return formEditorItem;
    }

    if (scene()->rootFormEditorItem() && scene()->rootFormEditorItem()->boundingRect().adjusted(-100, -100, 100, 100).contains(point))
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

void DragTool::instancesCompleted(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem* item, itemList)
        if (item->qmlItemNode() == m_dragNode)
            clearMoveDelay();
}

void DragTool::clearMoveDelay()
{
    if (!m_blockMove)
        return;
    m_blockMove = false;
    if  (m_dragNode.isValid())
        beginWithPoint(m_startPoint);
}

void DragTool::dropEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        end(event->scenePos());
        //Q_ASSERT(m_token.isValid());
        try {
            m_rewriterTransaction.commit();
        } catch (RewritingException &e) {
            QMessageBox::warning(0, "Error", e.description());
        }
        if (m_dragNode.isValid()) {
            QList<QmlItemNode> nodeList;
            nodeList.append(m_dragNode);
            view()->setSelectedQmlItemNodes(nodeList);
        }
        m_dragNode = ModelNode();
        view()->changeToSelectionTool();
    }
}

static ItemLibraryEntry itemLibraryEntryFromData(const QByteArray &data)
{
    QDataStream stream(data);

    ItemLibraryEntry itemLibraryEntry;
    stream >> itemLibraryEntry;

    return itemLibraryEntry;
}

void DragTool::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
        event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        QList<Import> importToBeAddedList;
        m_blockMove = false;
        if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo")) {
            Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo").isEmpty());
            ItemLibraryEntry itemLibraryEntry = itemLibraryEntryFromData(event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo"));
            if (!itemLibraryEntry.requiredImport().isEmpty()) {
                const QString newImportUrl = itemLibraryEntry.requiredImport();
                const QString newImportVersion = QString("%1.%2").arg(QString::number(itemLibraryEntry.majorVersion()), QString::number(itemLibraryEntry.minorVersion()));
                Import newImport = Import::createLibraryImport(newImportUrl, newImportVersion);

                if (!view()->model()->imports().contains(newImport)) {
                    importToBeAddedList.append(newImport);
                }

            }
        }

        view()->model()->changeImports(importToBeAddedList, QList<Import>());

        if (!m_rewriterTransaction.isValid()) {
            view()->clearSelectedModelNodes();
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

        try {
            m_rewriterTransaction.commit();
        } catch (RewritingException &e) {
            QMessageBox::warning(0, "Error", e.description());
        }

        QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::show();
        QList<QmlItemNode> nodeList;
        view()->setSelectedQmlItemNodes(nodeList);
        view()->changeToSelectionTool();
    }
}

void DragTool::dragMoveEvent(QGraphicsSceneDragDropEvent * event)
{
    if (m_blockMove)
        return;
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        QPointF scenePos = event->scenePos();
        if  (m_dragNode.isValid()) {

            FormEditorItem *parentItem = calculateContainer(event->scenePos() + QPoint(2, 2));
            if (!parentItem) {      //if there is no parent any more - the use left the scene
                end(event->scenePos());
                QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::show();
                m_dragNode.destroy(); //delete the node then
                return;
            }
            //move
            move(event->scenePos());
        } else {
            //create new node  if container
            if (m_dragNode.modelNode().isValid())
                return;

            FormEditorItem *parentItem = calculateContainer(event->scenePos());
            if (!parentItem)
                return;
            QmlItemNode parentNode;
            if (parentItem)
                parentNode = parentItem->qmlItemNode();

            if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo")) {
                Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo").isEmpty());
                ItemLibraryEntry itemLibraryEntry = itemLibraryEntryFromData(event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo"));
                createQmlItemNode(itemLibraryEntry, parentNode, event->scenePos());
            } else if (event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
                Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.libraryresource").isEmpty());
                QString imageName = QString::fromLatin1((event->mimeData()->data("application/vnd.bauhaus.libraryresource")));
                createQmlItemNodeFromImage(imageName, parentNode, event->scenePos());
            } else Q_ASSERT(false);
            m_blockMove = true;
            m_startPoint = event->scenePos();
            QTimer::singleShot(1000, m_timerHandler.data(), SLOT(clearMoveDelay()));
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
       containerItem != m_movingItem->parentItem()) {

        m_moveManipulator.reparentTo(containerItem);
    }

    //MoveManipulator::Snapping useSnapping = MoveManipulator::NoSnapping;
    MoveManipulator::Snapping useSnapping = MoveManipulator::UseSnapping;
   /* if (event->modifiers().testFlag(Qt::ControlModifier) != view()->isSnapButtonChecked())
        useSnapping = MoveManipulator::UseSnapping;*/

    m_moveManipulator.update(scenePos, useSnapping, MoveManipulator::UseBaseState);
}


}
