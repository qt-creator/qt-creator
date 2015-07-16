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

#include "modelnodeoperations.h"
#include "modelnodecontextmenu_helper.h"
#include "layoutingridlayout.h"

#include <cmath>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <variantproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>
#include <documentmanager.h>
#include <qmlanchors.h>
#include <nodelistproperty.h>

#include <limits>
#include <qmldesignerplugin.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <qmljseditor/qmljsfindreferences.h>

#include <QCoreApplication>
#include <QByteArray>

#include <functional>

namespace QmlDesigner {

const PropertyName auxDataString("anchors_");

static inline QList<QmlItemNode> siblingsForNode(const QmlItemNode &itemNode)
{
    QList<QmlItemNode> siblingList;

    if (itemNode.isValid() && itemNode.modelNode().hasParentProperty()) {
        QList<ModelNode> modelNodes = itemNode.modelNode().parentProperty().parentModelNode().directSubModelNodes();
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

static inline void reparentTo(const ModelNode &node, const QmlItemNode &parent)
{

    if (parent.isValid() && node.isValid()) {
        NodeAbstractProperty parentProperty;

        if (parent.hasDefaultPropertyName())
            parentProperty = parent.defaultNodeAbstractProperty();
        else
            parentProperty = parent.nodeAbstractProperty("data");

        parentProperty.reparentHere(node);
    }
}

static inline QPointF getUpperLeftPosition(const QList<ModelNode> &modelNodeList)
{
    QPointF postion(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max());
    foreach (const ModelNode &modelNode, modelNodeList) {
        if (QmlItemNode::isValidQmlItemNode(modelNode)) {
            QmlItemNode qmlIitemNode = QmlItemNode(modelNode);
            if (qmlIitemNode.instancePosition().x() < postion.x())
                postion.setX(qmlIitemNode.instancePosition().x());
            if (qmlIitemNode.instancePosition().y() < postion.y())
                postion.setY(qmlIitemNode.instancePosition().y());
        }
    }

    return postion;
}

static void setUpperLeftPostionToNode(const ModelNode &layoutNode, const QList<ModelNode> &modelNodeList)
{
    QPointF upperLeftPosition = getUpperLeftPosition(modelNodeList);
    layoutNode.variantProperty("x").setValue(qRound(upperLeftPosition.x()));
    layoutNode.variantProperty("y") .setValue(qRound(upperLeftPosition.y()));
}

namespace ModelNodeOperations {

void goIntoComponent(const ModelNode &modelNode)
{
    DocumentManager::goIntoComponent(modelNode);
}

void select(const SelectionContext &selectionState)
{
    if (selectionState.view())
        selectionState.view()->setSelectedModelNodes(QList<ModelNode>() << selectionState.targetNode());
}

void deSelect(const SelectionContext &selectionState)
{
    if (selectionState.view()) {
        QList<ModelNode> selectedNodes = selectionState.view()->selectedModelNodes();
        foreach (const ModelNode &node, selectionState.selectedModelNodes()) {
            if (selectedNodes.contains(node))
                selectedNodes.removeAll(node);
        }
        selectionState.view()->setSelectedModelNodes(selectedNodes);
    }
}

void cut(const SelectionContext &)
{
}


void copy(const SelectionContext &)
{
}

void deleteSelection(const SelectionContext &)
{
}

void toFront(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    try {
        QmlItemNode node = selectionState.firstSelectedModelNode();
        if (node.isValid()) {
            signed int maximumZ = getMaxZValue(siblingsForNode(node));
            maximumZ++;
            node.setVariantProperty("z", maximumZ);
        }
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}


void toBack(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;
    try {
        QmlItemNode node = selectionState.firstSelectedModelNode();
        if (node.isValid()) {
            signed int minimumZ = getMinZValue(siblingsForNode(node));
            minimumZ--;
            node.setVariantProperty("z", minimumZ);
        }

    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void raise(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    try {
        RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|raise"));
        foreach (ModelNode modelNode, selectionState.selectedModelNodes()) {
            QmlItemNode node = modelNode;
            if (node.isValid()) {
                signed int z  = node.instanceValue("z").toInt();
                z++;
                node.setVariantProperty("z", z);
            }
        }
    } catch (const RewritingException &e) { //better save then sorry
         e.showException();
    }
}

void lower(const SelectionContext &selectionState)
{

    if (!selectionState.view())
        return;

    try {
        RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|lower"));
        foreach (ModelNode modelNode, selectionState.selectedModelNodes()) {
            QmlItemNode node = modelNode;
            if (node.isValid()) {
                signed int z  = node.instanceValue("z").toInt();
                z--;
                node.setVariantProperty("z", z);
            }
        }
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void paste(const SelectionContext &)
{
}

void undo(const SelectionContext &)
{
}

void redo(const SelectionContext &)
{
}

void setVisible(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    try {
        selectionState.selectedModelNodes().first().variantProperty("visible").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void setFillWidth(const SelectionContext &selectionState)
{
    if (!selectionState.view()
            || !selectionState.hasSingleSelectedModelNode())
        return;

    try {
        selectionState.firstSelectedModelNode().variantProperty("Layout.fillWidth").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void setFillHeight(const SelectionContext &selectionState)
{
    if (!selectionState.view()
            || !selectionState.hasSingleSelectedModelNode())
        return;

    try {
        selectionState.firstSelectedModelNode().variantProperty("Layout.fillHeight").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void resetSize(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    try {
        RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|resetSize"));
        foreach (ModelNode node, selectionState.selectedModelNodes()) {
            node.removeProperty("width");
            node.removeProperty("height");
        }
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void resetPosition(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    try {
        RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|resetPosition"));
        foreach (ModelNode node, selectionState.selectedModelNodes()) {
            node.removeProperty("x");
            node.removeProperty("y");
        }
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void goIntoComponent(const SelectionContext &selectionState)
{
    goIntoComponent(selectionState.currentSingleSelectedNode());
}

void setId(const SelectionContext &)
{
}

void resetZ(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|resetZ"));
    foreach (ModelNode node, selectionState.selectedModelNodes()) {
        node.removeProperty("z");
    }
}

static inline void backupPropertyAndRemove(ModelNode node, const PropertyName &propertyName)
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


static inline void restoreProperty(const ModelNode &node, const PropertyName &propertyName)
{
    if (node.hasAuxiliaryData(auxDataString + propertyName))
        node.variantProperty(propertyName).setValue(node.auxiliaryData(auxDataString + propertyName));
}

void anchorsFill(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    try {
        RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|anchorsFill"));

        ModelNode modelNode = selectionState.currentSingleSelectedNode();

        QmlItemNode node = modelNode;
        if (node.isValid()) {
            node.anchors().fill();
            backupPropertyAndRemove(modelNode, "x");
            backupPropertyAndRemove(modelNode, "y");
            backupPropertyAndRemove(modelNode, "width");
            backupPropertyAndRemove(modelNode, "height");
        }

        transaction.commit();
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void anchorsReset(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|anchorsReset"));

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

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

typedef std::function<bool(const ModelNode &node1, const ModelNode &node2)> LessThan;

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
        if ((itemNode2.instancePosition().y() + itemNode2.instanceSize().height())  < itemNode1.instancePosition().y() +  itemNode1.instanceSize().height())
            return false; //first sort y (rows)
        return itemNode1.instancePosition().x() < itemNode2.instancePosition().x();
    }
    return false;
}


static void layoutHelperFunction(const SelectionContext &selectionContext,
                                 TypeName layoutType,
                                 LessThan lessThan)
{
    if (!selectionContext.view()
            || !selectionContext.hasSingleSelectedModelNode()
             || !selectionContext.view()->model()->hasNodeMetaInfo(layoutType))
        return;

    if (QmlItemNode::isValidQmlItemNode(selectionContext.firstSelectedModelNode())) {
        QmlItemNode qmlItemNode = QmlItemNode(selectionContext.firstSelectedModelNode());

        if (qmlItemNode.hasInstanceParentItem()) {

            ModelNode layoutNode;
            {
                RewriterTransaction transaction(selectionContext.view(), QByteArrayLiteral("DesignerActionManager|layoutHelperFunction1"));

                QmlItemNode parentNode = qmlItemNode.instanceParentItem();

                NodeMetaInfo metaInfo = selectionContext.view()->model()->metaInfo(layoutType);

                layoutNode = selectionContext.view()->createModelNode(layoutType, metaInfo.majorVersion(), metaInfo.minorVersion());

                reparentTo(layoutNode, parentNode);
            }

            {
                RewriterTransaction transaction(selectionContext.view(), QByteArrayLiteral("DesignerActionManager|layoutHelperFunction2"));

                QList<ModelNode> sortedSelectedNodes =  selectionContext.selectedModelNodes();
                Utils::sort(sortedSelectedNodes, lessThan);

                setUpperLeftPostionToNode(layoutNode, sortedSelectedNodes);
                LayoutInGridLayout::reparentToNodeAndRemovePositionForModelNodes(layoutNode, sortedSelectedNodes);
                if (layoutType.contains("Layout"))
                    LayoutInGridLayout::setSizeAsPreferredSize(sortedSelectedNodes);
            }
        }
    }
}

void layoutRowPositioner(const SelectionContext &selectionContext)
{
    layoutHelperFunction(selectionContext, "QtQuick.Row", compareByX);
}

void layoutColumnPositioner(const SelectionContext &selectionContext)
{
    layoutHelperFunction(selectionContext, "QtQuick.Column", compareByY);
}

void layoutGridPositioner(const SelectionContext &selectionContext)
{
    layoutHelperFunction(selectionContext, "QtQuick.Grid", compareByGrid);
}

void layoutFlowPositioner(const SelectionContext &selectionContext)
{
    layoutHelperFunction(selectionContext, "QtQuick.Flow", compareByGrid);
}

void layoutRowLayout(const SelectionContext &selectionContext)
{
    try {
        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        layoutHelperFunction(selectionContext, "QtQuick.Layouts.RowLayout", compareByX);
    } catch (RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void layoutColumnLayout(const SelectionContext &selectionContext)
{
    try {
        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        layoutHelperFunction(selectionContext, "QtQuick.Layouts.ColumnLayout", compareByY);
    } catch (RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void layoutGridLayout(const SelectionContext &selectionContext)
{
    try {
        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        LayoutInGridLayout::layout(selectionContext);
    } catch (RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void gotoImplementation(const SelectionContext &/*selectionState*/)
{
    const QString fileName = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->fileName().toString();
    const QString typeName = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->fileName().toFileInfo().baseName();

    QList<QmlJSEditor::FindReferences::Usage> usages = QmlJSEditor::FindReferences::findUsageOfType(fileName, typeName);

    if (usages.isEmpty()) {
        QString title = QCoreApplication::translate("ModelNodeOperations", "Goto implementation");
        QString description = QCoreApplication::translate("ModelNodeOperations", "Cannot find any implementation.");
        Core::AsynchronousMessageBox::warning(title, description);
        return;
    }

    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    Core::EditorManager::openEditorAt(usages.first().path, usages.first().line, usages.first().col);

}

void removeLayout(const SelectionContext &selectionContext)
{
    if (!selectionContext.view()
            || !selectionContext.hasSingleSelectedModelNode())
        return;

    ModelNode layout = selectionContext.currentSingleSelectedNode();

    if (!QmlItemNode::isValidQmlItemNode(layout))
        return;

    QmlItemNode layoutItem(layout);

    QmlItemNode parent = layoutItem.instanceParentItem();

    if (!parent.isValid())
        return;

    {
        RewriterTransaction transaction(selectionContext.view(), QByteArrayLiteral("DesignerActionManager|removeLayout"));

        foreach (const ModelNode &modelNode, selectionContext.currentSingleSelectedNode().directSubModelNodes()) {
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {

                QmlItemNode qmlItem(modelNode);
                if (modelNode.simplifiedTypeName() == "Item"
                        && modelNode.id().contains("spacer")) {
                    qmlItem.destroy();
                } else {
                    QPointF pos = qmlItem.instancePosition();
                    pos = layoutItem.instanceTransform().map(pos);
                    modelNode.variantProperty("x").setValue(pos.x());
                    modelNode.variantProperty("y").setValue(pos.y());
                }
            }
            if (modelNode.isValid())
                parent.modelNode().defaultNodeListProperty().reparentHere(modelNode);
        }
        layoutItem.destroy();
    }
}

void removePositioner(const SelectionContext &selectionContext)
{
    removeLayout(selectionContext);
}

} // namespace Mode

} //QmlDesigner
