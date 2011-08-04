/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "modelnodecontextmenu.h"
#include <QApplication>
#include <QMessageBox>
#include <coreplugin/editormanager/editormanager.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <variantproperty.h>
#include <nodeproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>
#include <designdocumentcontroller.h>


namespace QmlDesigner {

static inline QString captionForModelNode(const ModelNode &modelNode)
{
    if (modelNode.id().isEmpty())
        return modelNode.simplifiedTypeName();

    return modelNode.id();
}

static inline bool contains(const QmlItemNode &node, const QPoint &p)
{
    return node.instanceSceneTransform().mapRect(node.instanceBoundingRect()).contains(p);
}

static inline bool checkIfNodeIsAView(const ModelNode &node)
{
    return node.metaInfo().isValid() &&
            (node.metaInfo().isSubclassOf("QtQuick.ListView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.GridView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.PathView", -1, -1));
}

static inline bool isItem(const ModelNode &node)
{
    return node.metaInfo().isValid() && node.metaInfo().isSubclassOf("QtQuick.Item", -1, -1);
}

static inline QList<QmlItemNode> siblingsForNode(const QmlItemNode &itemNode)
{
    QList<QmlItemNode> siblingList;

    if (itemNode.isValid() && itemNode.modelNode().parentProperty().isValid()) {
        QList<ModelNode> modelNodes = itemNode.modelNode().parentProperty().parentModelNode().allDirectSubModelNodes();
        foreach (const ModelNode &node, modelNodes) {
            QmlItemNode childItemNode = node;
            if (childItemNode.isValid())
                siblingList.append(childItemNode);
        }
    }

    return siblingList;
}

static signed int getMaxZValue(const QList<QmlItemNode> &siblingList)
{
    signed int maximum = INT_MIN;
    foreach (const QmlItemNode &node, siblingList) {
        signed int z  = node.instanceValue("z").toInt();
        if (z > maximum)
            maximum = z;
    }
    return maximum;
}

static signed int getMinZValue(const QList<QmlItemNode> &siblingList)
{
    signed int minimum = INT_MAX;
    foreach (const QmlItemNode &node, siblingList) {
        signed int z  = node.instanceValue("z").toInt();
        if (z < minimum)
            minimum = z;
    }
    return minimum;
}


static inline bool modelNodeIsComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isComponent())
        return true;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource)
        return true;
    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isComponent())
            return true;
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
            return true;
    }

    return false;
}

static inline bool isFileComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isComponent())
        return true;

    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isComponent())
            return true;
    }

    return false;
}

static inline void openFileForComponent(const ModelNode &node)
{
    if (node.metaInfo().isComponent()) {
        Core::EditorManager::instance()->openEditor(node.metaInfo().componentFileName());
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate") &&
               node.nodeProperty("delegate").modelNode().metaInfo().isComponent()) {
        Core::EditorManager::instance()->openEditor(node.nodeProperty("delegate").modelNode().metaInfo().componentFileName());
    }
}

static inline void openInlineComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return;

    if (!DesignDocumentController::instance())
        return;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource)
        DesignDocumentController::instance()->changeCurrentModelTo(node);
    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
            DesignDocumentController::instance()->changeCurrentModelTo(node.nodeProperty("delegate").modelNode());
    }
}

static inline bool modelNodesHaveProperty(const QList<ModelNode> &modelNodeList, const QString &propertyName)
{
    foreach (const ModelNode &modelNode, modelNodeList)
        if (modelNode.hasProperty(propertyName))
            return true;

    return false;
}

ModelNodeContextMenu::ModelNodeContextMenu(QmlModelView *view) : m_view(view)
{
}

