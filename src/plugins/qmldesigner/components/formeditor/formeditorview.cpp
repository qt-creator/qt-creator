// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorview.h"
#include "nodeinstanceview.h"
#include "selectiontool.h"
#include "rotationtool.h"
#include "movetool.h"
#include "resizetool.h"
#include "dragtool.h"
#include "formeditorwidget.h"
#include <formeditorgraphicsview.h>
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "abstractcustomtool.h"

#include <auxiliarydataproperties.h>
#include <qmldesignerplugin.h>
#include <bindingproperty.h>
#include <designersettings.h>
#include <designmodecontext.h>
#include <model.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewriterview.h>
#include <variantproperty.h>
#include <zoomaction.h>
#include <qml3dnode.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <memory>
#include <QDebug>
#include <QPair>
#include <QPicture>
#include <QString>
#include <QTimer>

namespace QmlDesigner {

FormEditorView::FormEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{}

FormEditorScene* FormEditorView::scene() const
{
    return m_scene.data();
}

FormEditorView::~FormEditorView()
{
    m_currentTool = nullptr;
}

void FormEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (!isEnabled())
        return;

    m_formEditorWidget->setBackgoundImage({});

    temporaryBlockView();
    setupFormEditorWidget();

    setupRootItemSize();
}


//This function does the setup of the initial FormEditorItem tree in the scene
void FormEditorView::setupFormEditorItemTree(const QmlItemNode &qmlItemNode)
{
    if (!qmlItemNode.hasFormEditorItem())
        return;

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
        FormEditorItem *rootItem = m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::Flow);

        ModelNode node = qmlItemNode.modelNode();
        if (!node.hasAuxiliaryData(widthProperty) && !node.hasAuxiliaryData(heightProperty)) {
            node.setAuxiliaryData(widthProperty, 10000);
            node.setAuxiliaryData(heightProperty, 10000);
        }

        m_scene->synchronizeTransformation(rootItem);
        formEditorWidget()->setRootItemRect(qmlItemNode.instanceBoundingRect());

        const QList<QmlObjectNode> allDirectSubNodes = qmlItemNode.allDirectSubNodes();
        for (const QmlObjectNode &childNode : allDirectSubNodes) {
            if (QmlItemNode::isValidQmlItemNode(childNode)
                && childNode.toQmlItemNode().isFlowItem()) {
                setupFormEditorItemTree(childNode.toQmlItemNode());
            }
        }

        for (const QmlObjectNode &childNode : allDirectSubNodes) {
            if (QmlVisualNode::isValidQmlVisualNode(childNode)
                && childNode.toQmlVisualNode().isFlowTransition()) {
                setupFormEditorItemTree(childNode.toQmlItemNode());
            } else if (QmlVisualNode::isValidQmlVisualNode(childNode)
                       && childNode.toQmlVisualNode().isFlowDecision()) {
                setupFormEditorItemTree(childNode.toQmlItemNode());
            } else if (QmlVisualNode::isValidQmlVisualNode(childNode)
                       && childNode.toQmlVisualNode().isFlowWildcard()) {
                setupFormEditorItemTree(childNode.toQmlItemNode());
            }
        }
    } else {
        m_scene->addFormEditorItem(qmlItemNode, FormEditorScene::Default);
        for (const QmlObjectNode &nextNode : qmlItemNode.allDirectSubNodes()) //TODO instance children
            //If the node has source for components/custom parsers we ignore it.
            if (QmlItemNode::isValidQmlItemNode(nextNode) && nextNode.modelNode().nodeSourceType() == ModelNode::NodeWithoutSource)
                setupFormEditorItemTree(nextNode.toQmlItemNode());
    }

    checkRootModelNode();
}

