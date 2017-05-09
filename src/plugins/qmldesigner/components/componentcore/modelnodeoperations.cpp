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

#include "modelnodeoperations.h"
#include "modelnodecontextmenu_helper.h"
#include "layoutingridlayout.h"
#include "findimplementation.h"

#include "addsignalhandlerdialog.h"

#include <bindingproperty.h>
#include <nodelistproperty.h>
#include <nodehints.h>
#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <variantproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>
#include <documentmanager.h>
#include <qmlanchors.h>
#include <nodelistproperty.h>
#include <signalhandlerproperty.h>

#include <limits>
#include <qmldesignerplugin.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/editormanager/editormanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/icore.h>

#include <qmljseditor/qmljsfindreferences.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QByteArray>

#include <algorithm>
#include <functional>
#include <cmath>

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
        selectionState.view()->setSelectedModelNodes({selectionState.targetNode()});
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

enum OderAction {RaiseItem, LowerItem};

void changeOrder(const SelectionContext &selectionState, OderAction orderAction)
{
    if (!selectionState.view())
        return;

    QTC_ASSERT(selectionState.singleNodeIsSelected(), return);
    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    if (modelNode.isRootNode())
        return;
    if (!modelNode.parentProperty().isNodeListProperty())
        return;

    try {
        RewriterTransaction transaction(selectionState.view(), QByteArrayLiteral("DesignerActionManager|raise"));

        ModelNode modelNode = selectionState.currentSingleSelectedNode();
        NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
        const int index = parentProperty.indexOf(modelNode);

        if (orderAction == RaiseItem) {

            if (index < parentProperty.count() - 1)
                parentProperty.slide(index, index + 1);
        } else if (orderAction == LowerItem) {
            if (index > 0)
                parentProperty.slide(index, index - 1);
        }

        transaction.commit();
    } catch (const RewritingException &e) { //better save then sorry
         e.showException();
    }
}

void raise(const SelectionContext &selectionState)
{
    changeOrder(selectionState, RaiseItem);
}

void lower(const SelectionContext &selectionState)
{

    changeOrder(selectionState, LowerItem);
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
            QmlItemNode itemNode(node);
            itemNode.removeProperty("width");
            itemNode.removeProperty("height");
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
            QmlItemNode itemNode(node);
            itemNode.removeProperty("x");
            itemNode.removeProperty("y");
        }
        transaction.commit();
    } catch (const RewritingException &e) { //better save then sorry
        e.showException();
    }
}

void goIntoComponentOperation(const SelectionContext &selectionState)
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
        QmlItemNode itemNode(node);
        itemNode.removeProperty("z");
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
        const QmlItemNode qmlItemNode = QmlItemNode(selectionContext.firstSelectedModelNode());

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

/*
bool optionsPageLessThan(const IOptionsPage *p1, const IOptionsPage *p2)
{
    if (p1->category() != p2->category())
        return p1->category().alphabeticallyBefore(p2->category());
    return p1->id().alphabeticallyBefore(p2->id());
}

static inline QList<IOptionsPage*> sortedOptionsPages()
{
    QList<IOptionsPage*> rc = ExtensionSystem::PluginManager::getObjects<IOptionsPage>();
    qStableSort(rc.begin(), rc.end(), optionsPageLessThan);
    return rc;
}

*/
static PropertyNameList sortedPropertyNameList(const PropertyNameList &nameList)
{
    PropertyNameList sortedPropertyNameList = nameList;
    std::stable_sort(sortedPropertyNameList.begin(), sortedPropertyNameList.end());
    return sortedPropertyNameList;
}

static QString toUpper(const QString signal)
{
    QString ret = signal;
    ret[0] = signal.at(0).toUpper();
    return ret;
}