void ModelNodeContextMenu::execute(const QPoint &pos, bool selectionMenuBool)
{
    QMenu* menu = new QMenu();

    bool singleSelected = false;
    bool selectionIsEmpty = m_view->selectedModelNodes().isEmpty();
    ModelNode currentSingleNode;
    const QList<ModelNode> &selectedModelNodes =  m_view->selectedModelNodes();
    if (selectedModelNodes.count()== 1) {
        singleSelected = true;
        currentSingleNode = selectedModelNodes.first();
    }

    if (selectionMenuBool) {
        QMenu *selectionMenu = new QMenu(tr("Selection"), menu);
        menu->addMenu(selectionMenu);
        ModelNode parentNode;
        if (singleSelected) {
            //ModelNodeAction *selectionAction;
            //selectionAction = createModelNodeAction(tr("DeSelect: ") + captionForModelNode(currentSingleNode), selectionMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::DeSelectModelNode);
            //selectionMenu->addAction(selectionAction);
            if (!currentSingleNode.isRootNode()) {
                parentNode = currentSingleNode.parentProperty().parentModelNode();
                selectionMenu->addAction(createModelNodeAction(tr("Select parent: %1").arg(captionForModelNode(parentNode)),
                                                               selectionMenu, QList<ModelNode>() << parentNode, ModelNodeAction::SelectModelNode));
            }

            selectionMenu->addSeparator();
        }
        foreach (const ModelNode &node, m_view->allModelNodes()) {
            if (node != currentSingleNode && node != parentNode && contains(node, m_scenePos) && !node.isRootNode())
                selectionMenu->addAction(createModelNodeAction(tr("Select: %1").arg(captionForModelNode(node)), selectionMenu, QList<ModelNode>() << node, ModelNodeAction::SelectModelNode));
        }
    }

    QMenu *stackMenu = new QMenu(tr("Stack (z)"), menu);
    menu->addMenu(stackMenu);

    stackMenu->addAction(createModelNodeAction(tr("To Front"), stackMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ToFront, singleSelected));
    stackMenu->addAction(createModelNodeAction(tr("To Back"), stackMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ToBack, singleSelected));
    stackMenu->addAction(createModelNodeAction(tr("Raise"), stackMenu, QList<ModelNode>() << selectedModelNodes, ModelNodeAction::Raise));
    stackMenu->addAction(createModelNodeAction(tr("Lower"), stackMenu, QList<ModelNode>() << selectedModelNodes, ModelNodeAction::Lower));
    stackMenu->addSeparator();
    stackMenu->addAction(createModelNodeAction(tr("Reset z property"), stackMenu, QList<ModelNode>() << selectedModelNodes, ModelNodeAction::ResetZ));

    QMenu *editMenu = new QMenu(tr("Edit"), menu);
    menu->addMenu(editMenu);
    if (!selectionIsEmpty) {
        //editMenu->addAction(createModelNodeAction(tr("Change Id"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::SetId, singleSelected));
        ModelNodeAction* action = createModelNodeAction(tr("Reset Position"), editMenu, selectedModelNodes, ModelNodeAction::ResetPosition);
        if (!modelNodesHaveProperty(selectedModelNodes, QLatin1String("x")) && !modelNodesHaveProperty(selectedModelNodes, QLatin1String("y")))
            action->setDisabled(true);
        editMenu->addAction(action);
        action = createModelNodeAction(tr("Reset Size"), editMenu, selectedModelNodes, ModelNodeAction::ResetSize);
        if (!modelNodesHaveProperty(selectedModelNodes, QLatin1String("width")) && !modelNodesHaveProperty(selectedModelNodes, QLatin1String("height")))
            action->setDisabled(true);
        editMenu->addAction(action);
        action = createModelNodeAction(tr("Visibility"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ModelNodeVisibility, singleSelected);
        editMenu->addAction(action);
        if (!isItem(currentSingleNode))
            action->setDisabled(true);

    } else {
        editMenu->setEnabled(false);
    }

    menu->addSeparator();
    bool enterComponent = false;
    if (singleSelected) {
        enterComponent = modelNodeIsComponent(currentSingleNode);
    }
    menu->addAction(createModelNodeAction(tr("Go into Component"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::GoIntoComponent, enterComponent));

    menu->exec(pos);
    menu->deleteLater();
}

void ModelNodeContextMenu::setScenePos(const QPoint &pos)
{
    m_scenePos = pos;
}


ModelNodeAction* ModelNodeContextMenu::createModelNodeAction(const QString &description, QMenu *menu, const QList<ModelNode> &modelNodeList, ModelNodeAction::ModelNodeActionType type, bool enabled)
{
    ModelNodeAction* action = new ModelNodeAction(description, menu, m_view, modelNodeList, type);
    action->setEnabled(enabled);
    return action;
}


ModelNodeAction::ModelNodeAction( const QString & text, QObject *parent, QmlModelView *view,  const QList<ModelNode> &modelNodeList, ModelNodeActionType type) :
    QAction(text, parent), m_view(view), m_modelNodeList(modelNodeList), m_type(type)
{
    if (type == ModelNodeVisibility) {
        setCheckable(true);
        QmlItemNode itemNode = QmlItemNode(m_modelNodeList.first());
        if (itemNode.isValid())
            setChecked(itemNode.instanceValue("visible").toBool());
        else
            setEnabled(false);
    }
    connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
}

void ModelNodeAction::goIntoComponent(const ModelNode &modelNode)
{

    if (modelNode.isValid() && modelNodeIsComponent(modelNode)) {
        if (isFileComponent(modelNode))
            openFileForComponent(modelNode);
        else
            openInlineComponent(modelNode);
    }
}

void ModelNodeAction::actionTriggered(bool b)
{
    switch (m_type) {
    case ModelNodeAction::SelectModelNode: select(); break;
    case ModelNodeAction::DeSelectModelNode: deSelect(); break;
    case ModelNodeAction::CutSelection: cut(); break;
    case ModelNodeAction::CopySelection: copy(); break;
    case ModelNodeAction::DeleteSelection: deleteSelection(); break;
    case ModelNodeAction::ToFront: toFront(); break;
    case ModelNodeAction::ToBack: toBack(); break;
    case ModelNodeAction::Raise: raise(); break;
    case ModelNodeAction::Lower: lower(); break;
    case ModelNodeAction::Paste: paste(); break;
    case ModelNodeAction::Undo: undo(); break;
    case ModelNodeAction::Redo: redo(); break;
    case ModelNodeAction::ModelNodeVisibility: setVisible(b); break;
    case ModelNodeAction::ResetSize: resetSize(); break;
    case ModelNodeAction::ResetPosition: resetPosition(); break;
    case ModelNodeAction::GoIntoComponent: goIntoComponent(); break;
    case ModelNodeAction::SetId: setId(); break;
    case ModelNodeAction::ResetZ: resetZ(); break;
    }
}

void ModelNodeAction::select()
{
    if (m_view)
        m_view->setSelectedModelNodes(m_modelNodeList);
}

void ModelNodeAction::deSelect()
{
    if (m_view) {
        QList<ModelNode> selectedNodes = m_view->selectedModelNodes();
        foreach (const ModelNode &node, m_modelNodeList) {
            if (selectedNodes.contains(node))
                selectedNodes.removeAll(node);
        }
        m_view->setSelectedModelNodes(selectedNodes);
    }
}

void ModelNodeAction::cut()
{
}


void ModelNodeAction::copy()
{
}

void ModelNodeAction::deleteSelection()
{
}

void ModelNodeAction::toFront()
{
    if (!m_view)
        return;

    try {
        QmlItemNode node = m_modelNodeList.first();
        if (node.isValid()) {
            signed int maximumZ = getMaxZValue(siblingsForNode(node));
            maximumZ++;
            node.setVariantProperty("z", maximumZ);
        }
    } catch (RewritingException &e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
    }
}


void ModelNodeAction::toBack()
{
    if (!m_view)
        return;
    try {
        QmlItemNode node = m_modelNodeList.first();
        if (node.isValid()) {
            signed int minimumZ = getMinZValue(siblingsForNode(node));
            minimumZ--;
            node.setVariantProperty("z", minimumZ);
        }

    } catch (RewritingException &e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
    }
}

void ModelNodeAction::raise()
{
    if (!m_view)
        return;

    try {
        RewriterTransaction(m_view);
        foreach (ModelNode modelNode, m_modelNodeList) {
            QmlItemNode node = modelNode;
            if (node.isValid()) {
                signed int z  = node.instanceValue("z").toInt();
                z++;
                node.setVariantProperty("z", z);
            }
        }
    } catch (RewritingException &e) { //better save then sorry
         QMessageBox::warning(0, "Error", e.description());
    }
}

void ModelNodeAction::lower()
{
    if (!m_view)
        return;
    try {
        RewriterTransaction(m_view);
        foreach (ModelNode modelNode, m_modelNodeList) {
            QmlItemNode node = modelNode;
            if (node.isValid()) {
                signed int z  = node.instanceValue("z").toInt();
                z--;
                node.setVariantProperty("z", z);
            }
        }
    } catch (RewritingException &e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
    }
}

void ModelNodeAction::paste()
{
}

void ModelNodeAction::undo()
{
}

void ModelNodeAction::redo()
{
}

void ModelNodeAction::setVisible(bool b)
{
    if (!m_view)
        return;
    try {
        m_modelNodeList.first().variantProperty("visible") = b;
    } catch (RewritingException &e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
    }
}


void ModelNodeAction::resetSize()
{
    if (!m_view)
        return;
    try {
        RewriterTransaction(m_view);
        foreach (ModelNode node, m_modelNodeList) {
            node.removeProperty("width");
            node.removeProperty("height");
        }
    } catch (RewritingException &e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
    }
}

void ModelNodeAction::resetPosition()
{
    if (!m_view)
        return;
    try {
        RewriterTransaction(m_view);
        foreach (ModelNode node, m_modelNodeList) {
            node.removeProperty("x");
            node.removeProperty("y");
        }
    } catch (RewritingException &e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
    }
}

void ModelNodeAction::goIntoComponent()
{
    goIntoComponent(m_modelNodeList.first());
}

void ModelNodeAction::setId()
{
}

void ModelNodeAction::resetZ()
{
    if (!m_view)
        return;

    RewriterTransaction(m_view);
    foreach (ModelNode node, m_modelNodeList) {
        node.removeProperty("z");
    }
}

}
