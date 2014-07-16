/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include <metainfo.h>
#include <rewritingexception.h>

#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QMimeData>
#include <QTimer>
#include <QWidget>

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
        commitTransaction();
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

void DragTool::createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry,
                                 const QmlItemNode &parentNode,
                                 const QPointF &scenePos)
{
    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF pos = parentItem->mapFromScene(scenePos);

    m_dragNode = QmlItemNode::createQmlItemNode(view(), itemLibraryEntry, pos, parentNode);

    Q_ASSERT(m_dragNode.modelNode().isValid());

    QList<QmlItemNode> nodeList;
    nodeList.append(m_dragNode);
    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));
}

void DragTool::createQmlItemNodeFromImage(const QString &imageName,
                                          const QmlItemNode &parentNode,
                                          const QPointF &scenePos)
{
    if (!parentNode.isValid())
        return;

    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF pos = parentItem->mapFromScene(scenePos);

    m_dragNode = QmlItemNode::createQmlItemNodeFromImage(view(), imageName, pos, parentNode);

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
         if (formEditorItem
                 && formEditorItem != currentItem
                 && formEditorItem->isContainer()
                 && !formEditorItem->qmlItemNode().modelNode().metaInfo().isLayoutable()
                 && !isAncestorOf(currentItem, formEditorItem))
             return formEditorItem;
    }

    if (scene()->rootFormEditorItem())
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
    m_moveManipulator.synchronizeInstanceParent(itemList);
    foreach (FormEditorItem* item, itemList)
        if (item->qmlItemNode() == m_dragNode)
            clearMoveDelay();
}

void DragTool::instancesParentChanged(const QList<FormEditorItem *> &itemList)
{
    m_moveManipulator.synchronizeInstanceParent(itemList);
}

void DragTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > & /*propertyList*/)
{
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
}

void DragTool::commitTransaction()
{
    try {
        m_rewriterTransaction.commit();
    } catch (RewritingException &e) {
        e.showException();
    }
}

void DragTool::dropEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        end(generateUseSnapping(event->modifiers()));

        commitTransaction();

        if (m_dragNode.isValid()) {
            QList<QmlItemNode> nodeList;
            nodeList.append(m_dragNode);
            view()->setSelectedModelNodes(toModelNodeList(nodeList));
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

void DragTool::dragEnterEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
        event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        QList<Import> importToBeAddedList;
        m_blockMove = false;
        if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo")) {
            view()->widgetInfo().widget->setFocus();
            m_Aborted = false;
            Q_ASSERT(!event->mimeData()->data("application/vnd.bauhaus.itemlibraryinfo").isEmpty());
        }

        if (!m_rewriterTransaction.isValid()) {
            view()->clearSelectedModelNodes();
            m_rewriterTransaction = view()->beginRewriterTransaction(QByteArrayLiteral("DragTool::dragEnterEvent"));
        }
    }
}

void DragTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();

        m_moveManipulator.end();
        clear();
        if (m_dragNode.isValid())
            m_dragNode.destroy();

        commitTransaction();

        QList<QmlItemNode> nodeList;
        view()->setSelectedModelNodes(toModelNodeList(nodeList));
        view()->changeToSelectionTool();
    }
}

void DragTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent *event)
{
    if (m_blockMove)
        return;

    if (m_Aborted) {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat("application/vnd.bauhaus.itemlibraryinfo") ||
       event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
        event->accept();
        QPointF scenePos = event->scenePos();
        if  (m_dragNode.isValid()) {

            FormEditorItem *parentItem = calculateContainer(event->scenePos() + QPoint(2, 2));
            if (!parentItem) {      //if there is no parent any more - the use left the scene
                end();
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
            QTimer::singleShot(10000, m_timerHandler.data(), SLOT(clearMoveDelay()));
        }
    }

    if (event->mimeData()->hasFormat("application/vnd.bauhaus.libraryresource")) {
    }
}

void  DragTool::end()
{
    m_moveManipulator.end();
    clear();
}

void DragTool::end(Snapper::Snapping useSnapping)
{
    m_moveManipulator.end(useSnapping);
    clear();
}

void  DragTool::move(const QPointF &scenePos)
{
    if (!m_movingItem)
        return;

    FormEditorItem *containerItem = calculateContainer(scenePos - QPoint(2, 2), m_movingItem.data());
    if (containerItem && m_movingItem->parentItem() &&
       containerItem != m_movingItem->parentItem()) {

        m_moveManipulator.reparentTo(containerItem);
    }

    //MoveManipulator::Snapping useSnapping = MoveManipulator::NoSnapping;
    Snapper::Snapping useSnapping = Snapper::UseSnapping;
   /* if (event->modifiers().testFlag(Qt::ControlModifier) != view()->isSnapButtonChecked())
        useSnapping = MoveManipulator::UseSnapping;*/

    m_moveManipulator.update(scenePos, useSnapping, MoveManipulator::UseBaseState);
}


}
