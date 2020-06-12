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

#include "formeditorview.h"
#include "nodeinstanceview.h"
#include "selectiontool.h"
#include "movetool.h"
#include "resizetool.h"
#include "dragtool.h"
#include "formeditorwidget.h"
#include <formeditorgraphicsview.h>
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "abstractcustomtool.h"

#include <bindingproperty.h>
#include <variantproperty.h>
#include <designersettings.h>
#include <designmodecontext.h>
#include <modelnode.h>
#include <model.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <rewriterview.h>
#include <zoomaction.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QPair>
#include <QString>
#include <QTimer>
#include <memory>

namespace QmlDesigner {

FormEditorView::FormEditorView(QObject *parent)
    : AbstractView(parent)
{
}

FormEditorScene* FormEditorView::scene() const
{
    return m_scene.data();
}

FormEditorView::~FormEditorView()
{
    m_currentTool = nullptr;
    qDeleteAll(m_customToolList);
}

void FormEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);
    temporaryBlockView();

    AbstractView::modelAttached(model);

    Q_ASSERT(m_scene->formLayerItem());

    if (QmlItemNode::isValidQmlItemNode(rootModelNode()))
        setupFormEditorItemTree(rootModelNode());

    m_formEditorWidget->updateActions();

    if (!rewriterView()->errors().isEmpty())
        m_formEditorWidget->showErrorMessageBox(rewriterView()->errors());
    else
        m_formEditorWidget->hideErrorMessageBox();

    if (!rewriterView()->warnings().isEmpty())
        m_formEditorWidget->showWarningMessageBox(rewriterView()->warnings());
}


//This function does the setup of the initial FormEditorItem tree in the scene
void FormEditorView::setupFormEditorItemTree(const QmlItemNode &qmlItemNode)
{
    if (qmlItemNode.isFlowTransition()) {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::FlowTransition);
        if (qmlItemNode.hasNodeParent())
            m_scene->reparentItem(qmlItemNode, qmlItemNode.modelParentItem());
        m_scene->synchronizeTransformation(m_scene->itemForQmlItemNode(qmlItemNode));
    } else if (qmlItemNode.isFlowDecision()) {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::FlowDecision);
        if (qmlItemNode.hasNodeParent())
            m_scene->reparentItem(qmlItemNode, qmlItemNode.modelParentItem());
        m_scene->synchronizeTransformation(m_scene->itemForQmlItemNode(qmlItemNode));
    } else if (qmlItemNode.isFlowWildcard()) {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::FlowWildcard);
        if (qmlItemNode.hasNodeParent())
            m_scene->reparentItem(qmlItemNode, qmlItemNode.modelParentItem());
        m_scene->synchronizeTransformation(m_scene->itemForQmlItemNode(qmlItemNode));
    } else if (qmlItemNode.isFlowActionArea()) {
        m_scene->addFormEditorItem(qmlItemNode.toQmlItemNode(), FormEditorScene::FlowAction);
        m_scene->synchronizeParent(qmlItemNode.toQmlItemNode());
    } else if (qmlItemNode.isFlowItem() && !qmlItemNode.isRootNode()) {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::Flow);
        m_scene->synchronizeParent(qmlItemNode);
        m_scene->synchronizeTransformation(m_scene->itemForQmlItemNode(qmlItemNode));
        for (const QmlObjectNode &nextNode : qmlItemNode.allDirectSubNodes())
            if (QmlItemNode::isValidQmlItemNode(nextNode) && nextNode.toQmlItemNode().isFlowActionArea()) {
                setupFormEditorItemTree(nextNode.toQmlItemNode());
            }
    } else if (qmlItemNode.isFlowView() && qmlItemNode.isRootNode()) {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::Flow);

        ModelNode node = qmlItemNode.modelNode();
        if (!node.hasAuxiliaryData("width") && !node.hasAuxiliaryData("height")) {
            node.setAuxiliaryData("width", 10000);
            node.setAuxiliaryData("height", 10000);
        }

        for (const QmlObjectNode &nextNode : qmlItemNode.allDirectSubNodes()) {
            if (QmlItemNode::isValidQmlItemNode(nextNode) && nextNode.toQmlItemNode().isFlowItem()) {
                setupFormEditorItemTree(nextNode.toQmlItemNode());
            }
        }

        for (const QmlObjectNode &nextNode : qmlItemNode.allDirectSubNodes()) {
            if (QmlVisualNode::isValidQmlVisualNode(nextNode) && nextNode.toQmlVisualNode().isFlowTransition()) {
                setupFormEditorItemTree(nextNode.toQmlItemNode());
            } else if (QmlVisualNode::isValidQmlVisualNode(nextNode) && nextNode.toQmlVisualNode().isFlowDecision()) {
                setupFormEditorItemTree(nextNode.toQmlItemNode());
            } else if (QmlVisualNode::isValidQmlVisualNode(nextNode) && nextNode.toQmlVisualNode().isFlowWildcard()) {
                setupFormEditorItemTree(nextNode.toQmlItemNode());
            }
        }
    } else {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::Default);
        for (const QmlObjectNode &nextNode : qmlItemNode.allDirectSubNodes()) //TODO instance children
            //If the node has source for components/custom parsers we ignore it.
            if (QmlItemNode::isValidQmlItemNode(nextNode) && nextNode.modelNode().nodeSourceType() == ModelNode::NodeWithoutSource)
                setupFormEditorItemTree(nextNode.toQmlItemNode());
    }
}

