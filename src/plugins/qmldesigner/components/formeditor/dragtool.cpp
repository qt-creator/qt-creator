/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "dragtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include <metainfo.h>
#include <nodehints.h>
#include <rewritingexception.h>

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QLoggingCategory>
#include <QMimeData>
#include <QTimer>
#include <QWidget>

static Q_LOGGING_CATEGORY(dragToolInfo, "qtc.qmldesigner.formeditor");

namespace QmlDesigner {


DragTool::DragTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem()),
    m_blockMove(false),
    m_isAborted(false)
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

void DragTool::mousePressEvent(const QList<QGraphicsItem*> &, QGraphicsSceneMouseEvent *)
{
}

void DragTool::mouseMoveEvent(const QList<QGraphicsItem*> &, QGraphicsSceneMouseEvent *)
{
}

void DragTool::hoverMoveEvent(const QList<QGraphicsItem*> &, QGraphicsSceneMouseEvent * /*event*/)
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


void DragTool::mouseReleaseEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneMouseEvent *)
{
}

void DragTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/, QGraphicsSceneMouseEvent * /*event*/)
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
}

void DragTool::beginWithPoint(const QPointF &beginPoint)
{
    m_movingItem = scene()->itemForQmlItemNode(m_dragNode);

    m_moveManipulator.setItem(m_movingItem.data());
    m_moveManipulator.begin(beginPoint);
}

void DragTool::createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry,
                                 const QmlItemNode &parentNode,
                                 const QPointF &scenePosition)
{
    MetaInfo metaInfo = MetaInfo::global();

    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    QPointF positonInItemSpace = parentItem->qmlItemNode().instanceSceneContentItemTransform().inverted().map(scenePosition);

    m_dragNode = QmlItemNode::createQmlItemNode(view(), itemLibraryEntry, positonInItemSpace, parentNode);

    QList<QmlItemNode> nodeList;
    nodeList.append(m_dragNode);
    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));
}

void DragTool::createQmlItemNodeFromImage(const QString &imageName,
                                          const QmlItemNode &parentNode,
                                          const QPointF &scenePosition)
{
    if (parentNode.isValid()) {
        MetaInfo metaInfo = MetaInfo::global();

        FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
        QPointF positonInItemSpace = parentItem->qmlItemNode().instanceSceneContentItemTransform().inverted().map(scenePosition);

        m_dragNode = QmlItemNode::createQmlItemNodeFromImage(view(), imageName, positonInItemSpace, parentNode);

        QList<QmlItemNode> nodeList;
        nodeList.append(m_dragNode);
        m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(nodeList));
    }
}

FormEditorItem* DragTool::targetContainerOrRootItem(const QList<QGraphicsItem*> &itemList, FormEditorItem * currentItem)
{
    FormEditorItem *formEditorItem = containerFormEditorItem(itemList, QList<FormEditorItem*>() << currentItem);

    if (!formEditorItem)
        formEditorItem = scene()->rootFormEditorItem();

    return formEditorItem;
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
    if (m_blockMove) {
        m_blockMove = false;
        if (m_dragNode.isValid())
            beginWithPoint(m_startPoint);
    }
}

void DragTool::focusLost()
{
}

void DragTool::abort()
{
    if (!m_isAborted) {
        m_isAborted = true;

        if (m_dragNode.isValid())
            m_dragNode.destroy();
    }
}

