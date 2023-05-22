// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dragtool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "assetslibrarywidget.h"
#include "assetslibrarymodel.h"
#include "materialutils.h"
#include <metainfo.h>
#include <modelnodeoperations.h>
#include <nodehints.h>
#include <rewritingexception.h>
#include "qmldesignerconstants.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QLoggingCategory>
#include <QMimeData>
#include <QTimer>
#include <QWidget>
#include <QMessageBox>

static Q_LOGGING_CATEGORY(dragToolInfo, "qtc.qmldesigner.formeditor", QtWarningMsg);

namespace QmlDesigner {

DragTool::DragTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_moveManipulator(editorView->scene()->manipulatorLayerItem(), editorView),
    m_selectionIndicator(editorView->scene()->manipulatorLayerItem())
{
}

DragTool::~DragTool() = default;

void DragTool::clear()
{
    m_moveManipulator.clear();
    m_selectionIndicator.clear();
    m_movingItems.clear();
}

void DragTool::mousePressEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *) {}
void DragTool::mouseMoveEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *) {}
void DragTool::hoverMoveEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *) {}

void DragTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        abort();
        event->accept();
        commitTransaction();
        view()->changeToSelectionTool();
    }
}

void DragTool::keyReleaseEvent(QKeyEvent *) {}
void DragTool::mouseReleaseEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *) {}
void DragTool::mouseDoubleClickEvent(const QList<QGraphicsItem *> &, QGraphicsSceneMouseEvent *) {}
void DragTool::itemsAboutToRemoved(const QList<FormEditorItem *> &) {}
void DragTool::selectedItemsChanged(const QList<FormEditorItem *> &) {}
void DragTool::updateMoveManipulator() {}

void DragTool::beginWithPoint(const QPointF &beginPoint)
{
    m_movingItems = scene()->itemsForQmlItemNodes(m_dragNodes);

    m_moveManipulator.setItems(m_movingItems);
    m_moveManipulator.begin(beginPoint);
}

void DragTool::createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry,
                                 const QmlItemNode &parentNode,
                                 const QPointF &scenePosition)
{
    FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
    const QPointF positonInItemSpace = parentItem->qmlItemNode().instanceSceneContentItemTransform().inverted().map(scenePosition);
    QPointF itemPos = positonInItemSpace;

    const bool rootIsFlow = QmlItemNode(view()->rootModelNode()).isFlowView();

    QmlItemNode adjustedParentNode = parentNode;

    if (rootIsFlow) {
        itemPos = QPointF();
        adjustedParentNode = view()->rootModelNode();
    }

    m_dragNodes.append(QmlItemNode::createQmlItemNode(view(), itemLibraryEntry, itemPos, adjustedParentNode));

    if (rootIsFlow) {
        for (QmlItemNode &dragNode : m_dragNodes)
            dragNode.setFlowItemPosition(positonInItemSpace);
    }

    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(m_dragNodes));
}

void DragTool::createQmlItemNodeFromImage(const QString &imagePath,
                                          const QmlItemNode &parentNode,
                                          const QPointF &scenePosition)
{
    if (parentNode.isValid()) {
        FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
        QPointF positonInItemSpace = parentItem->qmlItemNode().instanceSceneContentItemTransform().inverted().map(scenePosition);

        m_dragNodes.append(QmlItemNode::createQmlItemNodeFromImage(view(), imagePath, positonInItemSpace, parentNode));
    }
}

void DragTool::createQmlItemNodeFromFont(const QString &fontPath,
                                         const QmlItemNode &parentNode,
                                         const QPointF &scenePos)
{
    if (parentNode.isValid()) {
        FormEditorItem *parentItem = scene()->itemForQmlItemNode(parentNode);
        QPointF positonInItemSpace = parentItem->qmlItemNode().instanceSceneContentItemTransform()
                .inverted().map(scenePos);

        const auto typeAndData = AssetsLibraryWidget::getAssetTypeAndData(fontPath);
        QString fontFamily = QString::fromUtf8(typeAndData.second);

        m_dragNodes.append(QmlItemNode::createQmlItemNodeFromFont(view(), fontFamily,
                                                                  positonInItemSpace, parentNode));
    }
}

FormEditorItem *DragTool::targetContainerOrRootItem(const QList<QGraphicsItem *> &itemList,
                                                    const QList<FormEditorItem *> &currentItems)
{
    FormEditorItem *formEditorItem = containerFormEditorItem(itemList, currentItems);

    if (!formEditorItem)
        formEditorItem = scene()->rootFormEditorItem();

    return formEditorItem;
}

void DragTool::formEditorItemsChanged(const QList<FormEditorItem *> &itemList)
{
    if (!m_movingItems.isEmpty()) {
        for (auto item : std::as_const(m_movingItems)) {
            if (itemList.contains(item)) {
                m_selectionIndicator.updateItems(m_movingItems);
                break;
            }
        }
    }
}

