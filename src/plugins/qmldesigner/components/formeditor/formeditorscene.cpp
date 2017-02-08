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

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditoritem.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"


#include <QGraphicsSceneDragDropEvent>

#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>

#include <QDebug>
#include <QList>
#include <QTime>

#include <utils/algorithm.h>

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
    m_manipulatorLayerItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    m_formLayerItem->setZValue(0.0);
    m_formLayerItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
}

void FormEditorScene::resetScene()
{
    foreach (QGraphicsItem *item, m_manipulatorLayerItem->childItems()) {
       removeItem(item);
       delete item;
    }

    setSceneRect(-canvasWidth()/2., -canvasHeight()/2., canvasWidth(), canvasHeight());
}

FormEditorItem* FormEditorScene::itemForQmlItemNode(const QmlItemNode &qmlItemNode) const
{
    Q_ASSERT(qmlItemNode.isValid());
    return m_qmlItemNodeItemHash.value(qmlItemNode);
}

double FormEditorScene::canvasWidth() const
{
    return DesignerSettings::getValue(DesignerSettingsKey::CANVASWIDTH).toDouble();
}

double FormEditorScene::canvasHeight() const
{
    return DesignerSettings::getValue(DesignerSettingsKey::CANVASHEIGHT).toDouble();
}

QList<FormEditorItem*> FormEditorScene::itemsForQmlItemNodes(const QList<QmlItemNode> &nodeList) const
{
    return Utils::filtered(Utils::transform(nodeList, [this](const QmlItemNode &qmlItemNode) {
        return itemForQmlItemNode(qmlItemNode);
    }), [] (FormEditorItem* item) { return item; });
}

QList<FormEditorItem*> FormEditorScene::allFormEditorItems() const
{
    return m_qmlItemNodeItemHash.values();
}

void FormEditorScene::updateAllFormEditorItems()
{
    foreach (FormEditorItem *item, allFormEditorItems())
        item->update();
}

void FormEditorScene::removeItemFromHash(FormEditorItem *item)
{
    m_qmlItemNodeItemHash.remove(item->qmlItemNode());
}

AbstractFormEditorTool* FormEditorScene::currentTool() const
{
    return m_editorView->currentTool();
}

//This function calculates the possible parent for reparent
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

void FormEditorScene::synchronizeTransformation(FormEditorItem *item)
{
    item->updateGeometry();
    item->update();

    if (item->qmlItemNode().isRootNode()) {
        formLayerItem()->update();
        manipulatorLayerItem()->update();
    }
}

void FormEditorScene::synchronizeParent(const QmlItemNode &qmlItemNode)
{
    QmlItemNode parentNode = qmlItemNode.instanceParent().toQmlItemNode();
    reparentItem(qmlItemNode, parentNode);
}

void FormEditorScene::synchronizeOtherProperty(FormEditorItem *item, const QByteArray &propertyName)
{
    Q_ASSERT(item);
    const QmlItemNode qmlItemNode = item->qmlItemNode();
    if (propertyName == "opacity")
        item->setOpacity(qmlItemNode.instanceValue("opacity").toDouble());

    if (propertyName == "clip")
        item->setFlag(QGraphicsItem::ItemClipsChildrenToShape, qmlItemNode.instanceValue("clip").toBool());

    if (propertyName == "z")
        item->setZValue(qmlItemNode.instanceValue("z").toDouble());

    if (propertyName == "visible")
        item->setContentVisible(qmlItemNode.instanceValue("visible").toBool());
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
    currentTool()->dropEvent(removeLayerItems(itemsAt(event->scenePos())), event);

    if (views().first())
        views().first()->setFocus();
}

void FormEditorScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dragEnterEvent(removeLayerItems(itemsAt(event->scenePos())), event);
}

void FormEditorScene::dragLeaveEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dragLeaveEvent(removeLayerItems(itemsAt(event->scenePos())), event);

    return;
}

void FormEditorScene::dragMoveEvent(QGraphicsSceneDragDropEvent * event)
{
    currentTool()->dragMoveEvent(removeLayerItems(itemsAt(event->scenePos())), event);
}

QList<QGraphicsItem *> FormEditorScene::removeLayerItems(const QList<QGraphicsItem *> &itemList)
{
    QList<QGraphicsItem *> itemListWithoutLayerItems;

    foreach (QGraphicsItem *item, itemList)
        if (item != manipulatorLayerItem() && item != formLayerItem())
            itemListWithoutLayerItems.append(item);

    return itemListWithoutLayerItems;
}

QList<QGraphicsItem *> FormEditorScene::itemsAt(const QPointF &pos)
{
    QTransform transform;

    if (!views().isEmpty())
        transform = views().first()->transform();

    return items(pos,
                 Qt::IntersectsItemShape,
                 Qt::DescendingOrder,
                 transform);
}

void FormEditorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (editorView() && editorView()->model())
        currentTool()->mousePressEvent(removeLayerItems(itemsAt(event->scenePos())), event);
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

    QGraphicsScene::mouseMoveEvent(event);

    if (time.elapsed() > 30) {
        time.restart();
        if (event->buttons())
            currentTool()->mouseMoveEvent(removeLayerItems(itemsAt(event->scenePos())), event);
        else
            currentTool()->hoverMoveEvent(removeLayerItems(itemsAt(event->scenePos())), event);

        event->accept();
    }
}

void FormEditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

    if (editorView() && editorView()->model()) {
        currentTool()->mouseReleaseEvent(removeLayerItems(itemsAt(event->scenePos())), event);

        event->accept();
    }
 }

void FormEditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    if (editorView() && editorView()->model()) {
        currentTool()->mouseDoubleClickEvent(removeLayerItems(itemsAt(event->scenePos())), event);

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

void FormEditorScene::focusOutEvent(QFocusEvent *)
{
    if (currentTool())
        currentTool()->focusLost();
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
            return QGraphicsScene::event(event);
        case QEvent::GraphicsSceneHoverMove :
            hoverMoveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
            return QGraphicsScene::event(event);
        case QEvent::GraphicsSceneHoverLeave :
            hoverLeaveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
            return QGraphicsScene::event(event);
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
    if (FormEditorItem *item = itemForQmlItemNode(node)) {
        item->setParentItem(0);
        if (newParent.isValid()) {
            if (FormEditorItem *parentItem = itemForQmlItemNode(newParent))
                item->setParentItem(parentItem);
        }
    } else {
        Q_ASSERT(itemForQmlItemNode(node));
    }
}

FormEditorItem* FormEditorScene::rootFormEditorItem() const
{
    return itemForQmlItemNode(editorView()->rootModelNode());
}

void FormEditorScene::clearFormEditorItems()
{
    const QList<QGraphicsItem*> itemList(items());

    const QList<FormEditorItem*> formEditorItemsTransformed =
            Utils::transform(itemList, [](QGraphicsItem *item) { return qgraphicsitem_cast<FormEditorItem* >(item); });

    const QList<FormEditorItem*> formEditorItems = Utils::filtered(formEditorItemsTransformed,
                                                                   [](FormEditorItem *item) { return item; });
    foreach (FormEditorItem *item, formEditorItems)
            item->setParentItem(0);

    foreach (FormEditorItem *item, formEditorItems)
            delete item;
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