static void addSignal(const QString &typeName, const QString &itemId, const QString &signalName, bool isRootModelNode)
{
    QScopedPointer<Model> model(Model::create("Item", 2, 0));
    RewriterView rewriterView(RewriterView::Amend, 0);

    TextEditor::TextEditorWidget *textEdit = qobject_cast<TextEditor::TextEditorWidget*>
            (Core::EditorManager::currentEditor()->widget());

    BaseTextEditModifier modifier(textEdit);

    rewriterView.setCheckSemanticErrors(false);
    rewriterView.setTextModifier(&modifier);

    model->setRewriterView(&rewriterView);

    PropertyName signalHandlerName;

    if (isRootModelNode)
        signalHandlerName = "on" + toUpper(signalName).toUtf8();
    else
        signalHandlerName = itemId.toUtf8() + ".on" + toUpper(signalName).toUtf8();

    foreach (const ModelNode &modelNode, rewriterView.allModelNodes()) {
        if (modelNode.type() == typeName.toUtf8()) {
            modelNode.signalHandlerProperty(signalHandlerName).setSource(QLatin1String("{\n}"));
        }
    }
}

static QStringList cleanSignalNames(const QStringList &input)
{
    QStringList output;

    foreach (const QString &signal, input)
        if (!signal.startsWith(QLatin1String("__")) && !output.contains(signal))
            output.append(signal);

    return output;
}

static QStringList getSortedSignalNameList(const ModelNode &modelNode)
{
    NodeMetaInfo metaInfo = modelNode.metaInfo();
    QStringList signalNames;

    if (metaInfo.isValid()) {
        foreach (const PropertyName &signalName, sortedPropertyNameList(metaInfo.signalNames()))
            if (!signalName.contains("Changed"))
                signalNames.append(QString::fromUtf8(signalName));

        foreach (const PropertyName &propertyName, sortedPropertyNameList(metaInfo.propertyNames()))
            if (!propertyName.contains("."))
                signalNames.append(QString::fromUtf8(propertyName + "Changed"));
    }

    return signalNames;
}

void addSignalHandlerOrGotoImplementation(const SelectionContext &selectionState, bool addAlwaysNewSlot)
{
    ModelNode modelNode;
    if (selectionState.singleNodeIsSelected())
        modelNode = selectionState.selectedModelNodes().first();

    bool isModelNodeRoot = true;

    QmlObjectNode qmlObjectNode(modelNode);

    if (!qmlObjectNode.isValid()) {
        QString title = QCoreApplication::translate("ModelNodeOperations", "Go to Implementation");
        QString description = QCoreApplication::translate("ModelNodeOperations", "Invalid item.");
        Core::AsynchronousMessageBox::warning(title, description);
        return;
    }

    if (!qmlObjectNode.isRootModelNode()) {
        isModelNodeRoot = false;
        try {
            RewriterTransaction transaction =
                    qmlObjectNode.view()->beginRewriterTransaction(QByteArrayLiteral("NavigatorTreeModel:exportItem"));

            QmlObjectNode qmlObjectNode(modelNode);
            qmlObjectNode.ensureAliasExport();
            transaction.commit();
        }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
            exception.showException();
        }
    }

    QString itemId = modelNode.id();

    const Utils::FileName currentDesignDocument = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->fileName();
    const QString fileName = currentDesignDocument.toString();
    const QString typeName = currentDesignDocument.toFileInfo().baseName();

    QStringList signalNames = cleanSignalNames(getSortedSignalNameList(selectionState.selectedModelNodes().first()));

    QList<QmlJSEditor::FindReferences::Usage> usages = QmlJSEditor::FindReferences::findUsageOfType(fileName, typeName);

    if (usages.isEmpty()) {
        QString title = QCoreApplication::translate("ModelNodeOperations", "Go to Implementation");
        QString description = QCoreApplication::translate("ModelNodeOperations", "Cannot find an implementation.");
        Core::AsynchronousMessageBox::warning(title, description);
        return;
    }

    usages = FindImplementation::run(usages.first().path, typeName, itemId);

    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);

    if (usages.count() > 0 && (addAlwaysNewSlot || usages.count() < 2)  && (!isModelNodeRoot  || addAlwaysNewSlot)) {
        Core::EditorManager::openEditorAt(usages.first().path, usages.first().line, usages.first().col);

        if (!signalNames.isEmpty()) {
            AddSignalHandlerDialog *dialog = new AddSignalHandlerDialog(Core::ICore::dialogParent());
            dialog->setSignals(signalNames);

            AddSignalHandlerDialog::connect(dialog, &AddSignalHandlerDialog::signalSelected, [=] {
                dialog->deleteLater();

                if (dialog->signal().isEmpty())
                    return;

                try {
                    RewriterTransaction transaction =
                            qmlObjectNode.view()->beginRewriterTransaction(QByteArrayLiteral("NavigatorTreeModel:exportItem"));

                    addSignal(typeName, itemId, dialog->signal(), isModelNodeRoot);
                }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
                    exception.showException();
                }

                addSignal(typeName, itemId, dialog->signal(), isModelNodeRoot);

                //Move cursor to correct curser position
                const QString filePath = Core::EditorManager::currentDocument()->filePath().toString();
                QList<QmlJSEditor::FindReferences::Usage> usages = FindImplementation::run(filePath, typeName, itemId);
                Core::EditorManager::openEditorAt(filePath, usages.first().line, usages.first().col + 1);
            } );
            dialog->show();

        }
        return;
    }

    Core::EditorManager::openEditorAt(usages.first().path, usages.first().line, usages.first().col + 1);
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