void DragTool::instancesCompleted(const QList<FormEditorItem *> &itemList)
{
    m_moveManipulator.synchronizeInstanceParent(itemList);
    for (FormEditorItem *item : itemList) {
        for (const QmlItemNode &dragNode : std::as_const(m_dragNodes)) {
            if (item->qmlItemNode() == dragNode) {
                clearMoveDelay();
                break;
            }
        }
    }
}

void DragTool::instancesParentChanged(const QList<FormEditorItem *> &itemList)
{
    m_moveManipulator.synchronizeInstanceParent(itemList);
}

void DragTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &) {}

void DragTool::clearMoveDelay()
{
    if (m_blockMove) {
        m_blockMove = false;
        if (!m_dragNodes.isEmpty())
            beginWithPoint(m_startPoint);
    }
}

void DragTool::focusLost() {}

void DragTool::abort()
{
    if (!m_isAborted) {
        m_isAborted = true;

        for (auto &node : m_dragNodes) {
            if (node.isValid())
                node.destroy();
        }
        m_dragNodes.clear();
    }
}

static ItemLibraryEntry itemLibraryEntryFromMimeData(const QMimeData *mimeData)
{
    QDataStream stream(mimeData->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO));

    ItemLibraryEntry itemLibraryEntry;
    stream >> itemLibraryEntry;

    return itemLibraryEntry;
}

static bool canBeDropped(const QMimeData *mimeData)
{
    return NodeHints::fromItemLibraryEntry(itemLibraryEntryFromMimeData(mimeData)).canBeDroppedInFormEditor();
}

static bool hasItemLibraryInfo(const QMimeData *mimeData)
{
    return mimeData->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
}

void DragTool::dropEvent(const QList<QGraphicsItem *> &itemList, QGraphicsSceneDragDropEvent *event)
{
    if (canBeDropped(event->mimeData())) {
        event->accept();
        end(generateUseSnapping(event->modifiers()));

        QString effectPath;
        const QStringList assetPaths = QString::fromUtf8(event->mimeData()
                                                         ->data(Constants::MIME_TYPE_ASSETS)).split(',');
        for (const QString &path : assetPaths) {
            const QString assetType = AssetsLibraryWidget::getAssetTypeAndData(path).first;
            if (assetType == Constants::MIME_TYPE_ASSET_EFFECT) {
                effectPath = path;
                break;
            }
        }

        bool resetPuppet = false;

        if (!effectPath.isEmpty()) {
            FormEditorItem *targetContainerFormEditorItem = targetContainerOrRootItem(itemList);
            if (targetContainerFormEditorItem) {
                if (!ModelNodeOperations::validateEffect(effectPath)) {
                    event->ignore();
                    return;
                }

                bool layerEffect = ModelNodeOperations::useLayerEffect();
                QmlItemNode parentQmlItemNode = targetContainerFormEditorItem->qmlItemNode();
                QmlItemNode::createQmlItemNodeForEffect(view(), parentQmlItemNode, effectPath, layerEffect);
                view()->setSelectedModelNodes({parentQmlItemNode});

                commitTransaction();
            }
        } else {
            for (QmlItemNode &node : m_dragNodes) {
                if (node.isValid()) {
                    if ((node.instanceParentItem().isValid()
                         && node.instanceParent().modelNode().metaInfo().isLayoutable())
                            || node.isFlowItem()) {
                        node.removeProperty("x");
                        node.removeProperty("y");
                        resetPuppet = true;
                    }
                }
            }
            if (resetPuppet)
                view()->resetPuppet(); // Otherwise the layout might not reposition the items

            commitTransaction();

            if (!m_dragNodes.isEmpty()) {
                QList<ModelNode> nodeList;
                for (auto &node : std::as_const(m_dragNodes)) {
                    if (node.isValid())
                        nodeList.append(node);
                }
                view()->setSelectedModelNodes(nodeList);
            }
            m_dragNodes.clear();
        }

        view()->changeToSelectionTool();
        view()->model()->endDrag();
    }
}

void DragTool::dragEnterEvent(const QList<QGraphicsItem *> &/*itemList*/, QGraphicsSceneDragDropEvent *event)
{
    if (canBeDropped(event->mimeData())) {
        m_blockMove = false;

        if (hasItemLibraryInfo(event->mimeData())) {
            view()->widgetInfo().widget->setFocus();
            m_isAborted = false;
        }

        if (!m_rewriterTransaction.isValid()) {
            m_rewriterTransaction = view()->beginRewriterTransaction(QByteArrayLiteral("DragTool::dragEnterEvent"));
        }
    }
}

