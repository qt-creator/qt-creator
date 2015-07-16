/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "formeditorview.h"
#include "selectiontool.h"
#include "movetool.h"
#include "resizetool.h"
#include "dragtool.h"
#include "formeditorwidget.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "abstractcustomtool.h"

#include <designmodecontext.h>
#include <modelnode.h>
#include <model.h>
#include <QDebug>
#include <QPair>
#include <QString>
#include <QTimer>
#include <zoomaction.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>

#include <coreplugin/icore.h>

namespace QmlDesigner {

FormEditorView::FormEditorView(QObject *parent)
    : AbstractView(parent),
      m_formEditorWidget(new FormEditorWidget(this)),
      m_scene(new FormEditorScene(m_formEditorWidget.data(), this)),
      m_moveTool(new MoveTool(this)),
      m_selectionTool(new SelectionTool(this)),
      m_resizeTool(new ResizeTool(this)),
      m_dragTool(new DragTool(this)),
      m_currentTool(m_selectionTool),
      m_transactionCounter(0)
{
    Internal::FormEditorContext *formEditorContext = new Internal::FormEditorContext(m_formEditorWidget.data());
    Core::ICore::addContextObject(formEditorContext);

    connect(formEditorWidget()->zoomAction(), SIGNAL(zoomLevelChanged(double)), SLOT(updateGraphicsIndicators()));
    connect(formEditorWidget()->showBoundingRectAction(), SIGNAL(toggled(bool)), scene(), SLOT(setShowBoundingRects(bool)));
}

FormEditorScene* FormEditorView::scene() const
{
    return m_scene.data();
}

FormEditorView::~FormEditorView()
{
    delete m_selectionTool;
    m_selectionTool = 0;
    delete m_moveTool;
    m_moveTool = 0;
    delete m_resizeTool;
    m_resizeTool = 0;
    delete m_dragTool;
    m_dragTool = 0;

    qDeleteAll(m_customToolList);

    // delete scene after tools to prevent double deletion
    // of items
    delete m_scene.data();
    delete m_formEditorWidget.data();
}

void FormEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);

    AbstractView::modelAttached(model);

    Q_ASSERT(m_scene->formLayerItem());

    if (QmlItemNode::isValidQmlItemNode(rootModelNode()))
        setupFormEditorItemTree(rootModelNode());

    m_formEditorWidget->updateActions();
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
    if (qmlItemNode.isValid()) {

        FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode);

        QList<QmlItemNode> nodeList;
        nodeList.append(qmlItemNode.allSubModelNodes());
        nodeList.append(qmlItemNode);

        QList<FormEditorItem*> removedItemList;
        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
        m_currentTool->itemsAboutToRemoved(removedItemList);
        item->setFormEditorVisible(false);
    }
}

void FormEditorView::nodeCreated(const ModelNode &createdNode)
{
    ModelNode node(createdNode);
    //If the node has source for components/custom parsers we ignore it.
    if (QmlItemNode::isValidQmlItemNode(node) && node.nodeSourceType() == ModelNode::NodeWithoutSource) //only setup QmlItems
        setupFormEditorItemTree(QmlItemNode(node));
}

void FormEditorView::modelAboutToBeDetached(Model *model)
{
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

    m_currentTool = m_selectionTool;

    AbstractView::modelAboutToBeDetached(model);
}

void FormEditorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    reset();
}

void FormEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    QmlItemNode qmlItemNode(removedNode);

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
    foreach (const AbstractProperty &property, propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty = property.toNodeAbstractProperty();
            QList<FormEditorItem*> removedItemList;

            foreach (const ModelNode &modelNode, nodeAbstractProperty.allSubNodes()) {
                QmlItemNode qmlItemNode(modelNode);

                if (qmlItemNode.isValid() && m_scene->hasItemForQmlItemNode(qmlItemNode)) {
                    FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode);
                    removedItemList.append(item);

                    delete item;
                }
            }

            m_currentTool->itemsAboutToRemoved(removedItemList);
        }
    }
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
    return createWidgetInfo(m_formEditorWidget.data(), 0, "FormEditor", WidgetInfo::CentralPane, 0, tr("Form Editor"));
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
        item->update();
    }
}

void FormEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedNodeList)));

    m_scene->update();
}

void FormEditorView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
    if (identifier == QStringLiteral("puppet crashed"))
        m_dragTool->clearMoveDelay();
}

AbstractFormEditorTool* FormEditorView::currentTool() const
{
    return m_currentTool;
}

bool FormEditorView::changeToMoveTool()
{
    if (m_currentTool == m_moveTool)
        return true;

    if (!isMoveToolAvailable())
        return false;

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_moveTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
    return true;
}

void FormEditorView::changeToDragTool()
{
    if (m_currentTool == m_dragTool)
        return;

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_dragTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
}


bool FormEditorView::changeToMoveTool(const QPointF &beginPoint)
{
    if (m_currentTool == m_moveTool)
        return true;

    if (!isMoveToolAvailable())
        return false;

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_moveTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
    m_moveTool->beginWithPoint(beginPoint);
    return true;
}

