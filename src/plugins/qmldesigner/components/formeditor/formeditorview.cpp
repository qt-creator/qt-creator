// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorview.h"
#include "abstractcustomtool.h"
#include "dragtool.h"
#include "formeditorgraphicsview.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "formeditortracing.h"
#include "formeditorwidget.h"
#include "movetool.h"
#include "nodeinstanceview.h"
#include "resizetool.h"
#include "rotationtool.h"
#include "selectiontool.h"

#include <qmldesignertr.h>
#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <customnotifications.h>
#include <designersettings.h>
#include <model.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qml3dnode.h>
#include <qmldesignerplugin.h>
#include <rewriterview.h>
#include <variantproperty.h>
#include <zoomaction.h>

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

using NanotraceHR::keyValue;

namespace {
constexpr AuxiliaryDataKeyView autoSizeProperty{AuxiliaryDataType::Temporary, "autoSize"};

using FormEditorTracing::category;
}

FormEditorView::FormEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
    NanotraceHR::Tracer tracer{"form editor view constructor", category()};
}

FormEditorScene* FormEditorView::scene() const
{
    return m_scene.data();
}

FormEditorView::~FormEditorView()
{
    NanotraceHR::Tracer tracer{"form editor view destructor", category()};

    m_currentTool = nullptr;
}

void FormEditorView::modelAttached(Model *model)
{
    NanotraceHR::Tracer tracer{"form editor view model attached", category(), keyValue("model", model)};

    AbstractView::modelAttached(model);

#ifndef QDS_USE_PROJECTSTORAGE
    m_hadIncompleteTypeInformation = model->rewriterView()->hasIncompleteTypeInformation();
#endif

    m_formEditorWidget->setBackgoundImage({});

    temporaryBlockView();
    setupFormEditorWidget();

    setupRootItemSize();
}


