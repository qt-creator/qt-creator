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

#include "abstractformeditortool.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditorscene.h"

#include <modelnodecontextmenu.h>

#include <QDebug>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <nodemetainfo.h>
#include <QAction>

namespace QmlDesigner {

AbstractFormEditorTool::AbstractFormEditorTool(FormEditorView *editorView) : m_view(editorView)
{
}


AbstractFormEditorTool::~AbstractFormEditorTool()
{

}

FormEditorView* AbstractFormEditorTool::view() const
{
    return m_view;
}

void AbstractFormEditorTool::setView(FormEditorView *view)
{
    m_view = view;
}

FormEditorScene* AbstractFormEditorTool::scene() const
{
    return view()->scene();
}

void AbstractFormEditorTool::setItems(const QList<FormEditorItem*> &itemList)
{
    m_itemList = itemList;
    selectedItemsChanged(m_itemList);
}

QList<FormEditorItem*> AbstractFormEditorTool::items() const
{
    return m_itemList;
}

QList<FormEditorItem *> AbstractFormEditorTool::toFormEditorItemList(const QList<QGraphicsItem *> &itemList)
{
    QList<FormEditorItem *> formEditorItemList;

    foreach (QGraphicsItem *graphicsItem, itemList) {
        FormEditorItem *formEditorItem = qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
        if (formEditorItem)
            formEditorItemList.append(formEditorItem);
    }

    return formEditorItemList;
}

bool AbstractFormEditorTool::topItemIsMovable(const QList<QGraphicsItem*> & itemList)
{
    QGraphicsItem *firstSelectableItem = topMovableGraphicsItem(itemList);
    if (firstSelectableItem == 0)
        return false;

    FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(firstSelectableItem);
    QList<ModelNode> selectedNodes = view()->selectedModelNodes();

    if (formEditorItem != 0
       && selectedNodes.contains(formEditorItem->qmlItemNode()))
        return true;

    return false;

}

bool AbstractFormEditorTool::topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList)
{
    QList<ModelNode> selectedNodes = view()->selectedModelNodes();

    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
            && selectedNodes.contains(formEditorItem->qmlItemNode())
            && formEditorItem->qmlItemNode().instanceIsMovable()
            && formEditorItem->qmlItemNode().modelIsMovable()
            && !formEditorItem->qmlItemNode().instanceIsInLayoutable()
            && (formEditorItem->qmlItemNode().instanceHasShowContent()))
            return true;
    }

    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
            && formEditorItem->qmlItemNode().isValid()
            && formEditorItem->qmlItemNode().instanceIsMovable()
            && formEditorItem->qmlItemNode().modelIsMovable()
            && !formEditorItem->qmlItemNode().instanceIsInLayoutable()
            && selectedNodes.contains(formEditorItem->qmlItemNode()))
            return true;
    }

    return false;

}

bool AbstractFormEditorTool::topItemIsResizeHandle(const QList<QGraphicsItem*> &/*itemList*/)
{
    return false;
}

QGraphicsItem *AbstractFormEditorTool::topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        if (item->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return item;
    }

    return 0;
}

FormEditorItem *AbstractFormEditorTool::topMovableFormEditorItem(const QList<QGraphicsItem*> &itemList, bool selectOnlyContentItems)
{
    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
                && formEditorItem->qmlItemNode().isValid()
                && !formEditorItem->qmlItemNode().instanceIsInLayoutable()
                && formEditorItem->qmlItemNode().instanceIsMovable()
                && formEditorItem->qmlItemNode().modelIsMovable()
                && (formEditorItem->qmlItemNode().instanceHasShowContent() || !selectOnlyContentItems))
            return formEditorItem;
    }

    return 0;
}

FormEditorItem* AbstractFormEditorTool::nearestFormEditorItem(const QPointF &point, const QList<QGraphicsItem*> & itemList)
{
    FormEditorItem* nearestItem = 0;
    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (!formEditorItem || !formEditorItem->qmlItemNode().isValid())
            continue;

        if (formEditorItem->parentItem() && !formEditorItem->parentItem()->isContentVisible())
            continue;

        if (!nearestItem)
            nearestItem = formEditorItem;
        else if (formEditorItem->selectionWeigth(point, 1) < nearestItem->selectionWeigth(point, 0))
            nearestItem = formEditorItem;
    }

    if (nearestItem && nearestItem->qmlItemNode().isInStackedContainer())
        return nearestItem->parentItem();

    return nearestItem;
}