static void deleteWithoutChildren(const QList<FormEditorItem*> &items)
{
    foreach (FormEditorItem *item, items) {
        foreach (QGraphicsItem *child, item->childItems()) {
            child->setParentItem(item->scene()->rootFormEditorItem());
        }
        delete item;
    }
}

static bool isFlowNonItem(const QmlItemNode &itemNode)
{
    return itemNode.isFlowTransition()
            || itemNode.isFlowWildcard()
            || itemNode.isFlowWildcard();
}

void FormEditorView::removeNodeFromScene(const QmlItemNode &qmlItemNode)
{
    QList<FormEditorItem*> removedItemList;

    if (qmlItemNode.isValid()) {
        QList<QmlItemNode> nodeList;
        nodeList.append(qmlItemNode.allSubModelNodes());
        nodeList.append(qmlItemNode);

        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));

        //The destructor of QGraphicsItem does delete all its children.
        //We have to keep the children if they are not children in the model anymore.
        //Otherwise we delete the children explicitly anyway.
        deleteWithoutChildren(removedItemList);
    } else if (isFlowNonItem(qmlItemNode)) {
        removedItemList.append(scene()->itemsForQmlItemNodes({qmlItemNode}));
        deleteWithoutChildren(removedItemList);
    }

    if (!removedItemList.isEmpty())
        m_currentTool->itemsAboutToRemoved(removedItemList);
}

void FormEditorView::hideNodeFromScene(const QmlItemNode &qmlItemNode)
{
    if (FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode)) {
        QList<FormEditorItem*> removedItems = scene()->itemsForQmlItemNodes(qmlItemNode.allSubModelNodes());
        removedItems.append(item);
        m_currentTool->itemsAboutToRemoved(removedItems);
        item->setFormEditorVisible(false);
    }
}

void FormEditorView::createFormEditorWidget()
{
    m_formEditorWidget = QPointer<FormEditorWidget>(new FormEditorWidget(this));
    m_scene = QPointer<FormEditorScene>(new FormEditorScene(m_formEditorWidget.data(), this));

    m_moveTool = std::make_unique<MoveTool>(this);
    m_selectionTool = std::make_unique<SelectionTool>(this);
    m_resizeTool = std::make_unique<ResizeTool>(this);
    m_dragTool = std::make_unique<DragTool>(this);

    m_currentTool = m_selectionTool.get();

    auto formEditorContext = new Internal::FormEditorContext(m_formEditorWidget.data());
    Core::ICore::addContextObject(formEditorContext);

    connect(m_formEditorWidget->zoomAction(), &ZoomAction::zoomLevelChanged, [this]() {
        m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
    });

    connect(m_formEditorWidget->showBoundingRectAction(), &QAction::toggled, scene(), &FormEditorScene::setShowBoundingRects);
    connect(m_formEditorWidget->resetAction(), &QAction::triggered, this, &FormEditorView::resetNodeInstanceView);
}

void FormEditorView::temporaryBlockView()
{
    m_formEditorWidget->graphicsView()->setUpdatesEnabled(false);
    static auto timer = new QTimer(qApp);
    timer->setSingleShot(true);
    timer->start(1000);

    connect(timer, &QTimer::timeout, this, [this]() {
        m_formEditorWidget->graphicsView()->setUpdatesEnabled(true);
    });
}