void moveToComponent(const SelectionContext &selectionContext)
{
    ModelNode modelNode;
    if (selectionContext.singleNodeIsSelected())
        modelNode = selectionContext.selectedModelNodes().first();

    if (modelNode.isValid())
        selectionContext.view()->model()->rewriterView()->moveToComponent(modelNode);
}

void goImplementation(const SelectionContext &selectionState)
{
    addSignalHandlerOrGotoImplementation(selectionState, false);
}

void addNewSignalHandler(const SelectionContext &selectionState)
{
    addSignalHandlerOrGotoImplementation(selectionState, true);
}

void addItemToStackedContainer(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    const PropertyName propertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(propertyName), return);
    BindingProperty binding = container.bindingProperty(propertyName);

    /* Check if there is already a TabBar attached. */
    ModelNode potentialTabBar;
    if (binding.isValid()) {
        AbstractProperty bindingTarget = binding.resolveToProperty();
        if (bindingTarget.isValid()) { // In this case the stacked container might be hooked up to a TabBar
            potentialTabBar = bindingTarget.parentModelNode();

            if (!(potentialTabBar.metaInfo().isValid()
                  && potentialTabBar.metaInfo().isSubclassOf("QtQuick.Controls.TabBar")))
                potentialTabBar = ModelNode();
        }
    }

    try {
        RewriterTransaction transaction =
                view->beginRewriterTransaction(QByteArrayLiteral("DesignerActionManager:addItemToStackedContainer"));

        NodeMetaInfo itemMetaInfo = view->model()->metaInfo("QtQuick.Item", -1, -1);
        QTC_ASSERT(itemMetaInfo.isValid(), return);
        QTC_ASSERT(itemMetaInfo.majorVersion() == 2, return);

        QmlDesigner::ModelNode itemNode =
                view->createModelNode("QtQuick.Item", itemMetaInfo.majorVersion(), itemMetaInfo.minorVersion());

        container.defaultNodeListProperty().reparentHere(itemNode);

        if (potentialTabBar.isValid()) {// The stacked container is hooked up to a TabBar
            NodeMetaInfo tabButtonMetaInfo = view->model()->metaInfo("QtQuick.Controls.TabButton", -1, -1);
            if (tabButtonMetaInfo.isValid()) {
                const int buttonIndex = potentialTabBar.directSubModelNodes().count();
                ModelNode tabButtonNode =
                        view->createModelNode("QtQuick.Controls.TabButton",
                                              tabButtonMetaInfo.majorVersion(),
                                              tabButtonMetaInfo.minorVersion());

                tabButtonNode.variantProperty("text").setValue(QString::fromLatin1("Tab %1").arg(buttonIndex));
                potentialTabBar.defaultNodeListProperty().reparentHere(tabButtonNode);

            }
        }

        transaction.commit();
    }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
        exception.showException();
    }
}

PropertyName getIndexPropertyName(const ModelNode &modelNode)
{
    const PropertyName propertyName = NodeHints::fromModelNode(modelNode).indexPropertyForStackedContainer().toUtf8();

    if (modelNode.metaInfo().hasProperty(propertyName))
        return propertyName;

    if (modelNode.metaInfo().hasProperty("currentIndex"))
        return "currentIndex";

    if (modelNode.metaInfo().hasProperty("index"))
        return "index";

    return PropertyName();
}