static void deleteWithoutChildren(const QList<FormEditorItem*> &items)
{
    for (const FormEditorItem *item : items) {
        for (QGraphicsItem *child : item->childItems()) {
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

void FormEditorView::createFormEditorWidget()
{
    m_formEditorWidget = QPointer<FormEditorWidget>(new FormEditorWidget(this));
    m_scene = QPointer<FormEditorScene>(new FormEditorScene(m_formEditorWidget.data(), this));

    m_moveTool = std::make_unique<MoveTool>(this);
    m_selectionTool = std::make_unique<SelectionTool>(this);
    m_rotationTool = std::make_unique<RotationTool>(this);
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

void FormEditorView::temporaryBlockView(int duration)
{
    m_formEditorWidget->graphicsView()->setUpdatesEnabled(false);
    static auto timer = new QTimer(qApp);
    timer->setSingleShot(true);
    timer->start(duration);

    connect(timer, &QTimer::timeout, this, [this]() {
        if (m_formEditorWidget && m_formEditorWidget->graphicsView())
            m_formEditorWidget->graphicsView()->setUpdatesEnabled(true);
    });
}

void FormEditorView::nodeCreated(const ModelNode &node)
{
    if (QmlVisualNode::isFlowTransition(node))
        setupFormEditorItemTree(QmlItemNode(node));
}

void FormEditorView::cleanupToolsAndScene()
{
    m_currentTool->setItems(QList<FormEditorItem *>());
    m_selectionTool->clear();
    m_rotationTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_dragTool->clear();
    for (auto &customTool : m_customTools)
        customTool->clear();
    m_scene->clearFormEditorItems();
    m_formEditorWidget->updateActions();
    m_formEditorWidget->resetView();
    scene()->resetScene();

    changeCurrentToolTo(m_selectionTool.get());
}

void FormEditorView::modelAboutToBeDetached(Model *model)
{
    cleanupToolsAndScene();
    AbstractView::modelAboutToBeDetached(model);
}

void FormEditorView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
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
    const QList<FormEditorItem *> items = m_scene->allFormEditorItems();
    for (FormEditorItem *item : items) {
        item->setParentItem(nullptr);
        m_scene->removeItemFromHash(item);
        deleteWithoutChildren({item});
    }

    QmlItemNode newItemNode(rootModelNode());
    if (newItemNode.isValid()) //only setup QmlItems
        setupFormEditorItemTree(newItemNode);



    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));

    checkRootModelNode();
}

void FormEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    QList<FormEditorItem*> removedItems;
    for (const AbstractProperty &property : propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty = property.toNodeAbstractProperty();

            const QList<ModelNode> modelNodes = nodeAbstractProperty.allSubNodes();
            for (const ModelNode &modelNode : modelNodes) {
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

static inline bool hasNodeSourceOrNonItemParent(const ModelNode &node)
{
    if (ModelNode parent = node.parentProperty().parentModelNode()) {
        if (parent.nodeSourceType() != ModelNode::NodeWithoutSource
                || !QmlItemNode::isItemOrWindow(parent)) {
            return true;
        }
        return hasNodeSourceOrNonItemParent(parent);
    }
    return false;
}

void FormEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    addOrRemoveFormEditorItem(node);
}

void FormEditorView::nodeSourceChanged(const ModelNode &node,
                                       [[maybe_unused]] const QString &newNodeSource)
{
    addOrRemoveFormEditorItem(node);
}

WidgetInfo FormEditorView::widgetInfo()
{
    if (!m_formEditorWidget)
        createFormEditorWidget();

    return createWidgetInfo(m_formEditorWidget.data(),
                            "FormEditor",
                            WidgetInfo::CentralPane,
                            0,
                            tr("2D"),
                            tr("2D view"),
                            DesignerWidgetFlags::IgnoreErrors);
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
                                          const QList<ModelNode> &lastSelectedNodeList)
{
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeListKeppInvalid(selectedNodeList)));

    m_scene->update();

    if (selectedNodeList.empty())
        m_formEditorWidget->zoomSelectionAction()->setEnabled(false);
    else
        m_formEditorWidget->zoomSelectionAction()->setEnabled(true);

    for (const ModelNode &node : lastSelectedNodeList) { /*Set Z to 0 for unselected items */
        QmlVisualNode visualNode(node); /* QmlVisualNode extends ModelNode with extra methods for "visual nodes" */
        if (visualNode.isFlowTransition()) { /* Check if a QmlVisualNode Transition */
            if (FormEditorItem *item = m_scene->itemForQmlItemNode(visualNode.toQmlItemNode())) { /* Get the form editor item from the form editor */
                item->setZValue(0);
            }
        }
   }
   for (const ModelNode &node : selectedNodeList) {
       QmlVisualNode visualNode(node);
       if (visualNode.isFlowTransition()) {
           if (FormEditorItem *item = m_scene->itemForQmlItemNode(visualNode.toQmlItemNode())) {
               item->setZValue(11);
           }
       }
   }
}