void FormEditorView::nodeCreated(const ModelNode &node)
{
    //If the node has source for components/custom parsers we ignore it.
    if (QmlItemNode::isValidQmlItemNode(node) && node.nodeSourceType() == ModelNode::NodeWithoutSource) //only setup QmlItems
        setupFormEditorItemTree(QmlItemNode(node));
    else if (QmlVisualNode::isFlowTransition(node))
        setupFormEditorItemTree(QmlItemNode(node));
}

void FormEditorView::modelAboutToBeDetached(Model *model)
{
    m_currentTool->setItems(QList<FormEditorItem*>());
    m_selectionTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_dragTool->clear();
    foreach (AbstractCustomTool *customTool, m_customToolList)
        customTool->clear();
    m_scene->clearFormEditorItems();
    m_formEditorWidget->updateActions();
    m_formEditorWidget->resetView();
    scene()->resetScene();

    m_currentTool = m_selectionTool.get();

    AbstractView::modelAboutToBeDetached(model);
}

void FormEditorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    reset();
}

void FormEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    const QmlItemNode qmlItemNode(removedNode);

    removeNodeFromScene(qmlItemNode);
}

void FormEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    foreach (FormEditorItem *item, m_scene->allFormEditorItems()) {
        item->setParentItem(nullptr);
    }

    foreach (FormEditorItem *item, m_scene->allFormEditorItems()) {
        m_scene->removeItemFromHash(item);
        delete item;
    }

    QmlItemNode newItemNode(rootModelNode());
    if (newItemNode.isValid()) //only setup QmlItems
        setupFormEditorItemTree(newItemNode);

    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
}

void FormEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    QList<FormEditorItem*> removedItems;
    foreach (const AbstractProperty &property, propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty = property.toNodeAbstractProperty();

            foreach (const ModelNode &modelNode, nodeAbstractProperty.allSubNodes()) {
                const QmlItemNode qmlItemNode(modelNode);

                if (qmlItemNode.isValid()){
                    if (FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode)) {
                        removedItems.append(item);
                        delete item;
                    }
                } else if (isFlowNonItem(qmlItemNode)) {
                    if (FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode)) {
                        removedItems.append(item);
                        delete item;
                    }
                }
            }
        }
    }
    m_currentTool->itemsAboutToRemoved(removedItems);
}

static inline bool hasNodeSourceParent(const ModelNode &node)
{
    if (node.hasParentProperty() && node.parentProperty().parentModelNode().isValid()) {
        ModelNode parent = node.parentProperty().parentModelNode();
        if (parent.nodeSourceType() != ModelNode::NodeWithoutSource)
            return true;
        return hasNodeSourceParent(parent);
    }
    return false;
}

void FormEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (hasNodeSourceParent(node))
        hideNodeFromScene(node);
}

WidgetInfo FormEditorView::widgetInfo()
{
    if (!m_formEditorWidget)
        createFormEditorWidget();

    return createWidgetInfo(m_formEditorWidget.data(), nullptr, "FormEditor", WidgetInfo::CentralPane, 0, tr("Form Editor"), DesignerWidgetFlags::IgnoreErrors);
}

FormEditorWidget *FormEditorView::formEditorWidget()
{
    return m_formEditorWidget.data();
}

void FormEditorView::nodeIdChanged(const ModelNode& node, const QString &/*newId*/, const QString &/*oldId*/)
{
    QmlItemNode itemNode(node);

    if (itemNode.isValid() && node.nodeSourceType() == ModelNode::NodeWithoutSource) {
        FormEditorItem *item = m_scene->itemForQmlItemNode(itemNode);
        if (item) {
            if (node.isSelected()) {
                m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
                m_scene->update();
             }
            item->update();
        }
    }

}

void FormEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeListKeppInvalid(selectedNodeList)));

    m_scene->update();
}

void FormEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                              AbstractView::PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyChange)
    for (const VariantProperty &property : propertyList) {
        QmlVisualNode node(property.parentModelNode());
        if (node.isFlowTransition() || node.isFlowDecision()) {
            if (FormEditorItem *item = m_scene->itemForQmlItemNode(node.toQmlItemNode())) {
                if (property.name() == "question" || property.name() == "dialogTitle")
                    item->updateGeometry();
            }
        }
    }
}

void FormEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                              AbstractView::PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyChange)
    for (const BindingProperty &property : propertyList) {
        QmlVisualNode node(property.parentModelNode());
        if (node.isFlowTransition()) {
            if (FormEditorItem *item = m_scene->itemForQmlItemNode(node.toQmlItemNode())) {
                if (property.name() == "condition" || property.name() == "question")
                    item->updateGeometry();

                if (node.hasNodeParent()) {
                    m_scene->reparentItem(node.toQmlItemNode(), node.toQmlItemNode().modelParentItem());
                    m_scene->synchronizeTransformation(item);
                    item->update();
                }
            }
        } else if (QmlFlowActionAreaNode::isValidQmlFlowActionAreaNode(property.parentModelNode())) {
            const QmlVisualNode target = property.resolveToModelNode();
            if (target.modelNode().isValid() && target.isFlowTransition()) {
                FormEditorItem *item = m_scene->itemForQmlItemNode(target.toQmlItemNode());
                if (item) {
                    const QmlItemNode itemNode = node.toQmlItemNode();
                    if (itemNode.hasNodeParent())
                        m_scene->reparentItem(itemNode, itemNode.modelParentItem());
                    m_scene->synchronizeTransformation(item);
                    item->update();
                }
            }
        }
    }
}

void FormEditorView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (!errors.isEmpty())
        m_formEditorWidget->showErrorMessageBox(errors);
    else
        m_formEditorWidget->hideErrorMessageBox();
}

void FormEditorView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &/*data*/)
{
    if (identifier == QLatin1String("puppet crashed"))
        m_dragTool->clearMoveDelay();
    if (identifier == QLatin1String("reset QmlPuppet"))
        temporaryBlockView();
    if (identifier == QLatin1String("fit root to screen")) {
        if (QmlItemNode(rootModelNode()).isFlowView()) {
            QRectF boundingRect;
            for (QGraphicsItem *item : scene()->items()) {
                if (auto formEditorItem = FormEditorItem::fromQGraphicsItem(item)) {
                    if (!formEditorItem->qmlItemNode().modelNode().isRootNode()
                        && !formEditorItem->sceneBoundingRect().isNull())
                        boundingRect = boundingRect.united(formEditorItem->sceneBoundingRect());
                }
            }
            m_formEditorWidget->graphicsView()->fitInView(boundingRect,
                                                          Qt::KeepAspectRatio);
        } else {
            m_formEditorWidget->graphicsView()->fitInView(m_formEditorWidget->rootItemRect(),
                                                          Qt::KeepAspectRatio);
        }

        const qreal scaleFactor = m_formEditorWidget->graphicsView()->viewportTransform().m11();
        float zoomLevel = ZoomAction::getClosestZoomLevel(scaleFactor);
        m_formEditorWidget->zoomAction()->forceZoomLevel(zoomLevel);
    }
    if (identifier == QLatin1String("fit selection to screen")) {
        if (nodeList.isEmpty())
            return;

        QRectF boundingRect;
        for (const ModelNode &node : nodeList) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(node))
                boundingRect = boundingRect.united(item->sceneBoundingRect());
        }

        m_formEditorWidget->graphicsView()->fitInView(boundingRect,
                                                      Qt::KeepAspectRatio);
        const qreal scaleFactor = m_formEditorWidget->graphicsView()->viewportTransform().m11();
        float zoomLevel = ZoomAction::getClosestZoomLevel(scaleFactor);
        m_formEditorWidget->zoomAction()->forceZoomLevel(zoomLevel);
    }
}

AbstractFormEditorTool *FormEditorView::currentTool() const
{
    return m_currentTool;
}

bool FormEditorView::changeToMoveTool()
{
    if (m_currentTool == m_moveTool.get())
        return true;
    if (!isMoveToolAvailable())
        return false;
    changeCurrentToolTo(m_moveTool.get());
    return true;
}

void FormEditorView::changeToDragTool()
{
    if (m_currentTool == m_dragTool.get())
        return;
    changeCurrentToolTo(m_dragTool.get());
}


bool FormEditorView::changeToMoveTool(const QPointF &beginPoint)
{
    if (m_currentTool == m_moveTool.get())
        return true;
    if (!isMoveToolAvailable())
        return false;
    changeCurrentToolTo(m_moveTool.get());
    m_moveTool->beginWithPoint(beginPoint);
    return true;
}

void FormEditorView::changeToSelectionTool()
{
    if (m_currentTool == m_selectionTool.get())
        return;
    changeCurrentToolTo(m_selectionTool.get());
}

void FormEditorView::changeToSelectionTool(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool == m_selectionTool.get())
        return;
    changeCurrentToolTo(m_selectionTool.get());
    m_selectionTool->selectUnderPoint(event);
}

void FormEditorView::changeToResizeTool()
{
    if (m_currentTool == m_resizeTool.get())
        return;
    changeCurrentToolTo(m_resizeTool.get());
}

