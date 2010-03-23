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

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditoritem.h"
#include "movemanipulator.h"
#include <metainfo.h>
#include <QGraphicsSceneDragDropEvent>

#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>

#include <QApplication>
#include <QDebug>
#include <QList>

#include "formeditornodeinstanceview.h"

#include "resizehandleitem.h"
#include <QtDebug>



namespace QmlDesigner {

FormEditorScene::FormEditorScene(FormEditorWidget *view, FormEditorView *editorView)
        : QGraphicsScene(),
        m_editorView(editorView),
        m_formLayerItem(new LayerItem(this)),
        m_manipulatorLayerItem(new LayerItem(this)),
        m_paintMode(NormalMode),
        m_showBoundingRects(true)
{
    setSceneRect(0, 0, 1, 1); // prevent automatic calculation (causing a recursion), right size will be set later

    m_manipulatorLayerItem->setZValue(1.0);
    m_formLayerItem->setZValue(0.0);
    m_formLayerItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    view->setScene(this);

//    setItemIndexMethod(QGraphicsScene::NoIndex);
}

FormEditorScene::~FormEditorScene()
{
    clear();  //FormEditorItems have to be cleared before destruction
              //Reason: FormEditorItems accesses FormEditorScene in destructor
}


FormEditorItem* FormEditorScene::itemForQmlItemNode(const QmlItemNode &qmlItemNode) const
{
    Q_ASSERT(hasItemForQmlItemNode(qmlItemNode));
    return m_qmlItemNodeItemHash.value(qmlItemNode);
}


QList<FormEditorItem*> FormEditorScene::itemsForQmlItemNodes(const QList<QmlItemNode> &nodeList) const
{
    QList<FormEditorItem*> itemList;
    foreach (const QmlItemNode &node, nodeList) {
        if (hasItemForQmlItemNode(node))
            itemList += itemForQmlItemNode(node);
    }

    return itemList;
}

QList<FormEditorItem*> FormEditorScene::allFormEditorItems() const
{
    return m_qmlItemNodeItemHash.values();
}

void FormEditorScene::updateAllFormEditorItems()
{
    foreach (FormEditorItem *item, allFormEditorItems()) {
        item->update();
    }
}

bool FormEditorScene::hasItemForQmlItemNode(const QmlItemNode &qmlItemNode) const
{
    return m_qmlItemNodeItemHash.contains(qmlItemNode);
}

void FormEditorScene::removeItemFromHash(FormEditorItem *item)
{
   m_qmlItemNodeItemHash.remove(item->qmlItemNode());
}


AbstractFormEditorTool* FormEditorScene::currentTool() const
{
    return m_editorView->currentTool();
}

//This method calculates the possible parent for reparent
FormEditorItem* FormEditorScene::calulateNewParent(FormEditorItem *formEditorItem)
{
    QList<QGraphicsItem *> list = items(formEditorItem->qmlItemNode().instanceBoundingRect().center());
    foreach (QGraphicsItem *graphicsItem, list) {
        if (qgraphicsitem_cast<FormEditorItem*>(graphicsItem) &&
            graphicsItem->collidesWithItem(formEditorItem, Qt::ContainsItemShape))
            return qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
    }
    return 0;
}

void FormEditorScene::synchronizeTransformation(const QmlItemNode &qmlItemNode)
{
    FormEditorItem *item = itemForQmlItemNode(qmlItemNode);
    item->updateGeometry();
    item->update();

    if (qmlItemNode.isRootNode()) {
        QRectF sceneRect(qmlItemNode.instanceBoundingRect());

        setSceneRect(sceneRect);
        formLayerItem()->update();
        manipulatorLayerItem()->update();
    }
}

void FormEditorScene::synchronizeParent(const QmlItemNode &qmlItemNode)
{
    QmlItemNode parentNode = qmlItemNode.instanceParent().toQmlItemNode();
    if (parentNode.isValid())
        reparentItem(qmlItemNode, parentNode);
}

void FormEditorScene::synchronizeOtherProperty(const QmlItemNode &qmlItemNode, const QString &propertyName)
{
    if (hasItemForQmlItemNode(qmlItemNode)) {
        FormEditorItem *item = itemForQmlItemNode(qmlItemNode);

        if (propertyName == "opacity")
            item->setOpacity(qmlItemNode.instanceValue("opacity").toDouble());

        if (item)
            item->update();
    }
}

void FormEditorScene::synchronizeState(const QmlItemNode &qmlItemNode)
{
    if (hasItemForQmlItemNode(qmlItemNode)) {
        FormEditorItem *item = itemForQmlItemNode(qmlItemNode);
        if (item)
            item->update();
    }
}


FormEditorItem *FormEditorScene::addFormEditorItem(const QmlItemNode &qmlItemNode)
{
    FormEditorItem *formEditorItem = new FormEditorItem(qmlItemNode, this);
    Q_ASSERT(!m_qmlItemNodeItemHash.contains(qmlItemNode));

    m_qmlItemNodeItemHash.insert(qmlItemNode, formEditorItem);
    if (qmlItemNode.isRootNode()) {
        QRectF sceneRect(qmlItemNode.instanceBoundingRect());

        setSceneRect(sceneRect);
       formLayerItem()->update();
       manipulatorLayerItem()->update();
    }

    return formEditorItem;
}


void FormEditorScene::dropEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dropEvent(event);