void FormEditorView::changeToSelectionTool()
{
    if (m_currentTool == m_selectionTool)
        return;

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
}

void FormEditorView::changeToSelectionTool(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool == m_selectionTool)
        return;

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));

    m_selectionTool->selectUnderPoint(event);
}

void FormEditorView::changeToResizeTool()
{
    if (m_currentTool == m_resizeTool)
        return;

    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = m_resizeTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
}

void FormEditorView::changeToTransformTools()
{
    if (m_currentTool == m_moveTool ||
       m_currentTool == m_resizeTool ||
       m_currentTool == m_selectionTool)
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

        if (handlingRank > 0) {
            m_scene->updateAllFormEditorItems();
            m_currentTool->clear();
            if (selectedCustomTool) {
                m_currentTool = selectedCustomTool;
                m_currentTool->clear();
                m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
            }
        }
    }
}

void FormEditorView::changeToCustomTool(AbstractCustomTool *customTool)
{
    m_scene->updateAllFormEditorItems();
    m_currentTool->clear();
    m_currentTool = customTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedModelNodes())));
}

void FormEditorView::registerTool(AbstractCustomTool *tool)
{
    tool->setView(this);
    m_customToolList.append(tool);
}

void FormEditorView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
    AbstractView::auxiliaryDataChanged(node, name, data);
    if (name == "invisible" && m_scene->hasItemForQmlItemNode(QmlItemNode(node))) {
        FormEditorItem *item(m_scene->itemForQmlItemNode(QmlItemNode(node)));
        bool isInvisible = data.toBool();
        if (item->isFormEditorVisible())
            item->setVisible(!isInvisible);
        ModelNode newNode(node);
        if (isInvisible)
            newNode.deselectNode();
    }
}

void FormEditorView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    QList<FormEditorItem*> itemNodeList;
    foreach (const ModelNode &node, completedNodeList) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
            itemNodeList.append(scene()->itemForQmlItemNode(qmlItemNode));
        }
    }
    currentTool()->instancesCompleted(itemNodeList);
}

void FormEditorView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    QList<FormEditorItem*> itemNodeList;

    foreach (const ModelNode &node, informationChangeHash.keys()) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeTransformation(qmlItemNode);
            if (qmlItemNode.isRootModelNode() && informationChangeHash.values(node).contains(Size)) {
                if (qmlItemNode.instanceBoundingRect().isEmpty() &&
                        !(qmlItemNode.propertyAffectedByCurrentState("width")
                          && qmlItemNode.propertyAffectedByCurrentState("height"))) {
                    rootModelNode().setAuxiliaryData("width", 640);
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

            itemNodeList.append(scene()->itemForQmlItemNode(qmlItemNode));
        }
    }

    m_currentTool->formEditorItemsChanged(itemNodeList);
}

void FormEditorView::instancesRenderImageChanged(const QVector<ModelNode> &nodeList)
{
    foreach (const ModelNode &node, nodeList) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode))
           scene()->itemForQmlItemNode(qmlItemNode)->update();
    }
}

void FormEditorView::instancesChildrenChanged(const QVector<ModelNode> &nodeList)
{
    QList<FormEditorItem*> itemNodeList;

    foreach (const ModelNode &node, nodeList) {
        QmlItemNode qmlItemNode(node);
        if (qmlItemNode.isValid() && scene()->hasItemForQmlItemNode(qmlItemNode)) {
            scene()->synchronizeParent(qmlItemNode);
            itemNodeList.append(scene()->itemForQmlItemNode(qmlItemNode));
        }
    }

    m_currentTool->formEditorItemsChanged(itemNodeList);
    m_currentTool->instancesParentChanged(itemNodeList);
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

QList<ModelNode> FormEditorView::adjustStatesForModelNodes(const QList<ModelNode> &nodeList) const
{
    QList<ModelNode> adjustedNodeList;
    foreach (const ModelNode &node, nodeList)
        adjustedNodeList.append(node);

    return adjustedNodeList;
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

void FormEditorView::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    typedef QPair<ModelNode, PropertyName> NodePropertyPair;
    foreach (const NodePropertyPair &nodePropertyPair, propertyList) {
        const QmlItemNode itemNode(nodePropertyPair.first);
        const PropertyName propertyName = nodePropertyPair.second;
        if (itemNode.isValid() && scene()->hasItemForQmlItemNode(itemNode)) {
            static PropertyNameList skipList = PropertyNameList() << "x" << "y" << "width" << "height";
            if (!skipList.contains(propertyName)) {
                m_scene->synchronizeOtherProperty(itemNode, propertyName);
                m_currentTool->formEditorItemsChanged(QList<FormEditorItem*>() << m_scene->itemForQmlItemNode(itemNode));
            }
        }
    }
}

void FormEditorView::updateGraphicsIndicators()
{
    m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
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
   QTimer::singleShot(200, this, SLOT(delayedReset()));
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