//This function does the setup of the initial FormEditorItem tree in the scene
void FormEditorView::setupFormEditorItemTree(const QmlItemNode &qmlItemNode)
{
    NanotraceHR::Tracer tracer{"form editor view setup form editor item tree",
                               category(),
                               keyValue("model node", qmlItemNode)};

    if (!qmlItemNode.hasFormEditorItem())
        return;

    if (!qmlItemNode.isEffectItem()) {
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

void FormEditorView::removeNodeFromScene(const QmlItemNode &qmlItemNode)
{
    NanotraceHR::Tracer tracer{"form editor view remove node from scene",
                               category(),
                               keyValue("model node", qmlItemNode)};

    QList<FormEditorItem *> removedItemList;

    if (qmlItemNode.isValid()) {
        QList<QmlItemNode> nodeList = qmlItemNode.allSubModelNodes();
        nodeList.append(qmlItemNode);
        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
    }

    if (!removedItemList.isEmpty()) {
        m_currentTool->itemsAboutToRemoved(removedItemList);

        //The destructor of QGraphicsItem does delete all its children.
        //We have to keep the children if they are not children in the model anymore.
        //Otherwise we delete the children explicitly anyway.
        deleteWithoutChildren(removedItemList);
    }
}

void FormEditorView::createFormEditorWidget()
{
    NanotraceHR::Tracer tracer{"form editor view create form editor widget", category()};

    m_formEditorWidget = QPointer<FormEditorWidget>(new FormEditorWidget(this));
    m_scene = QPointer<FormEditorScene>(new FormEditorScene(m_formEditorWidget.data(), this));

    m_moveTool = std::make_unique<MoveTool>(this);
    m_selectionTool = std::make_unique<SelectionTool>(this);
    m_rotationTool = std::make_unique<RotationTool>(this);
    m_resizeTool = std::make_unique<ResizeTool>(this);
    m_dragTool = std::make_unique<DragTool>(this);

    m_currentTool = m_selectionTool.get();

    connect(m_formEditorWidget->zoomAction(), &ZoomAction::zoomLevelChanged, [this] {
        m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
    });

    connect(m_formEditorWidget->showBoundingRectAction(), &QAction::toggled, scene(), &FormEditorScene::setShowBoundingRects);
    connect(m_formEditorWidget->resetAction(), &QAction::triggered, this, &FormEditorView::resetNodeInstanceView);
}

void FormEditorView::temporaryBlockView(int duration)
{
    NanotraceHR::Tracer tracer{"form editor view temporary block view",
                               category(),
                               keyValue("duration", duration)};

    m_formEditorWidget->graphicsView()->setUpdatesEnabled(false);
    static auto timer = new QTimer(qApp);
    timer->setSingleShot(true);
    timer->start(duration);

    connect(timer, &QTimer::timeout, this, [this] {
        if (m_formEditorWidget && m_formEditorWidget->graphicsView())
            m_formEditorWidget->graphicsView()->setUpdatesEnabled(true);
    });
}

void FormEditorView::cleanupToolsAndScene()
{
    NanotraceHR::Tracer tracer{"form editor view cleanup tools and scene", category()};

    QTC_ASSERT(m_scene, return);
    QTC_ASSERT(m_formEditorWidget, return );
    QTC_ASSERT(m_currentTool, return );

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
    NanotraceHR::Tracer tracer{"form editor view model about to be detached",
                               category(),
                               keyValue("model", model)};

    rootModelNode().removeAuxiliaryData(contextImageProperty);
    rootModelNode().removeAuxiliaryData(widthProperty);
    rootModelNode().removeAuxiliaryData(heightProperty);
    rootModelNode().removeAuxiliaryData(autoSizeProperty);

    cleanupToolsAndScene();
    AbstractView::modelAboutToBeDetached(model);
}

void FormEditorView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
    NanotraceHR::Tracer tracer{"form editor view imports changed", category()};

    reset();
}

void FormEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    NanotraceHR::Tracer tracer{"form editor view node about to be removed",
                               category(),
                               keyValue("model node", removedNode)};

    const QmlItemNode qmlItemNode(removedNode);

    removeNodeFromScene(qmlItemNode);
}

void FormEditorView::nodeRemoved(const ModelNode &/*removedNode*/,
                                 const NodeAbstractProperty &/*parentProperty*/,
                                 PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"form editor view node removed", category()};

    updateHasEffects();
}

void FormEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    NanotraceHR::Tracer tracer{"form editor view root node type changed", category()};

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
    NanotraceHR::Tracer tracer{"form editor view properties about to be removed", category()};

    QList<FormEditorItem *> removedItems;
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
                }
            }
        }
    }
    m_currentTool->itemsAboutToRemoved(removedItems);
}

static inline bool hasNodeSourceOrNonItemParent(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"form editor view has node source or non-item parent",
                               category(),
                               keyValue("model node", node)};

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
    NanotraceHR::Tracer tracer{"form editor view node reparented",
                               category(),
                               keyValue("model node", node)};

    addOrRemoveFormEditorItem(node);

    updateHasEffects();
}

void FormEditorView::nodeSourceChanged(const ModelNode &node,
                                       [[maybe_unused]] const QString &newNodeSource)
{
    NanotraceHR::Tracer tracer{"form editor view node source changed",
                               category(),
                               keyValue("model node", node)};

    addOrRemoveFormEditorItem(node);
}

WidgetInfo FormEditorView::widgetInfo()
{
    NanotraceHR::Tracer tracer{"form editor view widget info", category()};

    if (!m_formEditorWidget)
        createFormEditorWidget();

    return createWidgetInfo(m_formEditorWidget.data(),
                            "FormEditor",
                            WidgetInfo::CentralPane,
                            Tr::tr("2D"),
                            Tr::tr("2D view"),
                            DesignerWidgetFlags::IgnoreErrors);
}

