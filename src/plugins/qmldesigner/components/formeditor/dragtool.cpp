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

#include "dragtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"
#include <customdraganddrop.h>
#include <metainfo.h>
#include <rewritingexception.h>

#include "resizehandleitem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
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
    m_blockMove(false),
    m_Aborted(false)
{
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

void DragTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        abort();
        event->accept();
        view()->changeToSelectionTool();
    }
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
}

static inline bool isAncestorOf(FormEditorItem *formEditorItem, FormEditorItem *newParentItem)
{
    if (formEditorItem && newParentItem)
        return formEditorItem->isAncestorOf(newParentItem);
    return false;
}

FormEditorItem* DragTool::calculateContainer(const QPointF &point, FormEditorItem * currentItem)
{
    QList<QGraphicsItem *> list = scene()->items(point);
    foreach (QGraphicsItem *item, list) {
         FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
         if (formEditorItem && formEditorItem != currentItem && formEditorItem->isContainer()
             && !isAncestorOf(currentItem, formEditorItem))
             return formEditorItem;
    }

    if (scene()->rootFormEditorItem())
        return scene()->rootFormEditorItem();
    return 0;
}

 QList<Import> DragTool::missingImportList(const ItemLibraryEntry &itemLibraryEntry)
{
    QList<Import> importToBeAddedList;

    if (!itemLibraryEntry.requiredImport().isEmpty()) {
        const QString newImportUrl = itemLibraryEntry.requiredImport();
        const QString newImportVersion = QString("%1.%2").arg(QString::number(itemLibraryEntry.majorVersion()), QString::number(itemLibraryEntry.minorVersion()));
        Import newImport = Import::createLibraryImport(newImportUrl, newImportVersion);

        if (itemLibraryEntry.majorVersion() == -1 && itemLibraryEntry.minorVersion() == -1)
            newImport = Import::createFileImport(newImportUrl, QString());
        else
            newImport = Import::createLibraryImport(newImportUrl, newImportVersion);
    }
    return importToBeAddedList;
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
    QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::hide();
}

void DragTool::instancesParentChanged(const QList<FormEditorItem *> &itemList)
{
    m_moveManipulator.synchronizeInstanceParent(itemList);
}


void DragTool::clearMoveDelay()
{
    if (!m_blockMove)
        return;
    m_blockMove = false;
    if  (m_dragNode.isValid())
        beginWithPoint(m_startPoint);
}

void DragTool::abort()
{
    if (m_Aborted)
        return;

    m_Aborted = true;

    if (m_dragNode.isValid())
        m_dragNode.destroy();

    QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::hide();

    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();

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
            view()->widget()->setFocus();
            m_Aborted = false;
            Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo").isEmpty());

            importToBeAddedList = missingImportList(
                        itemLibraryEntryFromData(event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo")));
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

    if (m_Aborted) {
        event->ignore();
        return;
    }
    if (QmlDesignerItemLibraryDragAndDrop::CustomDragAndDrop::isAnimated())
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

            FormEditorItem *parentItem = calculateContainer(scenePos);
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
                QString imageName = QString::fromUtf8((event->mimeData()->data("application/vnd.bauhaus.libraryresource")));
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
    if (containerItem && m_movingItem->parentItem() &&
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