void DragTool::dragLeaveEvent(const QList<QGraphicsItem *> &/*itemList*/, QGraphicsSceneDragDropEvent *event)
{
    if (canBeDropped(event->mimeData())) {
        event->accept();

        m_moveManipulator.end();
        clear();

        for (auto &node : m_dragNodes) {
            if (node.isValid())
                node.destroy();
        }
        m_dragNodes.clear();

        commitTransaction();
    }

    view()->changeToSelectionTool();
}

void DragTool::createDragNodes(const QMimeData *mimeData, const QPointF &scenePosition,
                               const QList<QGraphicsItem *> &itemList)
{
    if (m_dragNodes.isEmpty()) {
        FormEditorItem *targetContainerFormEditorItem = targetContainerOrRootItem(itemList);
        if (targetContainerFormEditorItem) {
            QmlItemNode targetContainerQmlItemNode = targetContainerFormEditorItem->qmlItemNode();

            if (hasItemLibraryInfo(mimeData)) {
                createQmlItemNode(itemLibraryEntryFromMimeData(mimeData), targetContainerQmlItemNode,
                                  scenePosition);
            } else {
                const QStringList assetPaths = QString::fromUtf8(mimeData
                                    ->data(Constants::MIME_TYPE_ASSETS)).split(',');
                for (const QString &assetPath : assetPaths) {
                    QString assetType = AssetsLibraryWidget::getAssetTypeAndData(assetPath).first;
                    if (assetType == Constants::MIME_TYPE_ASSET_IMAGE)
                        createQmlItemNodeFromImage(assetPath, targetContainerQmlItemNode, scenePosition);
                    else if (assetType == Constants::MIME_TYPE_ASSET_FONT)
                        createQmlItemNodeFromFont(assetPath, targetContainerQmlItemNode, scenePosition);
                }

                if (!m_dragNodes.isEmpty())
                    m_selectionIndicator.setItems(scene()->itemsForQmlItemNodes(m_dragNodes));
            }

            m_blockMove = true;
            m_startPoint = scenePosition;
        }
    }
}

void DragTool::dragMoveEvent(const QList<QGraphicsItem *> &itemList, QGraphicsSceneDragDropEvent *event)
{
    FormEditorItem *targetContainerItem = targetContainerOrRootItem(itemList);
    const QStringList assetPaths = QString::fromUtf8(event->mimeData()
                                                     ->data(Constants::MIME_TYPE_ASSETS)).split(',');
    QString assetType = AssetsLibraryWidget::getAssetTypeAndData(assetPaths[0]).first;

    if (!m_blockMove
            && !m_isAborted
            && canBeDropped(event->mimeData())
            && assetType != Constants::MIME_TYPE_ASSET_EFFECT) {
        event->accept();
        if (!m_dragNodes.isEmpty()) {
            if (targetContainerItem) {
                move(event->scenePos(), itemList);
            } else {
                end();
                for (auto &node : m_dragNodes) {
                    if (node.isValid())
                        node.destroy();
                }
                m_dragNodes.clear();
            }
        } else {
            createDragNodes(event->mimeData(), event->scenePos(), itemList);
        }
    } else if (assetType != Constants::MIME_TYPE_ASSET_EFFECT) {
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

void DragTool::move(const QPointF &scenePosition, const QList<QGraphicsItem *> &itemList)
{
    if (!m_movingItems.isEmpty()) {
        FormEditorItem *containerItem = targetContainerOrRootItem(itemList, m_movingItems);
        for (auto &movingItem : std::as_const(m_movingItems)) {
            if (containerItem && movingItem->parentItem() &&
                containerItem != movingItem->parentItem()) {
                const QmlItemNode movingNode = movingItem->qmlItemNode();
                const QmlItemNode containerNode = containerItem->qmlItemNode();

                qCInfo(dragToolInfo()) << Q_FUNC_INFO << movingNode << containerNode << movingNode.canBereparentedTo(containerNode);

                if (movingNode.canBereparentedTo(containerNode))
                    m_moveManipulator.reparentTo(containerItem);
            }
        }

        Snapper::Snapping useSnapping = Snapper::UseSnapping;

        m_moveManipulator.update(scenePosition, useSnapping, MoveManipulator::UseBaseState);
    }
}

void DragTool::commitTransaction()
{
    try {
        handleView3dDrop();
        m_rewriterTransaction.commit();
    } catch (const RewritingException &e) {
        e.showException();
    }
}

void DragTool::handleView3dDrop()
{
    // If a View3D is dropped, we need to assign material to the included model
    for (const QmlItemNode &dragNode : std::as_const(m_dragNodes)) {
        if (dragNode.modelNode().metaInfo().isQtQuick3DView3D()) {
            auto model = dragNode.model();
            const QList<ModelNode> models = dragNode.modelNode().subModelNodesOfType(
                model->qtQuick3DModelMetaInfo());
            QTC_ASSERT(models.size() == 1, return);
            MaterialUtils::assignMaterialTo3dModel(view(), models.at(0));
        }
    }
}

} // namespace QmlDesigner