FormEditorWidget *FormEditorView::formEditorWidget()
{
    return m_formEditorWidget.data();
}

void FormEditorView::nodeIdChanged(const ModelNode& node, const QString &/*newId*/, const QString &/*oldId*/)
{
    NanotraceHR::Tracer tracer{"form editor view node id changed",
                               category(),
                               keyValue("model node", node)};

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
                                          const QList<ModelNode> &)
{
    NanotraceHR::Tracer tracer{"form editor view selected nodes changed", category()};

    m_currentTool->setItems(
        scene()->itemsForQmlItemNodes(toQmlItemNodeListKeppInvalid(selectedNodeList)));

    m_scene->update();

    if (selectedNodeList.empty())
        m_formEditorWidget->zoomSelectionAction()->setEnabled(false);
    else
        m_formEditorWidget->zoomSelectionAction()->setEnabled(true);
}

void FormEditorView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    NanotraceHR::Tracer tracer{"form editor view document messages changed", category()};

    QTC_ASSERT(model(), return);
    QTC_ASSERT(model()->rewriterView(), return);

    if (!errors.isEmpty() && !m_hadIncompleteTypeInformation)
        m_formEditorWidget->showErrorMessageBox(errors);
    else if (rewriterView()->errors().isEmpty())
        m_formEditorWidget->hideErrorMessageBox();

    checkRootModelNode();
}

void FormEditorView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
    NanotraceHR::Tracer tracer{"form editor view custom notification",
                               category(),
                               keyValue("identifier", identifier)};

    if (identifier == QLatin1String("puppet crashed"))
        m_dragTool->clearMoveDelay();
    if (identifier == QLatin1String("reset QmlPuppet"))
        temporaryBlockView();
#ifndef QDS_USE_PROJECTSTORAGE
    if (identifier == UpdateItemlibrary)
        m_hadIncompleteTypeInformation = model()->rewriterView()->hasIncompleteTypeInformation();
#endif
}

void FormEditorView::currentStateChanged(const ModelNode & /*node*/)
{
    NanotraceHR::Tracer tracer{"form editor view current state changed", category()};

    temporaryBlockView();
}

AbstractFormEditorTool *FormEditorView::currentTool() const
{
    NanotraceHR::Tracer tracer{"form editor view current tool", category()};

    return m_currentTool;
}

bool FormEditorView::changeToMoveTool()
{
    NanotraceHR::Tracer tracer{"form editor view change to move tool", category()};

    if (m_currentTool == m_moveTool.get())
        return true;
    if (!isMoveToolAvailable())
        return false;
    changeCurrentToolTo(m_moveTool.get());
    return true;
}

void FormEditorView::changeToDragTool()
{
    NanotraceHR::Tracer tracer{"form editor view change to drag tool", category()};

    if (m_currentTool == m_dragTool.get())
        return;
    changeCurrentToolTo(m_dragTool.get());
}


bool FormEditorView::changeToMoveTool(const QPointF &beginPoint)
{
    NanotraceHR::Tracer tracer{"form editor view change to move tool with point", category()};

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
    NanotraceHR::Tracer tracer{"form editor view change to selection tool", category()};

    if (m_currentTool == m_selectionTool.get())
        return;
    changeCurrentToolTo(m_selectionTool.get());
}

void FormEditorView::changeToSelectionTool(QGraphicsSceneMouseEvent *event)
{
    NanotraceHR::Tracer tracer{"form editor view change to selection tool with event", category()};

    if (m_currentTool == m_selectionTool.get())
        return;
    changeCurrentToolTo(m_selectionTool.get());
    m_selectionTool->selectUnderPoint(event);
}

void FormEditorView::resetToSelectionTool()
{
    NanotraceHR::Tracer tracer{"form editor view reset to selection tool", category()};

    changeCurrentToolTo(m_selectionTool.get());
}