void FormEditorView::changeToTransformTools()
{
    if (m_currentTool == m_moveTool.get() ||
       m_currentTool == m_resizeTool.get() ||
       m_currentTool == m_selectionTool.get())
        return;
    changeToSelectionTool();
}

void FormEditorView::changeToCustomTool()
{
    if (hasSelectedModelNodes()) {
        int handlingRank = 0;
        AbstractCustomTool *selectedCustomTool = nullptr;

        const ModelNode selectedModelNode = selectedModelNodes().constFirst();

        for (AbstractCustomTool *customTool : m_customToolList) {
            if (customTool->wantHandleItem(selectedModelNode) > handlingRank) {
                handlingRank = customTool->wantHandleItem(selectedModelNode);
                selectedCustomTool = customTool;
            }
        }

        if (handlingRank > 0 && selectedCustomTool)
            changeCurrentToolTo(selectedCustomTool);
    }
}

void FormEditorView::changeCurrentToolTo(AbstractFormEditorTool *newTool)
{
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = newTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(
        selectedModelNodes())));

    m_currentTool->start();
}

void FormEditorView::registerTool(AbstractCustomTool *tool)
{
    tool->setView(this);
    m_customToolList.append(tool);
}

void FormEditorView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
    QmlItemNode item(node);
    AbstractView::auxiliaryDataChanged(node, name, data);
    if (name == "invisible") {
        if (FormEditorItem *item = scene()->itemForQmlItemNode(QmlItemNode(node))) {
            bool isInvisible = data.toBool();
            if (item->isFormEditorVisible())
                item->setVisible(!isInvisible);
            ModelNode newNode(node);
            if (isInvisible)
                newNode.deselectNode();
        }
    } else if (item.isFlowTransition() || item.isFlowActionArea()
               || item.isFlowDecision() || item.isFlowWildcard()) {
        if (FormEditorItem *editorItem = m_scene->itemForQmlItemNode(item)) {
            // Update the geomtry if one of the following auxiliary properties has changed
            static const QStringList updateGeometryPropertyNames = {
                "breakPoint", "bezier", "transitionBezier", "type", "tranitionType", "radius",
                "transitionRadius", "labelPosition", "labelFlipSide", "inOffset", "outOffset",
                "blockSize", "blockRadius", "showDialogLabel", "dialogLabelPosition"
            };
            if (updateGeometryPropertyNames.contains(QString::fromUtf8(name)))
                editorItem->updateGeometry();

            editorItem->update();
        }
    } else if (item.isFlowView() || item.isFlowItem()) {
        scene()->update();
    } else if (name == "annotation" || name == "customId") {
        if (FormEditorItem *editorItem = scene()->itemForQmlItemNode(item)) {
            editorItem->update();
        }
    }
}

void FormEditorView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    QList<FormEditorItem*> itemNodeList;
    for (const ModelNode &node : completedNodeList) {
        const QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid()) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
                scene()->synchronizeParent(qmlItemNode);
                itemNodeList.append(item);
            }
        }
    }
    currentTool()->instancesCompleted(itemNodeList);
}

void FormEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash)
{
    QList<FormEditorItem*> changedItems;
    const int rootElementInitWidth = DesignerSettings::getValue(DesignerSettingsKey::ROOT_ELEMENT_INIT_WIDTH).toInt();
    const int rootElementInitHeight = DesignerSettings::getValue(DesignerSettingsKey::ROOT_ELEMENT_INIT_HEIGHT).toInt();

    QList<ModelNode> informationChangedNodes = Utils::filtered(informationChangedHash.keys(), [](const ModelNode &node) {
        return QmlItemNode::isValidQmlItemNode(node);
    });

    for (const ModelNode &node : informationChangedNodes) {
        const QmlItemNode qmlItemNode(node);
        if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeTransformation(item);
            if (qmlItemNode.isRootModelNode() && informationChangedHash.values(node).contains(Size)) {
                if (qmlItemNode.instanceBoundingRect().isEmpty() &&
                        !(qmlItemNode.propertyAffectedByCurrentState("width")
                          && qmlItemNode.propertyAffectedByCurrentState("height"))) {
                    if (!(rootModelNode().hasAuxiliaryData("width")))
                        rootModelNode().setAuxiliaryData("width", rootElementInitWidth);
                    if (!(rootModelNode().hasAuxiliaryData("height")))
                        rootModelNode().setAuxiliaryData("height", rootElementInitHeight);
                    rootModelNode().setAuxiliaryData("autoSize", true);
                    formEditorWidget()->updateActions();
                } else {
                    if (rootModelNode().hasAuxiliaryData("autoSize")
                            && (qmlItemNode.propertyAffectedByCurrentState("width")
                                || qmlItemNode.propertyAffectedByCurrentState("height"))) {
                        rootModelNode().setAuxiliaryData("width", QVariant());
                        rootModelNode().setAuxiliaryData("height", QVariant());
                        rootModelNode().removeAuxiliaryData("autoSize");
                        formEditorWidget()->updateActions();
                    }
                }
                formEditorWidget()->setRootItemRect(qmlItemNode.instanceBoundingRect());
                formEditorWidget()->centerScene();
            }

            changedItems.append(item);
        }
    }

    m_currentTool->formEditorItemsChanged(changedItems);
}