void FormEditorView::variantPropertiesChanged(
    const QList<VariantProperty> &propertyList,
    [[maybe_unused]] AbstractView::PropertyChangeFlags propertyChange)
{
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

void FormEditorView::bindingPropertiesChanged(
    const QList<BindingProperty> &propertyList,
    [[maybe_unused]] AbstractView::PropertyChangeFlags propertyChange)
{
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
            if (target.isFlowTransition()) {
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
    QTC_ASSERT(model(), return);
    QTC_ASSERT(model()->rewriterView(), return);

    if (!errors.isEmpty() && !model()->rewriterView()->hasIncompleteTypeInformation())
        m_formEditorWidget->showErrorMessageBox(errors);
    else if (rewriterView()->errors().isEmpty())
        m_formEditorWidget->hideErrorMessageBox();

    checkRootModelNode();
}

void FormEditorView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
    if (identifier == QLatin1String("puppet crashed"))
        m_dragTool->clearMoveDelay();
    if (identifier == QLatin1String("reset QmlPuppet"))
        temporaryBlockView();
}

void FormEditorView::currentStateChanged(const ModelNode & /*node*/)
{
    temporaryBlockView();
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

void FormEditorView::resetToSelectionTool()
{
    changeCurrentToolTo(m_selectionTool.get());
}

void FormEditorView::changeToRotationTool() {
    if (m_currentTool == m_rotationTool.get())
        return;
    changeCurrentToolTo(m_rotationTool.get());
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
            m_currentTool == m_rotationTool.get() ||
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

        for (const auto &customTool : std::as_const(m_customTools)) {
            if (customTool->wantHandleItem(selectedModelNode) > handlingRank) {
                handlingRank = customTool->wantHandleItem(selectedModelNode);
                selectedCustomTool = customTool.get();
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
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));

    m_currentTool->start();
}

void FormEditorView::registerTool(std::unique_ptr<AbstractCustomTool> &&tool)
{
    tool->setView(this);
    m_customTools.push_back(std::move(tool));
}

namespace {

template<typename Tuple>
constexpr bool contains(const Tuple &tuple, AuxiliaryDataKeyView key)
{
    return std::apply([=](const auto &...keys) { return ((key == keys) || ...); }, tuple);
}
} // namespace

void FormEditorView::auxiliaryDataChanged(const ModelNode &node,
                                          AuxiliaryDataKeyView key,
                                          const QVariant &data)
{
    QmlItemNode item(node);

    if (key == invisibleProperty) {
        if (FormEditorItem *item = scene()->itemForQmlItemNode(QmlItemNode(node))) {
            bool isInvisible = data.toBool();
            item->setVisible(!isInvisible);
            ModelNode newNode(node);
            if (isInvisible)
                newNode.deselectNode();
        }
    } else if (item.isFlowTransition() || item.isFlowActionArea() || item.isFlowDecision()
               || item.isFlowWildcard()) {
        if (FormEditorItem *editorItem = m_scene->itemForQmlItemNode(item)) {
            // Update the geomtry if one of the following auxiliary properties has changed
            auto updateGeometryPropertyNames = std::make_tuple(breakPointProperty,
                                                               bezierProperty,
                                                               transitionBezierProperty,
                                                               typeProperty,
                                                               transitionTypeProperty,
                                                               radiusProperty,
                                                               transitionRadiusProperty,
                                                               labelPositionProperty,
                                                               labelFlipSideProperty,
                                                               inOffsetProperty,
                                                               outOffsetProperty,
                                                               blockSizeProperty,
                                                               blockRadiusProperty,
                                                               showDialogLabelProperty,
                                                               dialogLabelPositionProperty);
            if (contains(updateGeometryPropertyNames, key))
                editorItem->updateGeometry();

            editorItem->update();
        }
    } else if (item.isFlowView() || item.isFlowItem()) {
        scene()->update();
    } else if (key == annotationProperty || key == customIdProperty) {
        if (FormEditorItem *editorItem = scene()->itemForQmlItemNode(item)) {
            editorItem->update();
        }
    }

    if (key.name == "FrameColor") {
        if (FormEditorItem *editorItem = scene()->itemForQmlItemNode(item))
            editorItem->setFrameColor(data.value<QColor>());
    }

    if (key == contextImageProperty && !Qml3DNode::isValidVisualRoot(rootModelNode()))
        m_formEditorWidget->setBackgoundImage(data.value<QImage>());
}

static void updateTransitions(FormEditorScene *scene, const QmlItemNode &qmlItemNode)
{
    QmlFlowTargetNode flowItem(qmlItemNode);
    if (flowItem.isValid() && flowItem.flowView().isValid()) {
        const auto nodes = flowItem.flowView().transitions();
        for (const ModelNode &node : nodes) {
            if (FormEditorItem *item = scene->itemForQmlItemNode(node))
                item->updateGeometry();
        }
    };
}

void FormEditorView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    if (Qml3DNode::isValidVisualRoot(rootModelNode())) {
        if (completedNodeList.contains(rootModelNode())) {
            FormEditorItem *item = scene()->itemForQmlItemNode(rootModelNode());
            if (item)
                scene()->synchronizeTransformation(item);
        }
    }

    const bool isFlow = QmlItemNode(rootModelNode()).isFlowView();
    QList<FormEditorItem*> itemNodeList;
    for (const ModelNode &node : completedNodeList) {
        ;
        if (const QmlItemNode qmlItemNode = (node)) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
                scene()->synchronizeParent(qmlItemNode);
                itemNodeList.append(item);
                if (isFlow && qmlItemNode.isFlowItem())
                    updateTransitions(scene(), qmlItemNode);
            }
        }
    }
    currentTool()->instancesCompleted(itemNodeList);
}