void FormEditorView::changeToRotationTool()
{
    NanotraceHR::Tracer tracer{"form editor view change to rotation tool", category()};

    if (m_currentTool == m_rotationTool.get())
        return;
    changeCurrentToolTo(m_rotationTool.get());
}

void FormEditorView::changeToResizeTool()
{
    NanotraceHR::Tracer tracer{"form editor view change to resize tool", category()};

    if (m_currentTool == m_resizeTool.get())
        return;
    changeCurrentToolTo(m_resizeTool.get());
}

void FormEditorView::changeToTransformTools()
{
    NanotraceHR::Tracer tracer{"form editor view change to transform tools", category()};

    if (m_currentTool == m_moveTool.get() || m_currentTool == m_resizeTool.get()
        || m_currentTool == m_rotationTool.get() || m_currentTool == m_selectionTool.get())
        return;
    changeToSelectionTool();
}

void FormEditorView::changeToCustomTool()
{
    NanotraceHR::Tracer tracer{"form editor view change to custom tool", category()};

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
    NanotraceHR::Tracer tracer{"form editor view change current tool to", category()};

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = newTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));

    m_currentTool->start();
}

void FormEditorView::registerTool(std::unique_ptr<AbstractCustomTool> &&tool)
{
    NanotraceHR::Tracer tracer{"form editor view register tool", category()};

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
    NanotraceHR::Tracer tracer{"form editor view auxiliary data changed",
                               category(),
                               keyValue("model node", node)};
    QmlItemNode item(node);

    if (key == invisibleProperty) {
        if (FormEditorItem *item = scene()->itemForQmlItemNode(QmlItemNode(node))) {
            bool isInvisible = data.toBool();
            item->setVisible(!isInvisible);
            ModelNode newNode(node);
            if (isInvisible)
                newNode.deselectNode();
        }
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

void FormEditorView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    NanotraceHR::Tracer tracer{"form editor view instances completed", category()};

    if (Qml3DNode::isValidVisualRoot(rootModelNode())) {
        if (completedNodeList.contains(rootModelNode())) {
            FormEditorItem *item = scene()->itemForQmlItemNode(rootModelNode());
            if (item)
                scene()->synchronizeTransformation(item);
        }
    }

    QList<FormEditorItem*> itemNodeList;
    for (const ModelNode &node : completedNodeList) {
        if (node) {
            const QmlItemNode qmlItemNode = node;
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
    NanotraceHR::Tracer tracer{"form editor view instance information changed", category()};

    QList<FormEditorItem *> changedItems;

    QList<ModelNode> informationChangedNodes = Utils::filtered(informationChangedHash.keys(),
                                                               [](const ModelNode &node) {
                                                                   return node.isValid();
                                                               });

    for (const ModelNode &node : informationChangedNodes) {
        const QmlItemNode qmlItemNode(node);
        if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeTransformation(item);
            if (qmlItemNode.isRootModelNode()) {
                const auto nodeValues = informationChangedHash.values(node);
                if (nodeValues.contains(Size) || nodeValues.contains(ImplicitSize))
                    setupRootItemSize();
            }
            changedItems.append(item);
        }
    }

    scene()->update();

    m_currentTool->formEditorItemsChanged(changedItems);
}

void FormEditorView::instancesRenderImageChanged(const QVector<ModelNode> &nodeList)
{
    NanotraceHR::Tracer tracer{"form editor view instances render image changed", category()};

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
    NanotraceHR::Tracer tracer{"form editor view instances children changed", category()};

    QList<FormEditorItem *> changedItems;

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
    NanotraceHR::Tracer tracer{"form editor view rewriter begin transaction", category()};

    m_transactionCounter++;
}

void FormEditorView::rewriterEndTransaction()
{
    NanotraceHR::Tracer tracer{"form editor view rewriter end transaction", category()};

    m_transactionCounter--;
}

double FormEditorView::containerPadding() const
{
    NanotraceHR::Tracer tracer{"form editor view container padding", category()};

    return m_formEditorWidget->containerPadding();
}

double FormEditorView::spacing() const
{
    NanotraceHR::Tracer tracer{"form editor view spacing", category()};

    return m_formEditorWidget->spacing();
}

void FormEditorView::gotoError(int line, int column)
{
    NanotraceHR::Tracer tracer{"form editor view goto error",
                               category(),
                               keyValue("line", line),
                               keyValue("column", column)};

    if (m_gotoErrorCallback)
        m_gotoErrorCallback(line, column);
}

void FormEditorView::setGotoErrorCallback(std::function<void (int, int)> gotoErrorCallback)
{
    NanotraceHR::Tracer tracer{"form editor view set goto error callback", category()};

    m_gotoErrorCallback = gotoErrorCallback;
}

void FormEditorView::exportAsImage()
{
    NanotraceHR::Tracer tracer{"form editor view export as image", category()};

    m_formEditorWidget->exportAsImage(m_scene->rootFormEditorItem()->boundingRect());
}

QImage FormEditorView::takeFormEditorScreenshot()
{
    NanotraceHR::Tracer tracer{"form editor view take form editor screenshot", category()};

    return m_formEditorWidget->takeFormEditorScreenshot();
}

QPicture FormEditorView::renderToPicture() const
{
    NanotraceHR::Tracer tracer{"form editor view render to picture", category()};

    return m_formEditorWidget->renderToPicture();
}

void FormEditorView::setupFormEditorWidget()
{
    NanotraceHR::Tracer tracer{"form editor view setup form editor widget", category()};

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

    updateHasEffects();
}

QmlItemNode findRecursiveQmlItemNode(const QmlObjectNode &firstQmlObjectNode)
{
    NanotraceHR::Tracer tracer{"form editor view find recursive qml item node",
                               category(),
                               keyValue("first qml object node", firstQmlObjectNode)};

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
    NanotraceHR::Tracer tracer{"form editor view instance property changed", category()};

    QList<FormEditorItem *> changedItems;
    bool needEffectUpdate = false;
    for (auto &nodePropertyPair : propertyList) {
        const QmlItemNode qmlItemNode(nodePropertyPair.first);
        const PropertyName propertyName = nodePropertyPair.second;
        if (qmlItemNode.modelNode().isValid()) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
                static const PropertyNameList skipList({"x", "y", "width", "height"});
                if (!skipList.contains(propertyName)) {
                    m_scene->synchronizeOtherProperty(item, propertyName);
                    changedItems.append(item);
                }
            } else if (propertyName == "visible" && qmlItemNode.isEffectItem()) {
                needEffectUpdate = true;
            }
        }
    }
    m_currentTool->formEditorItemsChanged(changedItems);
    if (needEffectUpdate)
        updateHasEffects();
}

