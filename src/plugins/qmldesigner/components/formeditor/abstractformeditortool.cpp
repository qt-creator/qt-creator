// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractformeditortool.h"
#include "assetslibrarywidget.h"
#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "modelnodecontextmenu.h"
#include "qmldesignerconstants.h"

#include <model/modelutils.h>

#include <QDebug>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <nodemetainfo.h>
#include <QAction>

namespace QmlDesigner {

AbstractFormEditorTool::AbstractFormEditorTool(FormEditorView *editorView) : m_view(editorView)
{
}

AbstractFormEditorTool::~AbstractFormEditorTool() = default;

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

    for (QGraphicsItem *graphicsItem : itemList) {
        auto formEditorItem = qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
        if (formEditorItem)
            formEditorItemList.append(formEditorItem);
    }

    return formEditorItemList;
}

bool AbstractFormEditorTool::topItemIsMovable(const QList<QGraphicsItem*> & itemList)
{
    QGraphicsItem *firstSelectableItem = topMovableGraphicsItem(itemList);
    if (firstSelectableItem == nullptr)
        return false;

    FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(firstSelectableItem);
    QList<ModelNode> selectedNodes = view()->selectedModelNodes();

    if (formEditorItem != nullptr
       && selectedNodes.contains(formEditorItem->qmlItemNode()))
        return true;

    return false;

}

bool AbstractFormEditorTool::topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList)
{
    QList<ModelNode> selectedNodes = view()->selectedModelNodes();

    for (QGraphicsItem *item : itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
            && selectedNodes.contains(formEditorItem->qmlItemNode())
            && formEditorItem->qmlItemNode().instanceIsMovable()
            && formEditorItem->qmlItemNode().modelIsMovable()
            && !formEditorItem->qmlItemNode().instanceIsInLayoutable()
            && (formEditorItem->qmlItemNode().instanceHasShowContent()))
            return true;
    }

    for (QGraphicsItem *item : itemList) {
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

bool AbstractFormEditorTool::selectedItemCursorInMovableArea(const QPointF &pos)
{
    if (!view()->hasSingleSelectedModelNode())
        return false;

    const ModelNode selectedNode = view()->singleSelectedModelNode();

    FormEditorItem *item = scene()->itemForQmlItemNode(selectedNode);

    if (!item)
        return false;

     if (!topSelectedItemIsMovable({item}))
         return false;

    const QPolygonF boundingRectInSceneSpace(item->mapToScene(item->qmlItemNode().instanceBoundingRect()));
    QRectF boundingRect = boundingRectInSceneSpace.boundingRect();
    QRectF innerRect = boundingRect;

    innerRect.adjust(2, 2, -2, -2);
    const int heightOffset = -20 / scene()->formLayerItem()->viewportTransform().m11();
    boundingRect.adjust(-2, heightOffset, 2, 2);

    return !innerRect.contains(pos) && boundingRect.contains(pos);
}

bool AbstractFormEditorTool::topItemIsResizeHandle(const QList<QGraphicsItem*> &/*itemList*/)
{
    return false;
}

QGraphicsItem *AbstractFormEditorTool::topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList)
{
    for (QGraphicsItem *item : itemList) {
        if (item->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return item;
    }

    return nullptr;
}

FormEditorItem *AbstractFormEditorTool::topMovableFormEditorItem(const QList<QGraphicsItem*> &itemList, bool selectOnlyContentItems)
{
    for (QGraphicsItem *item : itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
                && formEditorItem->qmlItemNode().isValid()
                && !formEditorItem->qmlItemNode().instanceIsInLayoutable()
                && formEditorItem->qmlItemNode().instanceIsMovable()
                && formEditorItem->qmlItemNode().modelIsMovable()
                && (formEditorItem->qmlItemNode().instanceHasShowContent() || !selectOnlyContentItems))
            return formEditorItem;
    }

    return nullptr;
}

FormEditorItem* AbstractFormEditorTool::nearestFormEditorItem(const QPointF &point, const QList<QGraphicsItem*> &itemList)
{
    FormEditorItem* nearestItem = nullptr;
    for (QGraphicsItem *item : itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (formEditorItem && formEditorItem->flowHitTest(point))
            return formEditorItem;

        if (!formEditorItem || !formEditorItem->qmlItemNode().isValid())
            continue;

        if (formEditorItem->parentItem() && !formEditorItem->parentItem()->isContentVisible())
            continue;

        if (formEditorItem && ModelUtils::isThisOrAncestorLocked(formEditorItem->qmlItemNode().modelNode()))
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

    for (FormEditorItem *item : itemList) {
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
    bool hasValidAssets = false;
    if (event->mimeData()->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        const QStringList assetPaths = QString::fromUtf8(event->mimeData()
                                ->data(Constants::MIME_TYPE_ASSETS)).split(',');
        for (const QString &assetPath : assetPaths) {
            QString assetType = AssetsLibraryWidget::getAssetTypeAndData(assetPath).first;
            if (assetType == Constants::MIME_TYPE_ASSET_IMAGE
             || assetType == Constants::MIME_TYPE_ASSET_FONT
             || assetType == Constants::MIME_TYPE_ASSET_EFFECT) {
                hasValidAssets = true;
                break;
            }
        }
    }

    if (event->mimeData()->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO) || hasValidAssets) {
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
    for (QGraphicsItem *item : itemList) {
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

        if (view()->selectedModelNodes().size() == 1) {
            currentSelectedNode = view()->selectedModelNodes().constFirst();

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
    for (FormEditorItem *item : itemList) {
        if (item
            && item->qmlItemNode().isValid()
            && item->qmlItemNode().isAncestorOf(formEditorItem->qmlItemNode()))
            return false;
    }

    return true;
}

FormEditorItem *AbstractFormEditorTool::containerFormEditorItem(const QList<QGraphicsItem *> &itemUnderMouseList, const QList<FormEditorItem *> &selectedItemList) const
{
    for (QGraphicsItem* item : itemUnderMouseList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (formEditorItem
                && !selectedItemList.contains(formEditorItem)
                && isNotAncestorOfItemInList(formEditorItem, selectedItemList)
                && formEditorItem->isContainer()
                && formEditorItem->isContentVisible())
            return formEditorItem;
    }

    return nullptr;
}

void AbstractFormEditorTool::clear()
{
    m_itemList.clear();
}

void AbstractFormEditorTool::start()
{

}
}