namespace {
constexpr AuxiliaryDataKeyView autoSizeProperty{AuxiliaryDataType::Temporary, "autoSize"};
}

void FormEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash)
{
    QList<FormEditorItem *> changedItems;

    QList<ModelNode> informationChangedNodes = Utils::filtered(
        informationChangedHash.keys(),
        [](const ModelNode &node) { return QmlItemNode::isValidQmlItemNode(node); });

    for (const ModelNode &node : informationChangedNodes) {
        const QmlItemNode qmlItemNode(node);
        if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeTransformation(item);
            if (qmlItemNode.isRootModelNode() && informationChangedHash.values(node).contains(Size))
                setupRootItemSize();

            changedItems.append(item);
        }
    }

    scene()->update();

    m_currentTool->formEditorItemsChanged(changedItems);
}

void FormEditorView::instancesRenderImageChanged(const QVector<ModelNode> &nodeList)
{
    for (const ModelNode &node : nodeList) {
        if (QmlItemNode::isValidQmlItemNode(node))
             if (FormEditorItem *item = scene()->itemForQmlItemNode(QmlItemNode(node)))
                 item->update();
        if (Qml3DNode::isValidVisualRoot(node)) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(node))
                item->update();
        }
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

QImage FormEditorView::takeFormEditorScreenshot()
{
    return m_formEditorWidget->takeFormEditorScreenshot();
}

QPicture FormEditorView::renderToPicture() const
{
    return m_formEditorWidget->renderToPicture();
}

void FormEditorView::setupFormEditorWidget()
{
    Q_ASSERT(model());

    Q_ASSERT(m_scene->formLayerItem());

    if (QmlItemNode::isValidQmlItemNode(rootModelNode()))
        setupFormEditorItemTree(rootModelNode());

    if (Qml3DNode::isValidVisualRoot(rootModelNode()))
        setupFormEditor3DView();

    m_formEditorWidget->initialize();

    if (!rewriterView()->errors().isEmpty())
        m_formEditorWidget->showErrorMessageBox(rewriterView()->errors());
    else
        m_formEditorWidget->hideErrorMessageBox();

    if (!rewriterView()->warnings().isEmpty())
        m_formEditorWidget->showWarningMessageBox(rewriterView()->warnings());

    checkRootModelNode();
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
    resetPuppet();
}