    if (views().first())
        views().first()->setFocus();
}

void FormEditorScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dragEnterEvent(event);
}

void FormEditorScene::dragLeaveEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dragLeaveEvent(event);

    return;

     if  (m_dragNode.isValid()) {
         m_dragNode.destroy();
     }
}

void FormEditorScene::dragMoveEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dragMoveEvent(event);
}

QList<QGraphicsItem *> FormEditorScene::removeLayerItems(const QList<QGraphicsItem *> &itemList)
{
    QList<QGraphicsItem *> itemListWithoutLayerItems;

    foreach (QGraphicsItem *item, itemList)
        if (item != manipulatorLayerItem() && item != formLayerItem())
            itemListWithoutLayerItems.append(item);

    return itemListWithoutLayerItems;
}

void FormEditorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    currentTool()->mousePressEvent(removeLayerItems(items(event->scenePos())), event);
}

void FormEditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons())
        currentTool()->mouseMoveEvent(removeLayerItems(items(event->scenePos())), event);
    else
        currentTool()->hoverMoveEvent(removeLayerItems(items(event->scenePos())), event);

    event->accept();
}

void FormEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    currentTool()->mouseReleaseEvent(removeLayerItems(items(event->scenePos())), event);

    event->accept();
 }

void FormEditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    currentTool()->mouseDoubleClickEvent(removeLayerItems(items(event->scenePos())), event);

    event->accept();
}

void FormEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
    currentTool()->keyPressEvent(keyEvent);
}

void FormEditorScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    currentTool()->keyReleaseEvent(keyEvent);
}

FormEditorView *FormEditorScene::editorView() const
{
   return m_editorView;
}

LayerItem* FormEditorScene::manipulatorLayerItem() const
{
    return m_manipulatorLayerItem.data();
}

LayerItem* FormEditorScene::formLayerItem() const
{
    return m_formLayerItem.data();
}

bool FormEditorScene::event(QEvent * event)
{
    switch(event->type())
    {
        case QEvent::GraphicsSceneHoverEnter :
            hoverEnterEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
            return true;
        case QEvent::GraphicsSceneHoverMove :
            hoverMoveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
            return true;
        case QEvent::GraphicsSceneHoverLeave :
            hoverLeaveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
            return true;
        default: return QGraphicsScene::event(event);
    }

}

void FormEditorScene::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    qDebug() << __FUNCTION__;
}

void FormEditorScene::hoverMoveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    qDebug() << __FUNCTION__;
}

void FormEditorScene::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    qDebug() << __FUNCTION__;
}


void FormEditorScene::reparentItem(const QmlItemNode &node, const QmlItemNode &newParent)
{
    Q_ASSERT(hasItemForQmlItemNode(node));
    Q_ASSERT(hasItemForQmlItemNode(newParent));
    FormEditorItem *item = itemForQmlItemNode(node);
    FormEditorItem *parentItem = itemForQmlItemNode(newParent);
    if (item->parentItem() != parentItem) {
        item->setParentItem(parentItem);
        item->update();
    }
}

FormEditorItem* FormEditorScene::rootFormEditorItem() const
{
    QList<QGraphicsItem*> childItemList(m_formLayerItem->childItems());
    if (!childItemList.isEmpty())
        return FormEditorItem::fromQGraphicsItem(childItemList.first());

    return 0;
}

FormEditorScene::PaintMode FormEditorScene::paintMode() const
{
    return m_paintMode;
}

void FormEditorScene::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void FormEditorScene::clearFormEditorItems()
{
    foreach (QGraphicsItem *item, items()) {
        if (qgraphicsitem_cast<FormEditorItem* >(item))
            delete item;
    }
}

void FormEditorScene::highlightBoundingRect(FormEditorItem *highlighItem)
{
    foreach(FormEditorItem *item, allFormEditorItems()) {
        if (item == highlighItem)
            item->setHighlightBoundingRect(true);
        else
            item->setHighlightBoundingRect(false);
    }
}

void FormEditorScene::setShowBoundingRects(bool show)
{
    m_showBoundingRects = show;
    updateAllFormEditorItems();
}

bool FormEditorScene::showBoundingRects() const
{
    return m_showBoundingRects;
}

}

