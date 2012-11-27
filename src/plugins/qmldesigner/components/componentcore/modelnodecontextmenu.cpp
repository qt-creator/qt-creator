/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <cmath>
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
#include <bindingproperty.h>
#include <nodeproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>
#include <designmodewidget.h>
#include <qmlanchors.h>

const QString auxDataString = QLatin1String("anchors_");

namespace QmlDesigner {

static inline DesignDocumentController* designDocumentController()
{
    return Internal::DesignModeWidget::instance()->currentDesignDocumentController();
}

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
    return node.isValid() && node.metaInfo().isValid() && node.metaInfo().isSubclassOf("QtQuick.Item", -1, -1);
}

static inline bool itemsHaveSameParent(const QList<ModelNode> &siblingList)
{
    if (siblingList.isEmpty())
        return false;


    QmlItemNode item(siblingList.first());
    if (!item.isValid())
        return false;

    if (item.isRootModelNode())
        return false;

    QmlItemNode parent = item.instanceParent().toQmlItemNode();
    if (!parent.isValid())
        return false;

    foreach (const ModelNode &node, siblingList) {
        QmlItemNode currentItem(node);
        if (!currentItem.isValid())
            return false;
        QmlItemNode currentParent = currentItem.instanceParent().toQmlItemNode();
        if (!currentParent.isValid())
            return false;
        if (currentItem.instanceIsInPositioner())
            return false;
        if (currentParent != parent)
            return false;
    }
    return true;
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

static inline void getWidthHeight(const ModelNode &node, int &width, int &height)
{
    QmlItemNode itemNode(node);
    if (itemNode.isValid()) {
        width = itemNode.instanceValue("width").toInt();
        height = itemNode.instanceValue("height").toInt();
    }
}

static inline void getProperties(const ModelNode node, QHash<QString, QVariant> &propertyHash)
{
    if (QmlObjectNode(node).isValid()) {
        foreach (const QString &propertyName, node.propertyNames()) {
            if (node.property(propertyName).isVariantProperty() ||
                    (node.property(propertyName).isBindingProperty() &&
                     !propertyName.contains(QLatin1String("anchors.")))) {
                propertyHash.insert(propertyName, QmlObjectNode(node).instanceValue(propertyName));
            }
        }
    }
    QmlItemNode itemNode(node);
    if (itemNode.isValid()) {
        propertyHash.insert(QLatin1String("width"), itemNode.instanceValue(QLatin1String("width")));
        propertyHash.insert(QLatin1String("height"), itemNode.instanceValue(QLatin1String("height")));
        propertyHash.remove(QLatin1String("x"));
        propertyHash.remove(QLatin1String("y"));
        propertyHash.remove(QLatin1String("rotation"));
        propertyHash.remove(QLatin1String("opacity"));
    }
}

static inline void applyProperties(ModelNode &node, const QHash<QString, QVariant> &propertyHash)
{
    QHash<QString, QVariant> auxiliaryData  = node.auxiliaryData();
    foreach (const QString propertyName, auxiliaryData.keys()) {
        node.setAuxiliaryData(propertyName, QVariant());
    }

    QHashIterator<QString, QVariant> i(propertyHash);
    while (i.hasNext()) {
        i.next();
        if (i.key() == QLatin1String("width") || i.key() == QLatin1String("height")) {
            node.setAuxiliaryData(i.key(), i.value());
        } else if (node.property(i.key()).isDynamic() &&
                   node.property(i.key()).dynamicTypeName() == QLatin1String("alias") &&
                   node.property(i.key()).isBindingProperty()) {
            AbstractProperty targetProperty = node.bindingProperty(i.key()).resolveToProperty();
            if (targetProperty.isValid()) {
                targetProperty.parentModelNode().setAuxiliaryData(targetProperty.name() + QLatin1String("@NodeInstance"), i.value());
            }
        } else {
            node.setAuxiliaryData(i.key() + QLatin1String("@NodeInstance"), i.value());
        }
    }
}

static inline void openFileForComponent(const ModelNode &node)
{
    //int width = 0;
    //int height = 0;
    QHash<QString, QVariant> propertyHash;
    if (node.metaInfo().isComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        designDocumentController()->changeToExternalSubComponent(node.metaInfo().componentFileName());
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate") &&
               node.nodeProperty("delegate").modelNode().metaInfo().isComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        designDocumentController()->changeToExternalSubComponent(node.nodeProperty("delegate").modelNode().metaInfo().componentFileName());
    }
    ModelNode rootModelNode = designDocumentController()->model()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

static inline void openInlineComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return;

    if (!designDocumentController())
        return;

    //int width = 0;
    //int height = 0;
    QHash<QString, QVariant> propertyHash;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        designDocumentController()->changeToSubComponent(node);
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource) {
            //getWidthHeight(node, width, height);
            getProperties(node, propertyHash);
            designDocumentController()->changeToSubComponent(node.nodeProperty("delegate").modelNode());
        }
    }

    ModelNode rootModelNode = designDocumentController()->model()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
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
    const bool isInBaseState = m_view->currentState().isBaseState();
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
            action->setEnabled(false);
        editMenu->addAction(action);
        action = createModelNodeAction(tr("Reset Size"), editMenu, selectedModelNodes, ModelNodeAction::ResetSize);
        if (!modelNodesHaveProperty(selectedModelNodes, QLatin1String("width")) && !modelNodesHaveProperty(selectedModelNodes, QLatin1String("height")))
            action->setEnabled(false);
        editMenu->addAction(action);
        action = createModelNodeAction(tr("Visibility"), editMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::ModelNodeVisibility, singleSelected);
        editMenu->addAction(action);
        if (singleSelected && !isItem(currentSingleNode))
            action->setEnabled(false);

    } else {
        editMenu->setEnabled(false);
    }

    QMenu *anchorMenu = new QMenu(tr("Anchors"), menu);
    menu->addMenu(anchorMenu);


    if (singleSelected && isInBaseState) {
        QmlItemNode itemNode(currentSingleNode);

        bool anchored = itemNode.instanceHasAnchors();
        bool isRootNode = itemNode.isRootNode();

        ModelNodeAction *action = createModelNodeAction(tr("Fill"), anchorMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::AnchorFill, !anchored && !isRootNode);
        anchorMenu->addAction(action);
        action = createModelNodeAction(tr("Reset"), anchorMenu, QList<ModelNode>() << currentSingleNode, ModelNodeAction::AnchorReset, anchored && !isRootNode);
        anchorMenu->addAction(action);
    } else {
        anchorMenu->setEnabled(false);
    }

    QMenu *layoutMenu = new QMenu(tr("Layout"), menu);
    menu->addMenu(layoutMenu);

    bool layoutingIsPossible = itemsHaveSameParent(selectedModelNodes) && isInBaseState;

    if (!singleSelected && !selectionIsEmpty && layoutingIsPossible) {

        ModelNodeAction *action = createModelNodeAction(tr("Layout in Row"), layoutMenu, selectedModelNodes, ModelNodeAction::LayoutRow, true);
        layoutMenu->addAction(action);
        action = createModelNodeAction(tr("Layout in Column"), layoutMenu, selectedModelNodes, ModelNodeAction::LayoutColumn, true);
        layoutMenu->addAction(action);
        action = createModelNodeAction(tr("Layout in Grid"), layoutMenu, selectedModelNodes, ModelNodeAction::LayoutGrid, true);
        layoutMenu->addAction(action);
        action = createModelNodeAction(tr("Layout in Flow"), layoutMenu, selectedModelNodes, ModelNodeAction::LayoutFlow, true);
        layoutMenu->addAction(action);

    } else {
        layoutMenu->setEnabled(false);
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

void ModelNodeContextMenu::showContextMenu(QmlModelView *view, const QPoint &globalPosition, const QPoint &scenePosition, bool showSelection)
{
    ModelNodeContextMenu contextMenu(view);
    contextMenu.setScenePos(scenePosition);
    contextMenu.execute(globalPosition, showSelection);
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
    try {
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
        case ModelNodeAction::AnchorFill: anchorsFill(); break;
        case ModelNodeAction::AnchorReset: anchorsReset(); break;
        case ModelNodeAction::LayoutColumn: layoutColumn(); break;
        case ModelNodeAction::LayoutRow: layoutRow(); break;
        case ModelNodeAction::LayoutGrid: layoutGrid(); break;
        case ModelNodeAction::LayoutFlow: layoutFlow(); break;
        }
    } catch (RewritingException e) { //better save then sorry
        QMessageBox::warning(0, "Error", e.description());
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
        RewriterTransaction transaction(m_view);
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
        RewriterTransaction transaction(m_view);
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
        RewriterTransaction transaction(m_view);
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
        RewriterTransaction transaction(m_view);
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

    RewriterTransaction transaction(m_view);
    foreach (ModelNode node, m_modelNodeList) {
        node.removeProperty("z");
    }
}

static inline void backupPropertyAndRemove(ModelNode node, const QString &propertyName)
{
    if (node.hasVariantProperty(propertyName)) {
        node.setAuxiliaryData(auxDataString + propertyName, node.variantProperty(propertyName).value());
        node.removeProperty(propertyName);

    }
    if (node.hasBindingProperty(propertyName)) {
        node.setAuxiliaryData(auxDataString + propertyName, QmlItemNode(node).instanceValue(propertyName));
        node.removeProperty(propertyName);
    }
}


static inline void restoreProperty(ModelNode node, const QString &propertyName)
{
    if (node.hasAuxiliaryData(auxDataString + propertyName))
        node.variantProperty(propertyName) = node.auxiliaryData(auxDataString + propertyName);
}

void ModelNodeAction::anchorsFill()
{
    if (!m_view)
        return;

    RewriterTransaction transaction(m_view);

    foreach (ModelNode modelNode, m_modelNodeList) {
        QmlItemNode node = modelNode;
        if (node.isValid()) {
            node.anchors().fill();
            backupPropertyAndRemove(modelNode, QLatin1String("x"));
            backupPropertyAndRemove(modelNode, QLatin1String("y"));
            backupPropertyAndRemove(modelNode, QLatin1String("width"));
            backupPropertyAndRemove(modelNode, QLatin1String("height"));
        }
    }
}

void ModelNodeAction::anchorsReset()
{
    if (!m_view)
        return;
    RewriterTransaction transaction(m_view);

    foreach (ModelNode modelNode, m_modelNodeList) {
        QmlItemNode node = modelNode;
        if (node.isValid()) {
            node.anchors().removeAnchors();
            node.anchors().removeMargins();
            restoreProperty(node, "x");
            restoreProperty(node, "y");
            restoreProperty(node, "width");
            restoreProperty(node, "height");
        }
    }
}

static inline void reparentTo(const ModelNode &node, const QmlItemNode &parent)
{

    if (parent.isValid() && node.isValid()) {
        NodeAbstractProperty parentProperty;

        if (parent.hasDefaultProperty()) {
            parentProperty = parent.nodeAbstractProperty(parent.defaultProperty());
        } else {
            parentProperty = parent.nodeAbstractProperty(QLatin1String("data"));
        }

        parentProperty.reparentHere(node);
    }
}


bool compareByX(const ModelNode &node1, const ModelNode &node2)
{
        QmlItemNode itemNode1 = QmlItemNode(node1);
        QmlItemNode itemNode2 = QmlItemNode(node2);
        if (itemNode1.isValid() && itemNode2.isValid())
            return itemNode1.instancePosition().x() < itemNode2.instancePosition().x();
        return false;
}

bool compareByY(const ModelNode &node1, const ModelNode &node2)
{
        QmlItemNode itemNode1 = QmlItemNode(node1);
        QmlItemNode itemNode2 = QmlItemNode(node2);
        if (itemNode1.isValid() && itemNode2.isValid())
            return itemNode1.instancePosition().y() < itemNode2.instancePosition().y();
        return false;
}

bool compareByGrid(const ModelNode &node1, const ModelNode &node2)
{
        QmlItemNode itemNode1 = QmlItemNode(node1);
        QmlItemNode itemNode2 = QmlItemNode(node2);
        if (itemNode1.isValid() && itemNode2.isValid()) {
            if ((itemNode1.instancePosition().y() + itemNode1.instanceSize().height())  < itemNode2.instancePosition().y())
                return true;
            if ((itemNode2.instancePosition().y() + itemNode2.instanceSize().height())  < itemNode1.instancePosition().y())
                return false; //first sort y (rows)
            return itemNode1.instancePosition().x() < itemNode2.instancePosition().x();
        }
        return false;
}

static inline QPoint getUpperLeftPosition(const QList<ModelNode> &modelNodeList)
{
    QPoint p(INT_MAX, INT_MAX);
    foreach (ModelNode modelNode, modelNodeList) {
        QmlItemNode itemNode = QmlItemNode(modelNode);
        if (itemNode.isValid()) {
            if (itemNode.instancePosition().x() < p.x())
                p.setX(itemNode.instancePosition().x());
            if (itemNode.instancePosition().y() < p.y())
                p.setY(itemNode.instancePosition().y());
        }

    }
    return p;
}

void ModelNodeAction::layoutRow()
{
    if (!m_view)
        return;

    NodeMetaInfo rowMetaInfo = m_view->model()->metaInfo(QLatin1String("QtQuick.Row"));

    if (!rowMetaInfo.isValid())
        return;

    ModelNode row;
    {
        RewriterTransaction transaction(m_view);

        QmlItemNode parent = QmlItemNode(m_modelNodeList.first()).instanceParent().toQmlItemNode();
        if (!parent.isValid())
            return;

        qDebug() << parent.modelNode().majorQtQuickVersion();

        row = m_view->createModelNode(QLatin1String("QtQuick.Row"), rowMetaInfo.majorVersion(), rowMetaInfo.minorVersion());

        reparentTo(row, parent);
    }

    {
        RewriterTransaction transaction(m_view);

        QPoint pos = getUpperLeftPosition(m_modelNodeList);
        row.variantProperty(QLatin1String("x")) = pos.x();
        row.variantProperty(QLatin1String("y")) = pos.y();

        QList<ModelNode> sortedList = m_modelNodeList;
        qSort(sortedList.begin(), sortedList.end(), compareByX);

        foreach (ModelNode modelNode, sortedList) {
            reparentTo(modelNode, row);
            modelNode.removeProperty(QLatin1String("x"));
            modelNode.removeProperty(QLatin1String("y"));
        }
    }
}

void ModelNodeAction::layoutColumn()
{
    if (!m_view)
        return;

    NodeMetaInfo columnMetaInfo = m_view->model()->metaInfo(QLatin1String("QtQuick.Column"));

    if (!columnMetaInfo.isValid())
        return;

    ModelNode column;
    {
        RewriterTransaction transaction(m_view);

        QmlItemNode parent = QmlItemNode(m_modelNodeList.first()).instanceParent().toQmlItemNode();
        if (!parent.isValid())
            return;

        column = m_view->createModelNode(QLatin1String("QtQuick.Column"), columnMetaInfo.majorVersion(), columnMetaInfo.minorVersion());

        reparentTo(column, parent);
    }

    {
        RewriterTransaction transaction(m_view);

        QPoint pos = getUpperLeftPosition(m_modelNodeList);
        column.variantProperty(QLatin1String("x")) = pos.x();
        column.variantProperty(QLatin1String("y")) = pos.y();

        QList<ModelNode> sortedList = m_modelNodeList;
        qSort(sortedList.begin(), sortedList.end(), compareByY);

        foreach (ModelNode modelNode, sortedList) {
            reparentTo(modelNode, column);
            modelNode.removeProperty(QLatin1String("x"));
            modelNode.removeProperty(QLatin1String("y"));
        }
    }
}

void ModelNodeAction::layoutGrid()
{
    if (!m_view)
        return;

    NodeMetaInfo gridMetaInfo = m_view->model()->metaInfo(QLatin1String("QtQuick.Grid"));

    if (!gridMetaInfo.isValid())
        return;

    ModelNode grid;
    {
        RewriterTransaction transaction(m_view);

        QmlItemNode parent = QmlItemNode(m_modelNodeList.first()).instanceParent().toQmlItemNode();
        if (!parent.isValid())
            return;

        grid = m_view->createModelNode(QLatin1String("QtQuick.Grid"), gridMetaInfo.majorVersion(), gridMetaInfo.minorVersion());
        grid.variantProperty(QLatin1String("columns")) = int(sqrt(double(m_modelNodeList.count())));

        reparentTo(grid, parent);
    }

    {
        RewriterTransaction transaction(m_view);

        QPoint pos = getUpperLeftPosition(m_modelNodeList);
        grid.variantProperty(QLatin1String("x")) = pos.x();
        grid.variantProperty(QLatin1String("y")) = pos.y();

        QList<ModelNode> sortedList = m_modelNodeList;
        qSort(sortedList.begin(), sortedList.end(), compareByGrid);

        foreach (ModelNode modelNode, sortedList) {
            reparentTo(modelNode, grid);
            modelNode.removeProperty(QLatin1String("x"));
            modelNode.removeProperty(QLatin1String("y"));
        }
    }
}

void ModelNodeAction::layoutFlow()
{
    if (!m_view)
        return;

    NodeMetaInfo flowMetaInfo = m_view->model()->metaInfo(QLatin1String("QtQuick.Flow"));

    if (!flowMetaInfo.isValid())
        return;

    ModelNode flow;
    {
        RewriterTransaction transaction(m_view);

        QmlItemNode parent = QmlItemNode(m_modelNodeList.first()).instanceParent().toQmlItemNode();
        if (!parent.isValid())
            return;

        flow = m_view->createModelNode(QLatin1String("QtQuick.Flow"), flowMetaInfo.majorVersion(), flowMetaInfo.minorVersion());

        reparentTo(flow, parent);
    }

    {
        RewriterTransaction transaction(m_view);

        QPoint pos = getUpperLeftPosition(m_modelNodeList);
        flow.variantProperty(QLatin1String("x")) = pos.x();
        flow.variantProperty(QLatin1String("y")) = pos.y();

        QList<ModelNode> sortedList = m_modelNodeList;
        qSort(sortedList.begin(), sortedList.end(), compareByGrid);

        foreach (ModelNode modelNode, sortedList) {
            reparentTo(modelNode, flow);
            modelNode.removeProperty(QLatin1String("x"));
            modelNode.removeProperty(QLatin1String("y"));
        }
    }
}

}