bool FormEditorView::isMoveToolAvailable() const
{
    NanotraceHR::Tracer tracer{"form editor view is move tool available", category()};

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
    NanotraceHR::Tracer tracer{"form editor view reset node instance view", category()};

    resetPuppet();
}

void FormEditorView::addOrRemoveFormEditorItem(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"form editor view add or remove form editor item",
                               category(),
                               keyValue("model node", node)};

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
    NanotraceHR::Tracer tracer{"form editor view check root model node", category()};

    if (m_formEditorWidget->errorMessageBoxIsVisible())
        return;

    QTC_ASSERT(rootModelNode().isValid(), return);

    if (!rootModelNode().metaInfo().isGraphicalItem()
        && !Qml3DNode::isValidVisualRoot(rootModelNode()))
        m_formEditorWidget->showErrorMessageBox(
            {DocumentMessage(Tr::tr("%1 is not supported as the root element by the 2D view.")
                                 .arg(rootModelNode().simplifiedTypeName()))});
    else
        m_formEditorWidget->hideErrorMessageBox();
}

void FormEditorView::setupFormEditor3DView()
{
    NanotraceHR::Tracer tracer{"form editor view setup form editor 3D view", category()};

    m_scene->addFormEditorItem(rootModelNode(), FormEditorScene::Preview3d);
    FormEditorItem *item = m_scene->itemForQmlItemNode(rootModelNode());
    item->updateGeometry();
}