void static setIndexProperty(const AbstractProperty &property, const QVariant &value)
{
    if (!property.exists() || property.isVariantProperty()) {
        /* Using QmlObjectNode ensures we take states into account. */
        property.parentQmlObjectNode().setVariantProperty(property.name(), value);
        return;
    } else if (property.isBindingProperty()) {
        /* Track one binding to the original source, incase a TabBar is attached */
        const AbstractProperty orignalProperty = property.toBindingProperty().resolveToProperty();
        if (orignalProperty.isValid() && (orignalProperty.isVariantProperty() || !orignalProperty.exists())) {
            orignalProperty.parentQmlObjectNode().setVariantProperty(orignalProperty.name(), value);
            return;
        }
    }

    const QString propertyName = QString::fromUtf8(property.name());

    QString title = QCoreApplication::translate("ModelNodeOperations", "Cannot Set Property %1").arg(propertyName);
    QString description = QCoreApplication::translate("ModelNodeOperations", "The property %1 is bound to an expression.").arg(propertyName);
    Core::AsynchronousMessageBox::warning(title, description);
}

void increaseIndexOfStackedContainer(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    const PropertyName propertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(propertyName), return);

    QmlItemNode containerItemNode(container);
    QTC_ASSERT(containerItemNode.isValid(), return);

    int value = containerItemNode.instanceValue(propertyName).toInt();
    ++value;

    const int maxValue = container.directSubModelNodes().count();

    QTC_ASSERT(value < maxValue, return);

    setIndexProperty(container.property(propertyName), value);
}

void decreaseIndexOfStackedContainer(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    const PropertyName propertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(propertyName), return);

    QmlItemNode containerItemNode(container);
    QTC_ASSERT(containerItemNode.isValid(), return);

    int value = containerItemNode.instanceValue(propertyName).toInt();
    --value;

    QTC_ASSERT(value > -1, return);

    setIndexProperty(container.property(propertyName), value);
}

void addTabBarToStackedContainer(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    NodeMetaInfo tabBarMetaInfo = view->model()->metaInfo("QtQuick.Controls.TabBar", -1, -1);
    QTC_ASSERT(tabBarMetaInfo.isValid(), return);
    QTC_ASSERT(tabBarMetaInfo.majorVersion() == 2, return);

    NodeMetaInfo tabButtonMetaInfo = view->model()->metaInfo("QtQuick.Controls.TabButton", -1, -1);
    QTC_ASSERT(tabButtonMetaInfo.isValid(), return);
    QTC_ASSERT(tabButtonMetaInfo.majorVersion() == 2, return);

    QmlItemNode containerItemNode(container);
    QTC_ASSERT(containerItemNode.isValid(), return);

    const PropertyName indexPropertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(indexPropertyName), return);

    try {
        RewriterTransaction transaction =
                view->beginRewriterTransaction(QByteArrayLiteral("DesignerActionManager:addItemToStackedContainer"));

        ModelNode tabBarNode =
                view->createModelNode("QtQuick.Controls.TabBar",
                                      tabBarMetaInfo.majorVersion(),
                                      tabBarMetaInfo.minorVersion());

        container.parentProperty().reparentHere(tabBarNode);

        const int maxValue = container.directSubModelNodes().count();

        QmlItemNode tabBarItem(tabBarNode);

        tabBarItem.anchors().setAnchor(AnchorLineLeft, containerItemNode, AnchorLineLeft);
        tabBarItem.anchors().setAnchor(AnchorLineRight, containerItemNode, AnchorLineRight);
        tabBarItem.anchors().setAnchor(AnchorLineBottom, containerItemNode, AnchorLineTop);

        for (int i = 0; i < maxValue; ++i) {
            ModelNode tabButtonNode =
                    view->createModelNode("QtQuick.Controls.TabButton",
                                          tabButtonMetaInfo.majorVersion(),
                                          tabButtonMetaInfo.minorVersion());

            tabButtonNode.variantProperty("text").setValue(QString::fromLatin1("Tab %1").arg(i));
            tabBarNode.defaultNodeListProperty().reparentHere(tabButtonNode);
        }

        const QString id = tabBarNode.validId();

        container.removeProperty(indexPropertyName);
        const QString expression = id + "." + QString::fromLatin1(indexPropertyName);
        container.bindingProperty(indexPropertyName).setExpression(expression);

        transaction.commit();
    }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
        exception.showException();
    }
}

} // namespace Mode

} //QmlDesigner