static ItemLibraryEntry itemLibraryEntryFromMimeData(const QMimeData *mimeData)
{
    QByteArray data = mimeData->data(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"));

    QDataStream stream(data);

    ItemLibraryEntry itemLibraryEntry;
    stream >> itemLibraryEntry;

    return itemLibraryEntry;
}

static bool canBeDropped(const QMimeData *mimeData)
{
    return NodeHints::fromItemLibraryEntry(itemLibraryEntryFromMimeData(mimeData)).canBeDroppedInFormEditor();
}

static bool canHandleMimeData(const QMimeData *mimeData)
{
    return mimeData->hasFormat(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"))
          || mimeData->hasFormat(QStringLiteral("application/vnd.bauhaus.libraryresource"));
}

static bool dragAndDropPossible(const QMimeData *mimeData)
{
    return canHandleMimeData(mimeData) && canBeDropped(mimeData);
}

static bool hasItemLibraryInfo(const QMimeData *mimeData)
{
    return mimeData->hasFormat(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"));
}

static bool hasLibraryResources(const QMimeData *mimeData)
{
    return mimeData->hasFormat(QStringLiteral("application/vnd.bauhaus.libraryresource"));
}

void DragTool::dropEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent *event)
{
    if (dragAndDropPossible(event->mimeData())) {
        event->accept();
        end(generateUseSnapping(event->modifiers()));

        commitTransaction();

        if (m_dragNode.isValid())
            view()->setSelectedModelNode(m_dragNode);


        m_dragNode = QmlItemNode();

        view()->changeToSelectionTool();
    }
}

void DragTool::dragEnterEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent *event)
{
    if (dragAndDropPossible(event->mimeData())) {
        m_blockMove = false;

        if (hasItemLibraryInfo(event->mimeData())) {

            view()->widgetInfo().widget->setFocus();
            m_isAborted = false;
        }

        if (!m_rewriterTransaction.isValid()) {
            view()->clearSelectedModelNodes();
            m_rewriterTransaction = view()->beginRewriterTransaction(QByteArrayLiteral("DragTool::dragEnterEvent"));
        }
    }
}

void DragTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent *event)
{
    if (dragAndDropPossible(event->mimeData())) {
        event->accept();

        m_moveManipulator.end();
        clear();
        if (m_dragNode.isValid())
            m_dragNode.destroy();

        commitTransaction();

        view()->changeToSelectionTool();
    }
}

static QString libraryResourceImageName(const QMimeData *mimeData)
{
   return QString::fromUtf8((mimeData->data(QStringLiteral("application/vnd.bauhaus.libraryresource"))));
}

void DragTool::createDragNode(const QMimeData *mimeData, const QPointF &scenePosition, const QList<QGraphicsItem*> &itemList)
{
    if (!m_dragNode.hasModelNode()) {
        FormEditorItem *targetContainerFormEditorItem = targetContainerOrRootItem(itemList);
        if (targetContainerFormEditorItem) {
            QmlItemNode targetContainerQmlItemNode;
            if (targetContainerFormEditorItem)
                targetContainerQmlItemNode = targetContainerFormEditorItem->qmlItemNode();

            if (hasItemLibraryInfo(mimeData))
                createQmlItemNode(itemLibraryEntryFromMimeData(mimeData), targetContainerQmlItemNode, scenePosition);
            else if (hasLibraryResources(mimeData))
                createQmlItemNodeFromImage(libraryResourceImageName(mimeData), targetContainerQmlItemNode, scenePosition);

            m_blockMove = true;
            m_startPoint = scenePosition;
        }
    }
}

void DragTool::dragMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event)
{
    if (!m_blockMove && !m_isAborted && dragAndDropPossible(event->mimeData())) {
        event->accept();
        if (m_dragNode.isValid()) {
            FormEditorItem *targetContainerItem = targetContainerOrRootItem(itemList);
            if (targetContainerItem) {
                move(event->scenePos(), itemList);
            } else {
                end();
                m_dragNode.destroy();
            }
        } else {
            createDragNode(event->mimeData(), event->scenePos(), itemList);
        }
    } else{
        event->ignore();
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

void  DragTool::move(const QPointF &scenePosition, const QList<QGraphicsItem*> &itemList)
{
    if (m_movingItem) {
        FormEditorItem *containerItem = targetContainerOrRootItem(itemList, m_movingItem.data());
        if (containerItem && m_movingItem->parentItem() &&
                containerItem != m_movingItem->parentItem()) {

            const QmlItemNode movingNode = m_movingItem->qmlItemNode();
            const QmlItemNode containerNode = containerItem->qmlItemNode();

            qCInfo(dragToolInfo()) << Q_FUNC_INFO << movingNode << containerNode << movingNode.canBereparentedTo(containerNode);

            if (movingNode.canBereparentedTo(containerNode))
                m_moveManipulator.reparentTo(containerItem);
        }

        Snapper::Snapping useSnapping = Snapper::UseSnapping;

        m_moveManipulator.update(scenePosition, useSnapping, MoveManipulator::UseBaseState);
    }
}

void DragTool::commitTransaction()
{
    try {
        m_rewriterTransaction.commit();
    } catch (const RewritingException &e) {
        e.showException();
    }
}

}