void FormEditorView::setupRootItemSize()
{
    NanotraceHR::Tracer tracer{"form editor view setup root item size", category()};

    if (const QmlItemNode rootQmlNode = rootModelNode()) {
        int rootElementInitWidth = designerSettings().rootElementInitWidth();
        int rootElementInitHeight = designerSettings().rootElementInitHeight();

        if (rootModelNode().hasAuxiliaryData(defaultWidthProperty))
            rootElementInitWidth = rootModelNode().auxiliaryData(defaultWidthProperty).value().toInt();
        if (rootModelNode().hasAuxiliaryData(defaultHeightProperty))
            rootElementInitHeight = rootModelNode().auxiliaryData(defaultHeightProperty).value().toInt();

        bool affectedByCurrentState = rootQmlNode.propertyAffectedByCurrentState("width")
                                      || rootQmlNode.propertyAffectedByCurrentState("height")
                                      || rootQmlNode.propertyAffectedByCurrentState("implicitWidth")
                                      || rootQmlNode.propertyAffectedByCurrentState("implicitHeight");

        QRectF rootRect = rootQmlNode.instanceBoundingRect();
        if (rootRect.isEmpty() && !affectedByCurrentState) {

            if (!rootModelNode().hasAuxiliaryData(widthProperty))
                rootModelNode().setAuxiliaryData(widthProperty, rootElementInitWidth);

            if (!rootModelNode().hasAuxiliaryData(heightProperty))
                rootModelNode().setAuxiliaryData(heightProperty, rootElementInitHeight);

            rootModelNode().setAuxiliaryData(autoSizeProperty, true);

            formEditorWidget()->updateActions();
            rootRect.setWidth(rootModelNode().auxiliaryData(widthProperty).value().toFloat() );
            rootRect.setHeight(rootModelNode().auxiliaryData(heightProperty).value().toFloat() );

        } else if (rootModelNode().hasAuxiliaryData(autoSizeProperty) && affectedByCurrentState ) {
            rootModelNode().removeAuxiliaryData(widthProperty);
            rootModelNode().removeAuxiliaryData(heightProperty);
            rootModelNode().removeAuxiliaryData(autoSizeProperty);
            formEditorWidget()->updateActions();
        }

        formEditorWidget()->setRootItemRect(rootRect);
        formEditorWidget()->centerScene();

        if (auto contextImage = rootModelNode().auxiliaryData(contextImageProperty))
            formEditorWidget()->setBackgoundImage(contextImage.value().value<QImage>());
    }
}

void FormEditorView::updateHasEffects()
{
    NanotraceHR::Tracer tracer{"form editor view update has effects", category()};

    if (model()) {
        const QList<ModelNode> nodes = allModelNodes();
        for (const auto &node : nodes) {
            QmlItemNode qmlNode(node);
            FormEditorItem *item = m_scene->itemForQmlItemNode(qmlNode);
            if (item)
                item->setHasEffect(false);
            if (qmlNode.isEffectItem() && qmlNode.instanceIsVisible()) {
                FormEditorItem *parentItem = m_scene->itemForQmlItemNode(qmlNode.modelParentItem());
                if (parentItem)
                    parentItem->setHasEffect(true);
            }
        }
    }
}

void FormEditorView::reset()
{
    NanotraceHR::Tracer tracer{"form editor view reset", category()};

    QTimer::singleShot(200, this, &FormEditorView::delayedReset);
}

void FormEditorView::delayedReset()
{
    NanotraceHR::Tracer tracer{"form editor view delayed reset", category()};

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

