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
#include "selectiontool.h"
#include "movetool.h"
#include "resizetool.h"
#include "dragtool.h"
#include "formeditorwidget.h"
#include <formeditorgraphicsview.h>
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "abstractcustomtool.h"

#include <designmodecontext.h>
#include <modelnode.h>
#include <model.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <rewriterview.h>
#include <zoomaction.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>

#include <QDebug>
#include <QPair>
#include <QString>
#include <QTimer>

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

    AbstractView::modelAttached(model);
    temporaryBlockView();

    Q_ASSERT(m_scene->formLayerItem());

    if (QmlItemNode::isValidQmlItemNode(rootModelNode()))
        setupFormEditorItemTree(rootModelNode());

    m_formEditorWidget->updateActions();

    if (!rewriterView()->errors().isEmpty())
        formEditorWidget()->showErrorMessageBox(rewriterView()->errors());
    else
        formEditorWidget()->hideErrorMessageBox();

    if (!rewriterView()->warnings().isEmpty())
        formEditorWidget()->showWarningMessageBox(rewriterView()->warnings());
}


//This function does the setup of the initial FormEditorItem tree in the scene
void FormEditorView::setupFormEditorItemTree(const QmlItemNode &qmlItemNode)
{
    m_scene->addFormEditorItem(qmlItemNode);

    foreach (const QmlObjectNode &nextNode, qmlItemNode.allDirectSubNodes()) //TODO instance children
        //If the node has source for components/custom parsers we ignore it.
        if (QmlItemNode(nextNode).isValid() && nextNode.modelNode().nodeSourceType() == ModelNode::NodeWithoutSource)
            setupFormEditorItemTree(nextNode.toQmlItemNode());
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

void FormEditorView::removeNodeFromScene(const QmlItemNode &qmlItemNode)
{
    if (qmlItemNode.isValid()) {
        QList<QmlItemNode> nodeList;
        nodeList.append(qmlItemNode.allSubModelNodes());
        nodeList.append(qmlItemNode);

        QList<FormEditorItem*> removedItemList;

        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
        m_currentTool->itemsAboutToRemoved(removedItemList);

        //The destructor of QGraphicsItem does delete all its children.
        //We have to keep the children if they are not children in the model anymore.
        //Otherwise we delete the children explicitly anyway.
        deleteWithoutChildren(removedItemList);
    }
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

    m_moveTool.reset(new MoveTool(this));
    m_selectionTool.reset(new SelectionTool(this));
    m_resizeTool.reset(new ResizeTool(this));
    m_dragTool.reset(new DragTool(this));

    m_currentTool = m_selectionTool.get();

    Internal::FormEditorContext *formEditorContext = new Internal::FormEditorContext(m_formEditorWidget.data());
    Core::ICore::addContextObject(formEditorContext);

    connect(formEditorWidget()->zoomAction(), &ZoomAction::zoomLevelChanged, [this]() {
        m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
    });
    connect(formEditorWidget()->showBoundingRectAction(), &QAction::toggled,
            scene(), &FormEditorScene::setShowBoundingRects);
}

void FormEditorView::temporaryBlockView()
{
    formEditorWidget()->graphicsView()->setBlockPainting(true);

    QTimer::singleShot(1000, this, [this]() {
        formEditorWidget()->graphicsView()->setBlockPainting(false);

    });
}

void FormEditorView::nodeCreated(const ModelNode &node)
{
    //If the node has source for components/custom parsers we ignore it.
    if (QmlItemNode::isValidQmlItemNode(node) && node.nodeSourceType() == ModelNode::NodeWithoutSource) //only setup QmlItems
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
        item->setParentItem(0);
        item->setParent(0);
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

    return createWidgetInfo(m_formEditorWidget.data(), 0, "FormEditor", WidgetInfo::CentralPane, 0, tr("Form Editor"), DesignerWidgetFlags::IgnoreErrors);
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
        if (item)
            item->update();
    }
}

void FormEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedNodeList)));

    m_scene->update();
}

void FormEditorView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (!errors.isEmpty())
        formEditorWidget()->showErrorMessageBox(errors);
    else
        formEditorWidget()->hideErrorMessageBox();
}

void FormEditorView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
    if (identifier == QStringLiteral("puppet crashed"))
        m_dragTool->clearMoveDelay();
    if (identifier == QStringLiteral("reset QmlPuppet"))
        temporaryBlockView();
}

AbstractFormEditorTool* FormEditorView::currentTool() const
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
        AbstractCustomTool *selectedCustomTool = 0;

        ModelNode selectedModelNode = selectedModelNodes().first();

        foreach (AbstractCustomTool *customTool, m_customToolList) {
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
}

void FormEditorView::registerTool(AbstractCustomTool *tool)
{
    tool->setView(this);
    m_customToolList.append(tool);
}

void FormEditorView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
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
    }
}

void FormEditorView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    QList<FormEditorItem*> itemNodeList;
    foreach (const ModelNode &node, completedNodeList) {
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

    QList<ModelNode> informationChangedNodes = Utils::filtered(informationChangedHash.keys(), [](const ModelNode &node) {
        return QmlItemNode::isValidQmlItemNode(node);
    });

    foreach (const ModelNode &node, informationChangedNodes) {
        const QmlItemNode qmlItemNode(node);
        if (FormEditorItem *item = scene()->itemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeTransformation(item);
            if (qmlItemNode.isRootModelNode() && informationChangedHash.values(node).contains(Size)) {
                if (qmlItemNode.instanceBoundingRect().isEmpty() &&
                        !(qmlItemNode.propertyAffectedByCurrentState("width")
                          && qmlItemNode.propertyAffectedByCurrentState("height"))) {
                    if (!(rootModelNode().hasAuxiliaryData("width")))
                        rootModelNode().setAuxiliaryData("width", 640);
                    if (!(rootModelNode().hasAuxiliaryData("height")))
                        rootModelNode().setAuxiliaryData("height", 480);
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
    foreach (const ModelNode &node, nodeList) {
        if (QmlItemNode::isValidQmlItemNode(node))
             if (FormEditorItem *item = scene()->itemForQmlItemNode(QmlItemNode(node)))
                 item->update();
    }
}

void FormEditorView::instancesChildrenChanged(const QVector<ModelNode> &nodeList)
{
    QList<FormEditorItem*> changedItems;

    foreach (const ModelNode &node, nodeList) {
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
    foreach (auto &nodePropertyPair, propertyList) {
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