QList<FormEditorItem *> AbstractFormEditorTool::filterSelectedModelNodes(const QList<FormEditorItem *> &itemList) const
{
    QList<FormEditorItem *> filteredItemList;

    foreach (FormEditorItem *item, itemList) {
        if (view()->isSelectedModelNode(item->qmlItemNode()))
            filteredItemList.append(item);
    }

    return filteredItemList;
}

void AbstractFormEditorTool::dropEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /* event */)
{
}

void AbstractFormEditorTool::dragEnterEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat(QLatin1String("application/vnd.bauhaus.itemlibraryinfo")) ||
        event->mimeData()->hasFormat(QLatin1String("application/vnd.bauhaus.libraryresource"))) {
        event->accept();
        view()->changeToDragTool();
        view()->currentTool()->dragEnterEvent(itemList, event);
    } else {
        event->ignore();
    }
}

void AbstractFormEditorTool::mousePressEvent(const QList<QGraphicsItem*> & /*itemList*/, QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        event->accept();
}

static bool containsItemNode(const QList<QGraphicsItem*> & itemList, const QmlItemNode &itemNode)
{
    foreach (QGraphicsItem *item, itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem && formEditorItem->qmlItemNode() == itemNode)
            return true;
    }

    return false;
}

void AbstractFormEditorTool::mouseReleaseEvent(const QList<QGraphicsItem*> & itemList, QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {

        QmlItemNode currentSelectedNode;

        if (view()->selectedModelNodes().count() == 1) {
            currentSelectedNode = view()->selectedModelNodes().first();

            if (!containsItemNode(itemList, currentSelectedNode)) {
                QmlItemNode selectedNode;

                FormEditorItem *formEditorItem = nearestFormEditorItem(event->scenePos(), itemList);

                if (formEditorItem && formEditorItem->qmlItemNode().isValid())
                    selectedNode = formEditorItem->qmlItemNode();

                if (selectedNode.isValid()) {
                    QList<ModelNode> nodeList;
                    nodeList.append(selectedNode);

                    view()->setSelectedModelNodes(nodeList);
                }
            }
        }

        showContextMenu(event);
        event->accept();
    }
}

void AbstractFormEditorTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        FormEditorItem *formEditorItem = nearestFormEditorItem(event->pos(), itemList);
        if (formEditorItem) {
            view()->setSelectedModelNode(formEditorItem->qmlItemNode().modelNode());
            view()->changeToCustomTool();
        }
    }
}

void AbstractFormEditorTool::showContextMenu(QGraphicsSceneMouseEvent *event)
{
    ModelNodeContextMenu::showContextMenu(view(), event->screenPos(), event->scenePos().toPoint(), true);
}

Snapper::Snapping AbstractFormEditorTool::generateUseSnapping(Qt::KeyboardModifiers keyboardModifier) const
{
    bool shouldSnapping = view()->formEditorWidget()->snappingAction()->isChecked();
    bool shouldSnappingAndAnchoring = view()->formEditorWidget()->snappingAndAnchoringAction()->isChecked();

    Snapper::Snapping useSnapping = Snapper::NoSnapping;
    if (keyboardModifier.testFlag(Qt::ControlModifier) != (shouldSnapping || shouldSnappingAndAnchoring)) {
        if (shouldSnappingAndAnchoring)
            useSnapping = Snapper::UseSnappingAndAnchoring;
        else
            useSnapping = Snapper::UseSnapping;
    }

    return useSnapping;
}

static bool isNotAncestorOfItemInList(FormEditorItem *formEditorItem, const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (item
            && item->qmlItemNode().isValid()
            && item->qmlItemNode().isAncestorOf(formEditorItem->qmlItemNode()))
            return false;
    }

    return true;
}

FormEditorItem *AbstractFormEditorTool::containerFormEditorItem(const QList<QGraphicsItem *> &itemUnderMouseList, const QList<FormEditorItem *> &selectedItemList) const
{
    foreach (QGraphicsItem* item, itemUnderMouseList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
                && !selectedItemList.contains(formEditorItem)
                && isNotAncestorOfItemInList(formEditorItem, selectedItemList)
                && formEditorItem->isContainer()
                && formEditorItem->isContentVisible())
            return formEditorItem;
    }

    return 0;
}

void AbstractFormEditorTool::clear()
{
    m_itemList.clear();
}
}
