/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "selectiontool.h"
#include "movetool.h"
#include "resizetool.h"
#include "anchortool.h"
#include "dragtool.h"
#include "itemcreatortool.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "formeditornodeinstanceview.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include <rewritertransaction.h>
#include <modelnode.h>
#include <modelutilities.h>
#include <itemlibraryinfo.h>
#include <metainfo.h>
#include <model.h>
#include <QApplication>
#include <QtDebug>
#include <QPair>
#include <QString>
#include <QDir>
#include <QAction>
#include <zoomaction.h>

namespace QmlDesigner {

FormEditorView::FormEditorView(QObject *parent)
    : QmlModelView(parent),
      m_formEditorWidget(new FormEditorWidget(this)),
      m_scene(new FormEditorScene(m_formEditorWidget.data(), this)),
      m_moveTool(new MoveTool(this)),
      m_selectionTool(new SelectionTool(this)),
      m_resizeTool(new ResizeTool(this)),
      m_anchorTool(new AnchorTool(this)),
      m_dragTool(new DragTool(this)),
      m_itemCreatorTool(new ItemCreatorTool(this)),
      m_currentTool(m_selectionTool)
{
    connect(widget()->zoomAction(), SIGNAL(zoomLevelChanged(double)), SLOT(updateGraphicsIndicators()));
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
    delete m_anchorTool;
    m_anchorTool = 0;
    delete m_dragTool;
    m_dragTool = 0;


    // delete scene after tools to prevent double deletion
    // of items
    delete m_scene.data();
    delete m_formEditorWidget.data();
}

void FormEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);

    QmlModelView::modelAttached(model);

    Q_ASSERT(m_scene->formLayerItem());

    if (rootQmlObjectNode().toQmlItemNode().isValid())
        setupFormEditorItemTree(rootQmlObjectNode().toQmlItemNode());
}


//This method does the setup of the initial FormEditorItem tree in the scene
void FormEditorView::setupFormEditorItemTree(const QmlItemNode &qmlItemNode)
{
    m_scene->addFormEditorItem(qmlItemNode);

    foreach (const QmlItemNode &nextNode, qmlItemNode.children()) //TODO instance children
        setupFormEditorItemTree(nextNode);
}

void FormEditorView::nodeCreated(const ModelNode &createdNode)
{
    QmlModelView::nodeCreated(createdNode);
    ModelNode node(createdNode);
    if (QmlItemNode(node).isValid()) //only setup QmlItems
        setupFormEditorItemTree(QmlItemNode(node));
}

void FormEditorView::modelAboutToBeDetached(Model *model)
{
    m_selectionTool->clear();
    m_moveTool->clear();
    m_resizeTool->clear();
    m_anchorTool->clear();
    m_dragTool->clear();
    m_scene->clearFormEditorItems();

    QmlModelView::modelAboutToBeDetached(model);
}

void FormEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    QmlItemNode qmlItemNode(removedNode);

    if (qmlItemNode.isValid()) {

        FormEditorItem *item = m_scene->itemForQmlItemNode(qmlItemNode);

        QList<QmlItemNode> nodeList;
        nodeList.append(qmlItemNode.allSubModelNodes());
        nodeList.append(qmlItemNode);

        QList<FormEditorItem*> removedItemList;
        removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
        m_currentTool->itemsAboutToRemoved(removedItemList);

        delete item;
    }

    QmlModelView::nodeAboutToBeRemoved(removedNode);
}

 void FormEditorView::nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion)
 {
     QmlItemNode oldItemNode(node);
     if (oldItemNode.isValid() && m_scene->hasItemForQmlItemNode(oldItemNode)) {
         FormEditorItem *item = m_scene->itemForQmlItemNode(oldItemNode);

         QList<QmlItemNode> nodeList;
         nodeList.append(oldItemNode.allSubModelNodes());
         nodeList.append(oldItemNode);

         QList<FormEditorItem*> removedItemList;
         removedItemList.append(scene()->itemsForQmlItemNodes(nodeList));
         m_currentTool->itemsAboutToRemoved(removedItemList);

         delete item;
     }

     QmlModelView::nodeTypeChanged(node, type, majorVersion, minorVersion);

     QmlItemNode newItemNode(node);
     if (newItemNode.isValid()) //only setup QmlItems
         setupFormEditorItemTree(newItemNode);

     m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
 }

void FormEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesAboutToBeRemoved(propertyList);
}
void FormEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    QmlModelView::nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);
}

void FormEditorView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::variantPropertiesChanged(propertyList, propertyChange);
}

void FormEditorView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::bindingPropertiesChanged(propertyList, propertyChange);
}

FormEditorWidget *FormEditorView::widget() const
{
    Q_ASSERT(!m_formEditorWidget.isNull());
    return m_formEditorWidget.data();
}

void FormEditorView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    nodeInstanceView()->nodeIdChanged(node, newId, oldId);
}

void FormEditorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &lastSelectedNodeList)
{
    QmlModelView::selectedNodesChanged(selectedNodeList, lastSelectedNodeList);

    m_currentTool->setItems(scene()->itemsForQmlItemNodes(toQmlItemNodeList(selectedNodeList)));
    if (scene()->paintMode() == FormEditorScene::AnchorMode) {
        foreach (FormEditorItem *item, m_scene->itemsForQmlItemNodes(toQmlItemNodeList(selectedNodeList)))
            item->update();

        foreach (FormEditorItem *item, m_scene->itemsForQmlItemNodes(toQmlItemNodeList(lastSelectedNodeList)))
            item->update();
    }

    m_scene->update();
}

AbstractFormEditorTool* FormEditorView::currentTool() const
{
    return m_currentTool;
}

void FormEditorView::changeToMoveTool()
{
    if (m_currentTool == m_moveTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::SizeAllCursor);
    m_currentTool->clear();
    m_currentTool = m_moveTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToDragTool()
{
    if (m_currentTool == m_dragTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::ArrowCursor);
    setCurrentState(baseState());
    m_currentTool->clear();
    m_currentTool = m_dragTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}


void FormEditorView::changeToMoveTool(const QPointF &beginPoint)
{
    if (m_currentTool == m_moveTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::SizeAllCursor);
    m_currentTool->clear();
    m_currentTool = m_moveTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
    m_moveTool->beginWithPoint(beginPoint);
}

void FormEditorView::changeToSelectionTool()
{
    if (m_currentTool == m_selectionTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::ArrowCursor);
    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToItemCreatorTool()
{
    if(m_currentTool == m_itemCreatorTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::CrossCursor);
    setCurrentState(baseState());
    m_currentTool->clear();
    m_currentTool = m_itemCreatorTool;
    m_currentTool->clear();
    setSelectedQmlItemNodes(QList<QmlItemNode>());
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToSelectionTool(QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool == m_selectionTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::ArrowCursor);
    m_currentTool->clear();
    m_currentTool = m_selectionTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));

    m_selectionTool->selectUnderPoint(event);
}

void FormEditorView::changeToResizeTool()
{
    if (m_currentTool == m_resizeTool)
        return;

    scene()->setPaintMode(FormEditorScene::NormalMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::ArrowCursor);
    m_currentTool->clear();
    m_currentTool = m_resizeTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToAnchorTool()
{
    if (m_currentTool == m_anchorTool)
        return;

    scene()->setPaintMode(FormEditorScene::AnchorMode);
    m_scene->updateAllFormEditorItems();
    setCursor(Qt::ArrowCursor);
    m_currentTool->clear();
    m_currentTool = m_anchorTool;
    m_currentTool->clear();
    m_currentTool->setItems(scene()->itemsForQmlItemNodes(selectedQmlItemNodes()));
}

void FormEditorView::changeToTransformTools()
{
    if (m_currentTool == m_moveTool ||
       m_currentTool == m_resizeTool ||
       m_currentTool == m_selectionTool)
        return;

    changeToSelectionTool();
}

void FormEditorView::setCursor(const QCursor &cursor)
{
    m_formEditorWidget->setCursor(cursor);
}

bool FormEditorView::isSnapButtonChecked() const
{
    return m_formEditorWidget->isSnapButtonChecked();
}


void FormEditorView::nodeSlidedToIndex(const NodeListProperty &listProperty, int /*newIndex*/, int /*oldIndex*/)
{
    QList<ModelNode> newOrderModelNodeList = listProperty.toModelNodeList();
    foreach(const ModelNode &node, newOrderModelNodeList) {
        FormEditorItem *item = m_scene->itemForQmlItemNode(QmlItemNode(node));
        if (item) {
            FormEditorItem *oldParentItem = item->parentItem();
            item->setParentItem(0);
            item->setParentItem(oldParentItem);
        }
    }

    m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
}

void FormEditorView::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
    QmlModelView::auxiliaryDataChanged(node, name, data);
    if (name == "invisible") {
        FormEditorItem *item(m_scene->itemForQmlItemNode(QmlItemNode(node)));
        bool isInvisible = data.toBool();
        item->setVisible(!isInvisible);
        ModelNode newNode(node);
        if (isInvisible)
            newNode.deselectNode();
    }
}

