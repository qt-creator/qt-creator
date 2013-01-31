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

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditoritem.h"
#include "movemanipulator.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"


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
#include <QDebug>



namespace QmlDesigner {

FormEditorScene::FormEditorScene(FormEditorWidget *view, FormEditorView *editorView)
        : QGraphicsScene(),
        m_editorView(editorView),
        m_showBoundingRects(true)
{
    setupScene();
    view->setScene(this);
    setItemIndexMethod(QGraphicsScene::NoIndex);
}

FormEditorScene::~FormEditorScene()
{
    clear();  //FormEditorItems have to be cleared before destruction
              //Reason: FormEditorItems accesses FormEditorScene in destructor
}


void FormEditorScene::setupScene()
{
    m_formLayerItem = new LayerItem(this);
    m_manipulatorLayerItem = new LayerItem(this);
    m_manipulatorLayerItem->setZValue(1.0);
    m_formLayerItem->setZValue(0.0);
    m_formLayerItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
}

void FormEditorScene::resetScene()
{
    foreach (QGraphicsItem *item, m_manipulatorLayerItem->childItems())
       removeItem(item);

    setSceneRect(-canvasWidth()/2., -canvasHeight()/2., canvasWidth(), canvasHeight());
}

FormEditorItem* FormEditorScene::itemForQmlItemNode(const QmlItemNode &qmlItemNode) const
{
    Q_ASSERT(hasItemForQmlItemNode(qmlItemNode));
    return m_qmlItemNodeItemHash.value(qmlItemNode);
}

double FormEditorScene::canvasWidth() const
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.canvasWidth;
}

double FormEditorScene::canvasHeight() const
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.canvasHeight;
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
    if (formEditorItem->qmlItemNode().isValid()) {
        QList<QGraphicsItem *> list = items(formEditorItem->qmlItemNode().instanceBoundingRect().center());
        foreach (QGraphicsItem *graphicsItem, list) {
            if (qgraphicsitem_cast<FormEditorItem*>(graphicsItem) &&
                graphicsItem->collidesWithItem(formEditorItem, Qt::ContainsItemShape))
                return qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
        }
    }

    return 0;
}

void FormEditorScene::synchronizeTransformation(const QmlItemNode &qmlItemNode)
{
    FormEditorItem *item = itemForQmlItemNode(qmlItemNode);
    item->updateGeometry();

    if (qmlItemNode.isRootNode()) {
        formLayerItem()->update();
        manipulatorLayerItem()->update();
    }
}

void FormEditorScene::synchronizeParent(const QmlItemNode &qmlItemNode)
{
    QmlItemNode parentNode = qmlItemNode.instanceParent().toQmlItemNode();
    reparentItem(qmlItemNode, parentNode);
}

void FormEditorScene::synchronizeOtherProperty(const QmlItemNode &qmlItemNode, const QString &propertyName)
{
    if (hasItemForQmlItemNode(qmlItemNode)) {
        FormEditorItem *item = itemForQmlItemNode(qmlItemNode);

        if (propertyName == "opacity")
            item->setOpacity(qmlItemNode.instanceValue("opacity").toDouble());

        if (propertyName == "clip")
            item->setFlag(QGraphicsItem::ItemClipsChildrenToShape, qmlItemNode.instanceValue("clip").toBool());

        if (propertyName == "z")
            item->setZValue(qmlItemNode.instanceValue("z").toDouble());

        if (propertyName == "visible")
            item->setContentVisible(qmlItemNode.instanceValue("visible").toBool());
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
        setSceneRect(-canvasWidth()/2., -canvasHeight()/2., canvasWidth(), canvasHeight());
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
    if (editorView() && editorView()->model())
        currentTool()->mousePressEvent(removeLayerItems(items(event->scenePos())), event);
}

static QTime staticTimer()
{
    QTime timer;
    timer.start();
    return timer;
}

void FormEditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    static QTime time = staticTimer();

    if (time.elapsed() > 30) {
        time.restart();
        if (event->buttons())
            currentTool()->mouseMoveEvent(removeLayerItems(items(event->scenePos())), event);
        else
            currentTool()->hoverMoveEvent(removeLayerItems(items(event->scenePos())), event);

        event->accept();
    }
}

void FormEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (editorView() && editorView()->model()) {
        currentTool()->mouseReleaseEvent(removeLayerItems(items(event->scenePos())), event);

        event->accept();
    }
 }

void FormEditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (editorView() && editorView()->model()) {
        currentTool()->mouseDoubleClickEvent(removeLayerItems(items(event->scenePos())), event);

        event->accept();
    }
}

void FormEditorScene::keyPressEvent(QKeyEvent *keyEvent)
{
    if (editorView() && editorView()->model())
        currentTool()->keyPressEvent(keyEvent);
}

void FormEditorScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (editorView() && editorView()->model())
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
    switch (event->type())
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
        case QEvent::ShortcutOverride :
            if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
                currentTool()->keyPressEvent(static_cast<QKeyEvent*>(event));
                return true;
            }
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
    FormEditorItem *item = itemForQmlItemNode(node);
    FormEditorItem *parentItem = 0;
    if (newParent.isValid() && hasItemForQmlItemNode(newParent))
        parentItem = itemForQmlItemNode(newParent);

    item->setParentItem(0);
    item->setParentItem(parentItem);
}

FormEditorItem* FormEditorScene::rootFormEditorItem() const
{
     if (hasItemForQmlItemNode(editorView()->rootModelNode()))
         return itemForQmlItemNode(editorView()->rootModelNode());
    return 0;
}

void FormEditorScene::clearFormEditorItems()
{
    QList<QGraphicsItem*> itemList(items());

    foreach (QGraphicsItem *item, itemList) {
        if (qgraphicsitem_cast<FormEditorItem* >(item))
            item->setParentItem(0);
    }

    foreach (QGraphicsItem *item, itemList) {
        if (qgraphicsitem_cast<FormEditorItem* >(item))
            delete item;
    }
}

void FormEditorScene::highlightBoundingRect(FormEditorItem *highlighItem)
{
    foreach (FormEditorItem *item, allFormEditorItems()) {
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