void FormEditorView::instancesRenderImageChanged(const QVector<ModelNode> &nodeList)
{
    for (const ModelNode &node : nodeList) {
        if (QmlItemNode::isValidQmlItemNode(node))
             if (FormEditorItem *item = scene()->itemForQmlItemNode(QmlItemNode(node)))
                 item->update();
    }
}

void FormEditorView::instancesChildrenChanged(const QVector<ModelNode> &nodeList)
{
    QList<FormEditorItem*> changedItems;

    for (const ModelNode &node : nodeList) {
        const QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid()) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
                scene()->synchronizeParent(qmlItemNode);
                changedItems.append(item);
            }
        }
    }

    m_currentTool->formEditorItemsChanged(changedItems);
    m_currentTool->instancesParentChanged(changedItems);
}

void FormEditorView::rewriterBeginTransaction()
{
    m_transactionCounter++;
}

void FormEditorView::rewriterEndTransaction()
{
    m_transactionCounter--;
}

double FormEditorView::containerPadding() const
{
    return m_formEditorWidget->containerPadding();
}

double FormEditorView::spacing() const
{
    return m_formEditorWidget->spacing();
}

void FormEditorView::gotoError(int line, int column)
{
    if (m_gotoErrorCallback)
        m_gotoErrorCallback(line, column);
}

void FormEditorView::setGotoErrorCallback(std::function<void (int, int)> gotoErrorCallback)
{
    m_gotoErrorCallback = gotoErrorCallback;
}

void FormEditorView::exportAsImage()
{
    m_formEditorWidget->exportAsImage(m_scene->rootFormEditorItem()->boundingRect());
}

QmlItemNode findRecursiveQmlItemNode(const QmlObjectNode &firstQmlObjectNode)
{
    QmlObjectNode qmlObjectNode = firstQmlObjectNode;

    while (true)  {
        QmlItemNode itemNode = qmlObjectNode.toQmlItemNode();
        if (itemNode.isValid())
            return itemNode;
        if (qmlObjectNode.hasInstanceParent())
            qmlObjectNode = qmlObjectNode.instanceParent();
        else
            break;
    }

    return QmlItemNode();
}

void FormEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    QList<FormEditorItem*> changedItems;
    for (auto &nodePropertyPair : propertyList) {
        const QmlItemNode qmlItemNode(nodePropertyPair.first);
        const PropertyName propertyName = nodePropertyPair.second;
        if (qmlItemNode.isValid()) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
                static const PropertyNameList skipList({"x", "y", "width", "height"});
                if (!skipList.contains(propertyName)) {
                    m_scene->synchronizeOtherProperty(item, propertyName);
                    changedItems.append(item);
                }
            }
        }
    }
    m_currentTool->formEditorItemsChanged(changedItems);
}

bool FormEditorView::isMoveToolAvailable() const
{
    if (hasSingleSelectedModelNode() && QmlItemNode::isValidQmlItemNode(singleSelectedModelNode())) {
        QmlItemNode selectedQmlItemNode(singleSelectedModelNode());
        return selectedQmlItemNode.instanceIsMovable()
                && selectedQmlItemNode.modelIsMovable()
                && !selectedQmlItemNode.instanceIsInLayoutable();
    }

    return true;
}

void FormEditorView::resetNodeInstanceView()
{
    setCurrentStateNode(rootModelNode());
    resetPuppet();
}

void FormEditorView::reset()
{
   QTimer::singleShot(200, this, &FormEditorView::delayedReset);
}

void FormEditorView::delayedReset()
{
    m_selectionTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_dragTool->clear();
    m_scene->clearFormEditorItems();
    if (isAttached() && QmlItemNode::isValidQmlItemNode(rootModelNode()))
        setupFormEditorItemTree(rootModelNode());
}

}