double FormEditorView::margins() const
{
    return m_formEditorWidget->margins();
}

double FormEditorView::spacing() const
{
    return m_formEditorWidget->spacing();
}

void FormEditorView::activateItemCreator(const QString &name)
{
    if (m_currentTool == m_itemCreatorTool) {
        m_itemCreatorTool->setItemString(name);
        return;
    }
    changeToItemCreatorTool();
    m_itemCreatorTool->setItemString(name);
}

void FormEditorView::deActivateItemCreator()
{
    if (m_currentTool == m_itemCreatorTool) {
        changeToSelectionTool();
        emit ItemCreatorDeActivated();
    }
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

void FormEditorView::transformChanged(const QmlObjectNode &qmlObjectNode)
{
    QmlItemNode itemNode = qmlObjectNode.toQmlItemNode();
    if (itemNode.isValid() && scene()->hasItemForQmlItemNode(itemNode)) {
        m_scene->synchronizeTransformation(itemNode);
        m_currentTool->formEditorItemsChanged(QList<FormEditorItem*>() << m_scene->itemForQmlItemNode(itemNode));
    }
}

void FormEditorView::parentChanged(const QmlObjectNode &qmlObjectNode)
{
    QmlItemNode itemNode = qmlObjectNode.toQmlItemNode();
    QmlItemNode parentNode = qmlObjectNode.instanceParent().toQmlItemNode();
    if (itemNode.isValid()
        && scene()->hasItemForQmlItemNode(itemNode)
        && parentNode.isValid()
        && scene()->hasItemForQmlItemNode(parentNode)
        && scene()->itemForQmlItemNode(itemNode)->parentItem() != scene()->itemForQmlItemNode(parentNode)) {
        scene()->synchronizeParent(itemNode);
        m_currentTool->formEditorItemsChanged(QList<FormEditorItem*>() << m_scene->itemForQmlItemNode(itemNode));
    }
}

void FormEditorView::otherPropertyChanged(const QmlObjectNode &qmlObjectNode)
{
    Q_ASSERT(qmlObjectNode.isValid());

    QmlItemNode itemNode = findRecursiveQmlItemNode(qmlObjectNode);

    if (itemNode.isValid() && scene()->hasItemForQmlItemNode(itemNode)) {
        m_scene->synchronizeOtherProperty(itemNode);
        m_currentTool->formEditorItemsChanged(QList<FormEditorItem*>() << m_scene->itemForQmlItemNode(itemNode));
    }
}

void FormEditorView::updateGraphicsIndicators()
{
    m_currentTool->formEditorItemsChanged(scene()->allFormEditorItems());
}

void FormEditorView::updateItem(const QmlObjectNode &qmlObjectNode)
{

    Q_ASSERT(qmlObjectNode.isValid());

    QmlItemNode itemNode = findRecursiveQmlItemNode(qmlObjectNode);

    if (itemNode.isValid() && scene()->hasItemForQmlItemNode(itemNode)) {
        m_scene->synchronizeOtherProperty(itemNode);
        m_currentTool->formEditorItemsChanged(QList<FormEditorItem*>() << m_scene->itemForQmlItemNode(itemNode));
    }
}

void FormEditorView::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    QmlModelView::stateChanged(newQmlModelState, oldQmlModelState);


    m_formEditorWidget->anchorToolAction()->setEnabled(newQmlModelState.isBaseState());

    if (!newQmlModelState.isBaseState() && currentTool() == m_anchorTool) {
        changeToTransformTools();
        m_formEditorWidget->transformToolAction()->setChecked(true);
    }

//    FormEditorItem *item = m_scene->itemForQmlItemNode(fxObjectNode);
//
//    m_currentTool->formEditorItemsChanged(itemList);
}


}