void FormEditorView::addOrRemoveFormEditorItem(const ModelNode &node)
{
    // If node is not connected to scene root, don't do anything yet to avoid duplicated effort,
    // as any removal or addition will remove or add descendants as well.
    if (!node.isInHierarchy())
        return;

    QmlItemNode itemNode(node);

    auto removeItemFromScene = [this, &itemNode]() {
        if (FormEditorItem *item = m_scene->itemForQmlItemNode(itemNode)) {
            QList<FormEditorItem *> removed = scene()->itemsForQmlItemNodes(itemNode.allSubModelNodes());
            removed.append(item);
            m_currentTool->itemsAboutToRemoved(removed);
            removeNodeFromScene(itemNode);
        }
    };
    if (hasNodeSourceOrNonItemParent(node)) {
        removeItemFromScene();
    } else if (itemNode.isValid()) {
        if (node.nodeSourceType() == ModelNode::NodeWithoutSource) {
            if (!m_scene->itemForQmlItemNode(itemNode)) {
                setupFormEditorItemTree(itemNode);
                // Simulate selection change to refresh tools
                selectedNodesChanged(selectedModelNodes(), {});
            }
        } else {
            removeItemFromScene();
        }
    }
}

void FormEditorView::checkRootModelNode()
{
    if (m_formEditorWidget->errorMessageBoxIsVisible())
        return;

    QTC_ASSERT(rootModelNode().isValid(), return);

    if (!rootModelNode().metaInfo().isGraphicalItem()
        && !Qml3DNode::isValidVisualRoot(rootModelNode()))
        m_formEditorWidget->showErrorMessageBox(
            {DocumentMessage(tr("%1 is not supported as the root element by the 2D view.")
                                 .arg(rootModelNode().simplifiedTypeName()))});
    else
        m_formEditorWidget->hideErrorMessageBox();
}

void FormEditorView::setupFormEditor3DView()
{
    m_scene->addFormEditorItem(rootModelNode(), FormEditorScene::Preview3d);
    FormEditorItem *item = m_scene->itemForQmlItemNode(rootModelNode());
    item->updateGeometry();
}

void FormEditorView::setupRootItemSize()
{
    if (const QmlItemNode rootQmlNode = rootModelNode()) {
        const int rootElementInitWidth = QmlDesignerPlugin::settings()
                                             .value(DesignerSettingsKey::ROOT_ELEMENT_INIT_WIDTH)
                                             .toInt();
        const int rootElementInitHeight = QmlDesignerPlugin::settings()
                                              .value(DesignerSettingsKey::ROOT_ELEMENT_INIT_HEIGHT)
                                              .toInt();

        if (rootQmlNode.instanceBoundingRect().isEmpty()
            && !(rootQmlNode.propertyAffectedByCurrentState("width")
                 && rootQmlNode.propertyAffectedByCurrentState("height"))) {
            if (!rootModelNode().hasAuxiliaryData(widthProperty))
                rootModelNode().setAuxiliaryData(widthProperty, rootElementInitWidth);
            if (!rootModelNode().hasAuxiliaryData(heightProperty))
                rootModelNode().setAuxiliaryData(heightProperty, rootElementInitHeight);
            rootModelNode().setAuxiliaryData(autoSizeProperty, true);
            formEditorWidget()->updateActions();
        } else {
            if (rootModelNode().hasAuxiliaryData(autoSizeProperty)
                && (rootQmlNode.propertyAffectedByCurrentState("width")
                    || rootQmlNode.propertyAffectedByCurrentState("height"))) {
                rootModelNode().removeAuxiliaryData(widthProperty);
                rootModelNode().removeAuxiliaryData(heightProperty);
                rootModelNode().removeAuxiliaryData(autoSizeProperty);
                formEditorWidget()->updateActions();
            }
        }

        formEditorWidget()->setRootItemRect(rootQmlNode.instanceBoundingRect());
        formEditorWidget()->centerScene();

        auto contextImage = rootModelNode().auxiliaryData(contextImageProperty);

        if (contextImage)
            m_formEditorWidget->setBackgoundImage(contextImage.value().value<QImage>());
    }
}

void FormEditorView::reset()
{
   QTimer::singleShot(200, this, &FormEditorView::delayedReset);
}

void FormEditorView::delayedReset()
{
    m_selectionTool->clear();
    m_rotationTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_dragTool->clear();
    m_scene->clearFormEditorItems();
    if (isAttached() && QmlItemNode::isValidQmlItemNode(rootModelNode()))
        setupFormEditorItemTree(rootModelNode());
}

}

