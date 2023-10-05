// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodeoperations.h"
#include "coreplugin/coreplugintr.h"
#include "designmodewidget.h"
#include "modelnodecontextmenu_helper.h"
#include "addimagesdialog.h"
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
#include <nodeproperty.h>
#include <signalhandlerproperty.h>
#include <nodeinstanceview.h>

#include <componentcore_constants.h>
#include <stylesheetmerger.h>

#include <designermcumanager.h>
#include <qmldesignerplugin.h>
#include <qmldesignerconstants.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/editormanager/editormanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/icore.h>

#include <qmljseditor/qmljsfindreferences.h>

#include <annotationeditor/annotationeditor.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include "projectexplorer/target.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/smallstring.h>

#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QByteArray>
#include <QFileDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QPointer>
#include <QMessageBox>
#include <QPair>

#include <algorithm>
#include <functional>
#include <cmath>
#include <limits>

#include <bindingeditor/signallist.h>

using namespace Utils;

namespace QmlDesigner {

namespace {
const Utils::SmallString auxDataString("anchors_");

Utils::SmallString auxPropertyString(Utils::SmallStringView name)
{
    return auxDataString + name;
}
} // namespace

inline static void reparentTo(const ModelNode &node, const QmlItemNode &parent)
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

inline static QPointF getUpperLeftPosition(const QList<ModelNode> &modelNodeList)
{
    QPointF postion(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max());
    for (const ModelNode &modelNode : modelNodeList) {
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

bool goIntoComponent(const ModelNode &modelNode)
{
    return DocumentManager::goIntoComponent(modelNode);
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
        const QList<ModelNode> nodes = selectionState.selectedModelNodes();
        for (const ModelNode &node : nodes) {
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
            ModelNode modelNode = selectionState.currentSingleSelectedNode();
            NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
            const int index = parentProperty.indexOf(modelNode);
            const int lastIndex = parentProperty.count() - 1;

            if (index != lastIndex)
                parentProperty.slide(index, lastIndex);
        }
    } catch (const RewritingException &e) { //better safe than sorry
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
            ModelNode modelNode = selectionState.currentSingleSelectedNode();
            NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
            const int index = parentProperty.indexOf(modelNode);

            if (index != 0)
                parentProperty.slide(index, 0);
        }

    } catch (const RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

enum OrderAction {RaiseItem, LowerItem};

void changeOrder(const SelectionContext &selectionState, OrderAction orderAction)
{
    if (!selectionState.view())
        return;

    QTC_ASSERT(selectionState.singleNodeIsSelected(), return);
    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    if (modelNode.isRootNode())
        return;
    if (!modelNode.parentProperty().isNodeListProperty())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|changeOrder", [orderAction, selectionState, modelNode]() {
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
    });
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
        selectionState.selectedModelNodes().constFirst().variantProperty("visible").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better safe than sorry
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
    } catch (const RewritingException &e) { //better safe than sorry
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
    } catch (const RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void resetSize(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|resetSize",[selectionState](){
        const QList<ModelNode> nodes = selectionState.selectedModelNodes();
        for (const ModelNode &node : nodes) {
            QmlItemNode itemNode(node);
            if (itemNode.isValid()) {
                itemNode.removeProperty("width");
                itemNode.removeProperty("height");
            }
        }
    });
}

void resetPosition(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|resetPosition",[selectionState](){
        const QList<ModelNode> nodes = selectionState.selectedModelNodes();
        for (const ModelNode &node : nodes) {
            QmlItemNode itemNode(node);
            if (itemNode.isValid()) {
                itemNode.removeProperty("x");
                itemNode.removeProperty("y");
            }
        }
    });
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

    selectionState.view()->executeInTransaction("DesignerActionManager|resetZ", [selectionState](){
        for (ModelNode node : selectionState.selectedModelNodes()) {
            QmlItemNode itemNode(node);
            if (itemNode.isValid())
                itemNode.removeProperty("z");
        }
    });
}

void reverse(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|reverse", [selectionState](){
        NodeListProperty::reverseModelNodes(selectionState.selectedModelNodes());
    });
}

inline static void backupPropertyAndRemove(const ModelNode &node, const PropertyName &propertyName)
{
    if (node.hasVariantProperty(propertyName)) {
        node.setAuxiliaryData(AuxiliaryDataType::Document,
                              auxPropertyString(propertyName),
                              node.variantProperty(propertyName).value());
        node.removeProperty(propertyName);

    }
    if (node.hasBindingProperty(propertyName)) {
        node.setAuxiliaryData(AuxiliaryDataType::Document,
                              auxPropertyString(propertyName),
                              QmlItemNode(node).instanceValue(propertyName));
        node.removeProperty(propertyName);
    }
}

static void restoreProperty(const ModelNode &node, const PropertyName &propertyName)
{
    if (auto data = node.auxiliaryData(AuxiliaryDataType::Document, auxPropertyString(propertyName)))
        node.variantProperty(propertyName).setValue(*data);
}

void anchorsFill(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|anchorsFill",[selectionState](){
        ModelNode modelNode = selectionState.currentSingleSelectedNode();

        QmlItemNode node = modelNode;
        if (node.isValid()) {
            node.anchors().fill();
            backupPropertyAndRemove(modelNode, "x");
            backupPropertyAndRemove(modelNode, "y");
            backupPropertyAndRemove(modelNode, "width");
            backupPropertyAndRemove(modelNode, "height");
        }
    });
}

void anchorsReset(const SelectionContext &selectionState)
{
    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|anchorsReset",[selectionState](){
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
    });
}

using LessThan = std::function<bool (const ModelNode &, const ModelNode&)>;

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
                                 const TypeName &layoutType,
                                 const LessThan &lessThan)
{
    if (!selectionContext.view()
             || !selectionContext.view()->model()->hasNodeMetaInfo(layoutType))
        return;

    if (QmlItemNode::isValidQmlItemNode(selectionContext.firstSelectedModelNode())) {
        const QmlItemNode qmlItemNode = QmlItemNode(selectionContext.firstSelectedModelNode());

        if (qmlItemNode.hasInstanceParentItem()) {

            selectionContext.view()->executeInTransaction("DesignerActionManager|layoutHelperFunction",[=](){

                QmlItemNode parentNode = qmlItemNode.instanceParentItem();

                NodeMetaInfo metaInfo = selectionContext.view()->model()->metaInfo(layoutType);

                const ModelNode layoutNode = selectionContext.view()->createModelNode(layoutType, metaInfo.majorVersion(), metaInfo.minorVersion());

                reparentTo(layoutNode, parentNode);

                QList<ModelNode> sortedSelectedNodes =  selectionContext.selectedModelNodes();
                Utils::sort(sortedSelectedNodes, lessThan);

                setUpperLeftPostionToNode(layoutNode, sortedSelectedNodes);
                LayoutInGridLayout::reparentToNodeAndRemovePositionForModelNodes(layoutNode, sortedSelectedNodes);
                if (layoutType.contains("Layout"))
                    LayoutInGridLayout::setSizeAsPreferredSize(sortedSelectedNodes);
            });
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
    } catch (RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void layoutColumnLayout(const SelectionContext &selectionContext)
{
    try {
        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        layoutHelperFunction(selectionContext, "QtQuick.Layouts.ColumnLayout", compareByY);
    } catch (RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void layoutGridLayout(const SelectionContext &selectionContext)
{
    try {
        Q_ASSERT(!DesignerMcuManager::instance().isMCUProject()); //remove this line when grids are finally supported

        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        LayoutInGridLayout::layout(selectionContext);
    } catch (RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

static PropertyNameList sortedPropertyNameList(const PropertyMetaInfos &properties)
{
    auto propertyNames = Utils::transform<PropertyNameList>(properties, [](const auto &property) {
        return property.name();
    });

    std::sort(propertyNames.begin(), propertyNames.end());

    propertyNames.erase(std::unique(propertyNames.begin(), propertyNames.end()), propertyNames.end());

    return propertyNames;
}

static QString toUpper(const QString &signal)
{
    QString ret = signal;
    ret[0] = signal.at(0).toUpper();
    return ret;
}

static void addSignal(const QString &typeName,
                      const QString &itemId,
                      const QString &signalName,
                      bool isRootModelNode,
                      ExternalDependenciesInterface &externanDependencies)
{
    auto model = Model::create("Item", 2, 0);
    RewriterView rewriterView(externanDependencies, RewriterView::Amend);

    auto textEdit = qobject_cast<TextEditor::TextEditorWidget*>
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
    const QList<ModelNode> nodes = rewriterView.allModelNodes();
    for (const ModelNode &modelNode : nodes) {
        if (modelNode.type() == typeName.toUtf8()) {
            modelNode.signalHandlerProperty(signalHandlerName).setSource(QLatin1String("{\n}"));
        }
    }
}

static QStringList cleanSignalNames(const QStringList &input)
{
    QStringList output;

    for (const QString &signal : input)
        if (!signal.startsWith(QLatin1String("__")) && !output.contains(signal))
            output.append(signal);

    return output;
}

static QStringList getSortedSignalNameList(const ModelNode &modelNode)
{
    NodeMetaInfo metaInfo = modelNode.metaInfo();
    QStringList signalNames;

    if (metaInfo.isValid()) {
        // TODO seem to be broken because there can be properties without notifier and the notifier can be even have a different name

        const PropertyNameList signalNameList = metaInfo.signalNames();
        for (const PropertyName &signalName : signalNameList)
            if (!signalName.contains("Changed"))
                signalNames.append(QString::fromUtf8(signalName));

        const PropertyNameList propertyNameList = sortedPropertyNameList(metaInfo.properties());
        for (const PropertyName &propertyName : propertyNameList)
            if (!propertyName.contains("."))
                signalNames.append(QString::fromUtf8(propertyName + "Changed"));
    }

    return signalNames;
}

void addSignalHandlerOrGotoImplementation(const SelectionContext &selectionState, bool addAlwaysNewSlot)
{
    ModelNode modelNode;
    if (selectionState.singleNodeIsSelected())
        modelNode = selectionState.selectedModelNodes().constFirst();

    bool isModelNodeRoot = true;

    QmlObjectNode qmlObjectNode(modelNode);

    if (!qmlObjectNode.isValid()) {
        QString title = QCoreApplication::translate("ModelNodeOperations", "Go to Implementation");
        QString description = QCoreApplication::translate("ModelNodeOperations", "Invalid component.");
        Core::AsynchronousMessageBox::warning(title, description);
        return;
    }

    if (!qmlObjectNode.isRootModelNode()) {
        isModelNodeRoot = false;
        qmlObjectNode.view()->executeInTransaction("NavigatorTreeModel:exportItem", [&qmlObjectNode](){
            qmlObjectNode.ensureAliasExport();
        });
    }

    QString itemId = modelNode.id();

    const Utils::FilePath currentDesignDocument = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()->fileName();
    const QString fileName = currentDesignDocument.toString();
    const QString typeName = currentDesignDocument.baseName();

    QStringList signalNames = cleanSignalNames(getSortedSignalNameList(selectionState.selectedModelNodes().constFirst()));

    QList<QmlJSEditor::FindReferences::Usage> usages
        = QmlJSEditor::FindReferences::findUsageOfType(currentDesignDocument, typeName);

    if (usages.isEmpty()) {
        QString title = QCoreApplication::translate("ModelNodeOperations", "Go to Implementation");
        QString description = QCoreApplication::translate("ModelNodeOperations", "Cannot find an implementation.");
        Core::AsynchronousMessageBox::warning(title, description);
        return;
    }

    usages = FindImplementation::run(usages.constFirst().path.toString(), typeName, itemId);

    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);

    if (!usages.isEmpty() && (addAlwaysNewSlot || usages.size() < 2)
        && (!isModelNodeRoot || addAlwaysNewSlot)) {
        Core::EditorManager::openEditorAt(
            {usages.constFirst().path, usages.constFirst().line, usages.constFirst().col});

        if (!signalNames.isEmpty()) {
            auto dialog = new AddSignalHandlerDialog(Core::ICore::dialogParent());
            dialog->setSignals(signalNames);

            AddSignalHandlerDialog::connect(dialog, &AddSignalHandlerDialog::signalSelected, [=] {
                dialog->deleteLater();

                if (dialog->signal().isEmpty())
                    return;

                qmlObjectNode.view()->executeInTransaction("NavigatorTreeModel:exportItem", [=]() {
                    addSignal(typeName,
                              itemId,
                              dialog->signal(),
                              isModelNodeRoot,
                              selectionState.view()->externalDependencies());
                });

                addSignal(typeName,
                          itemId,
                          dialog->signal(),
                          isModelNodeRoot,
                          selectionState.view()->externalDependencies());

                //Move cursor to correct curser position
                const QString filePath = Core::EditorManager::currentDocument()->filePath().toString();
                QList<QmlJSEditor::FindReferences::Usage> usages = FindImplementation::run(filePath, typeName, itemId);
                Core::EditorManager::openEditorAt({Utils::FilePath::fromString(filePath),
                                                   usages.constFirst().line,
                                                   usages.constFirst().col + 1});
            } );
            dialog->show();

        }
        return;
    }

    Core::EditorManager::openEditorAt(
        {usages.constFirst().path, usages.constFirst().line, usages.constFirst().col + 1});
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

    selectionContext.view()->executeInTransaction("DesignerActionManager|removeLayout", [selectionContext, &layoutItem, parent](){
        const QList<ModelNode> modelNodes = selectionContext.currentSingleSelectedNode().directSubModelNodes();
        for (const ModelNode &modelNode : modelNodes) {
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
            parent.modelNode().defaultNodeListProperty().reparentHere(modelNode);
        }
        layoutItem.destroy();
    });
}

void removePositioner(const SelectionContext &selectionContext)
{
    removeLayout(selectionContext);
}

void moveToComponent(const SelectionContext &selectionContext)
{
    ModelNode modelNode;
    if (selectionContext.singleNodeIsSelected())
        modelNode = selectionContext.selectedModelNodes().constFirst();

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

// Open a model's material in the material editor
void editMaterial(const SelectionContext &selectionContext)
{
    ModelNode modelNode = selectionContext.targetNode();

    if (!modelNode.isValid())
        modelNode = selectionContext.currentSingleSelectedNode();

    QTC_ASSERT(modelNode.isValid(), return);

    BindingProperty prop = modelNode.bindingProperty("materials");
    if (!prop.exists())
        return;

    AbstractView *view = selectionContext.view();

    ModelNode material;

    if (view->hasId(prop.expression())) {
        material = view->modelNodeForId(prop.expression());
    } else {
        QList<ModelNode> materials = prop.resolveToModelNodeList();

        if (materials.size() > 0)
            material = materials.first();
    }

    if (material.isValid()) {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialEditor");

        // to MaterialBrowser...
        view->emitCustomNotification("select_material", {material});
    }
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

            if (!potentialTabBar.metaInfo().isQtQuickControlsTabBar())
                potentialTabBar = ModelNode();
        }
    }

    view->executeInTransaction("DesignerActionManager:addItemToStackedContainer", [=](){

        NodeMetaInfo itemMetaInfo = view->model()->metaInfo("QtQuick.Item", -1, -1);
        QTC_ASSERT(itemMetaInfo.isValid(), return);
        QTC_ASSERT(itemMetaInfo.majorVersion() == 2, return);

        QmlDesigner::ModelNode itemNode =
                view->createModelNode("QtQuick.Item", itemMetaInfo.majorVersion(), itemMetaInfo.minorVersion());

        container.defaultNodeListProperty().reparentHere(itemNode);

        if (potentialTabBar.isValid()) {// The stacked container is hooked up to a TabBar
            NodeMetaInfo tabButtonMetaInfo = view->model()->metaInfo("QtQuick.Controls.TabButton", -1, -1);
            if (tabButtonMetaInfo.isValid()) {
                const int buttonIndex = potentialTabBar.directSubModelNodes().size();
                ModelNode tabButtonNode =
                        view->createModelNode("QtQuick.Controls.TabButton",
                                              tabButtonMetaInfo.majorVersion(),
                                              tabButtonMetaInfo.minorVersion());

                tabButtonNode.variantProperty("text").setValue(QString::fromLatin1("Tab %1").arg(buttonIndex));
                potentialTabBar.defaultNodeListProperty().reparentHere(tabButtonNode);

            }
        }
    });
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

static void setIndexProperty(const AbstractProperty &property, const QVariant &value)
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

    const int maxValue = container.directSubModelNodes().size();

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

    view->executeInTransaction("DesignerActionManager:addItemToStackedContainer",
                               [view, container, containerItemNode, tabBarMetaInfo, tabButtonMetaInfo, indexPropertyName](){

        ModelNode tabBarNode =
                view->createModelNode("QtQuick.Controls.TabBar",
                                      tabBarMetaInfo.majorVersion(),
                                      tabBarMetaInfo.minorVersion());

        container.parentProperty().reparentHere(tabBarNode);

        const int maxValue = container.directSubModelNodes().size();

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
    });

}

AddFilesResult addFilesToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    QString directory = showDialog ? AddImagesDialog::getDirectory(fileNames, defaultDir) : defaultDir;
    if (directory.isEmpty())
        return AddFilesResult::cancelled(directory);

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    QTC_ASSERT(document, return AddFilesResult::failed(directory));

    QList<QPair<QString, QString>> copyList;
    QStringList removeList;
    for (const QString &fileName : fileNames) {
        const QString targetFile = directory + "/" + QFileInfo(fileName).fileName();
        Utils::FilePath srcFilePath = Utils::FilePath::fromString(fileName);
        Utils::FilePath targetFilePath = Utils::FilePath::fromString(targetFile);
        if (targetFilePath.exists()) {
            if (srcFilePath.lastModified() == targetFilePath.lastModified())
                continue;
            const QString title = QCoreApplication::translate(
                        "ModelNodeOperations", "Overwrite Existing File?");
            const QString question = QCoreApplication::translate(
                        "ModelNodeOperations", "File already exists. Overwrite?\n\"%1\"").arg(targetFile);
            if (QMessageBox::question(qobject_cast<QWidget *>(Core::ICore::dialogParent()),
                                      title, question, QMessageBox::Yes | QMessageBox::No)
                    != QMessageBox::Yes) {
                continue;
            }
            removeList.append(targetFile);
        }
        copyList.append({fileName, targetFile});
    }
    // Defer actual file operations after we have dealt with possible popup dialogs to avoid
    // unnecessarily refreshing file models multiple times during the operation
    for (const auto &file : std::as_const(removeList))
        QFile::remove(file);

    for (const auto &filePair : std::as_const(copyList)) {
        const bool success = QFile::copy(filePair.first, filePair.second);
        if (!success)
            return AddFilesResult::failed(directory);

        ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(document->fileName());
        if (node) {
            ProjectExplorer::FolderNode *containingFolder = node->parentFolderNode();
            if (containingFolder)
                containingFolder->addFiles({Utils::FilePath::fromString(filePair.second)});
        }
    }

    return AddFilesResult::succeeded(directory);
}

static QString getAssetDefaultDirectory(const QString &assetDir, const QString &defaultDirectory)
{
    QString adjustedDefaultDirectory = defaultDirectory;

    Utils::FilePath contentPath = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath();

    if (contentPath.pathAppended("content").exists())
        contentPath = contentPath.pathAppended("content");

    Utils::FilePath assetPath = contentPath.pathAppended(assetDir);

    if (!assetPath.exists()) {
        // Create the default asset type directory if it doesn't exist
        QDir dir(contentPath.toString());
        dir.mkpath(assetDir);
    }

    if (assetPath.exists() && assetPath.isDir())
        adjustedDefaultDirectory = assetPath.toString();

    return adjustedDefaultDirectory;
}

AddFilesResult addFontToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    return addFilesToProject(fileNames, getAssetDefaultDirectory("fonts", defaultDir), showDialog);
}

AddFilesResult addSoundToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    return addFilesToProject(fileNames, getAssetDefaultDirectory("sounds", defaultDir), showDialog);
}

AddFilesResult addShaderToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    return addFilesToProject(fileNames, getAssetDefaultDirectory("shaders", defaultDir), showDialog);
}

AddFilesResult addImageToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    return addFilesToProject(fileNames, getAssetDefaultDirectory("images", defaultDir), showDialog);
}

AddFilesResult addVideoToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    return addFilesToProject(fileNames, getAssetDefaultDirectory("videos", defaultDir), showDialog);
}

void createFlowActionArea(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    NodeMetaInfo actionAreaMetaInfo = view->model()->metaInfo("FlowView.FlowActionArea", -1, -1);
    QTC_ASSERT(actionAreaMetaInfo.isValid(), return);

    const QPointF pos = selectionContext.scenePosition().isNull() ? QPointF() : selectionContext.scenePosition() - QmlItemNode(container).flowPosition();

    view->executeInTransaction("DesignerActionManager:createFlowActionArea",
                               [view, container, actionAreaMetaInfo, pos](){

                                   ModelNode flowActionNode =
                                       view->createModelNode("FlowView.FlowActionArea",
                                                             actionAreaMetaInfo.majorVersion(),
                                                             actionAreaMetaInfo.minorVersion());

                                   if (!pos.isNull()) {
                                       flowActionNode.variantProperty("x").setValue(pos.x());
                                       flowActionNode.variantProperty("y").setValue(pos.y());
                                   }

                                   container.defaultNodeListProperty().reparentHere(flowActionNode);
                                   view->setSelectedModelNode(flowActionNode);
                               });

}

void addTransition(const SelectionContext &selectionContext)
{
    if (selectionContext.view()) {
        AbstractView *view = selectionContext.view();
        QmlFlowTargetNode targetNode = selectionContext.targetNode();
        QmlFlowTargetNode sourceNode = selectionContext.currentSingleSelectedNode();

        QTC_ASSERT(targetNode.isValid(), return);
        QTC_ASSERT(sourceNode.isValid(), return);



        view->executeInTransaction("DesignerActionManager:addTransition",
                                   [targetNode, &sourceNode](){
                                       sourceNode.assignTargetItem(targetNode);
                                   });
    }
}

void addFlowEffect(const SelectionContext &selectionContext, const TypeName &typeName)
{
   AbstractView *view = selectionContext.view();

   QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
   ModelNode container = selectionContext.currentSingleSelectedNode();
   QTC_ASSERT(container.isValid(), return);
   QTC_ASSERT(container.metaInfo().isValid(), return);
   QTC_ASSERT(QmlItemNode::isFlowTransition(container), return);

   NodeMetaInfo effectMetaInfo = view->model()->metaInfo("FlowView." + typeName, -1, -1);
   QTC_ASSERT(typeName == "None" || effectMetaInfo.isValid(), return);

   view->executeInTransaction("DesignerActionManager:addFlowEffect", [=]() {
       if (container.hasProperty("effect"))
           container.removeProperty("effect");

       if (effectMetaInfo.isQtObject()) {
           ModelNode effectNode = view->createModelNode(useProjectStorage()
                                                            ? typeName
                                                            : effectMetaInfo.typeName(),
                                                        effectMetaInfo.majorVersion(),
                                                        effectMetaInfo.minorVersion());

           container.nodeProperty("effect").reparentHere(effectNode);
           view->setSelectedModelNode(effectNode);
       }
   });
}

void setFlowStartItem(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode node = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(node.isValid(), return);
    QTC_ASSERT(node.metaInfo().isValid(), return);
    QmlFlowItemNode flowItem(node);
    QTC_ASSERT(flowItem.isValid(), return);
    QTC_ASSERT(flowItem.flowView().isValid(), return);
    view->executeInTransaction("DesignerActionManager:setFlowStartItem",
                               [&flowItem](){
        flowItem.flowView().setStartFlowItem(flowItem);
    });
}

static bool hasStudioComponentsImport(const SelectionContext &context)
{
    if (context.view() && context.view()->model()) {
        Import import = Import::createLibraryImport("QtQuick.Studio.Components", "1.0");
        return context.view()->model()->hasImport(import, true, true);
    }

    return false;
}

inline static void setAdjustedPos(const QmlDesigner::ModelNode &modelNode)
{
    if (modelNode.hasParentProperty()) {
        ModelNode parentNode = modelNode.parentProperty().parentModelNode();

        const QPointF instancePos = QmlItemNode(modelNode).instancePosition();
        const int x = instancePos.x() - parentNode.variantProperty("x").value().toInt();
        const int y = instancePos.y() - parentNode.variantProperty("y").value().toInt();

        modelNode.variantProperty("x").setValue(x);
        modelNode.variantProperty("y").setValue(y);
    }
}

void reparentToNodeAndAdjustPosition(const ModelNode &parentModelNode,
                                     const QList<ModelNode> &modelNodeList)
{
    for (const ModelNode &modelNode : modelNodeList) {
        reparentTo(modelNode, parentModelNode);
        setAdjustedPos(modelNode);

        for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
            if (variantProperty.name().contains("anchors."))
                modelNode.removeProperty(variantProperty.name());
        }
        for (const BindingProperty &bindingProperty : modelNode.bindingProperties()) {
            if (bindingProperty.name().contains("anchors."))
                modelNode.removeProperty(bindingProperty.name());
        }
    }
}

void addToGroupItem(const SelectionContext &selectionContext)
{
    const TypeName typeName = "QtQuick.Studio.Components.GroupItem";

    try {
        if (!hasStudioComponentsImport(selectionContext)) {
            Import studioImport = Import::createLibraryImport("QtQuick.Studio.Components", "1.0");
            selectionContext.view()-> model()->changeImports({studioImport}, {});
        }

        if (!selectionContext.view())
            return;

        if (QmlItemNode::isValidQmlItemNode(selectionContext.firstSelectedModelNode())) {
            const QmlItemNode qmlItemNode = QmlItemNode(selectionContext.firstSelectedModelNode());

            if (qmlItemNode.hasInstanceParentItem()) {
                ModelNode groupNode;
                selectionContext.view()->executeInTransaction("DesignerActionManager|addToGroupItem1",[=, &groupNode](){

                    QmlItemNode parentNode = qmlItemNode.instanceParentItem();
                    NodeMetaInfo metaInfo = selectionContext.view()->model()->metaInfo(typeName);
                    groupNode = selectionContext.view()->createModelNode(typeName, metaInfo.majorVersion(), metaInfo.minorVersion());
                    reparentTo(groupNode, parentNode);
                });
                selectionContext.view()->executeInTransaction("DesignerActionManager|addToGroupItem2",[=](){

                    QList<ModelNode> selectedNodes = selectionContext.selectedModelNodes();
                    setUpperLeftPostionToNode(groupNode, selectedNodes);

                    reparentToNodeAndAdjustPosition(groupNode, selectedNodes);
                });
            }
        }
    } catch (RewritingException &e) {
        e.showException();
    }
}

void selectFlowEffect(const SelectionContext &selectionContext)
{
    if (!selectionContext.singleNodeIsSelected())
        return;

    ModelNode node = selectionContext.currentSingleSelectedNode();
    QmlVisualNode transition(node);

    QTC_ASSERT(transition.isValid(), return);
    QTC_ASSERT(transition.isFlowTransition(), return);

    if (node.hasNodeProperty("effect")) {
        selectionContext.view()->setSelectedModelNode(node.nodeProperty("effect").modelNode());
    }
}

static QString baseDirectory(const QUrl &url)
{
    QString filePath = url.toLocalFile();
    return QFileInfo(filePath).absoluteDir().path();
}

static void getTypeAndImport(const SelectionContext &selectionContext,
                             QString &type,
                             QString &import)
{
    static QString s_lastBrowserPath;
    QString path = s_lastBrowserPath;

    if (path.isEmpty())
        path = baseDirectory(selectionContext.view()->model()->fileUrl());

    QString newFile = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
                                                   ComponentCoreConstants::addCustomEffectDialogDisplayString,
                                                   path,
                                                   "*.qml");

    if (!newFile.isEmpty()) {
        QFileInfo file(newFile);

        type = file.fileName();
        type.remove(".qml");

        s_lastBrowserPath = file.absolutePath();

        import = QFileInfo(s_lastBrowserPath).baseName();
    }
}

void addCustomFlowEffect(const SelectionContext &selectionContext)
{

    TypeName typeName;

    QString typeString;
    QString importString;

    getTypeAndImport(selectionContext, typeString, importString);

    typeName = typeString.toUtf8();

    if (typeName.isEmpty())
        return;

    AbstractView *view = selectionContext.view();

    view->executeInTransaction("DesignerActionManager:addFlowEffect", [view, importString]() {
        const Import import = Import::createFileImport("FlowEffects");

        if (!importString.isEmpty() && !view->model()->hasImport(import, true, true)) {
            view->model()->changeImports({import}, {});
        }
    });

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);
    QTC_ASSERT(QmlItemNode::isFlowTransition(container), return);

    NodeMetaInfo effectMetaInfo = view->model()->metaInfo(typeName, -1, -1);
    QTC_ASSERT(typeName == "None" || effectMetaInfo.isValid(), return);

    view->executeInTransaction("DesignerActionManager:addFlowEffect", [=]() {
        if (container.hasProperty("effect"))
            container.removeProperty("effect");

        if (effectMetaInfo.isValid()) {
            ModelNode effectNode = view->createModelNode(useProjectStorage()
                                                             ? typeName
                                                             : effectMetaInfo.typeName(),
                                                         effectMetaInfo.majorVersion(),
                                                         effectMetaInfo.minorVersion());

            container.nodeProperty("effect").reparentHere(effectNode);
            view->setSelectedModelNode(effectNode);
        }
    });
}

static QString fromCamelCase(const QString &s)
{
    static QRegularExpression regExp1 {"(.)([A-Z][a-z]+)"};
    static QRegularExpression regExp2 {"([a-z0-9])([A-Z])"};

    QString result = s;
    result.replace(regExp1, "\\1 \\2");
    result.replace(regExp2, "\\1 \\2");

    return result;
}

QString getTemplateDialog(const Utils::FilePath &projectPath)
{

    const Utils::FilePath templatesPath = projectPath.pathAppended("templates");

    const QStringList templateFiles = QDir(templatesPath.toString()).entryList({"*.qml"});

    QStringList names;

    for (const QString &name : templateFiles) {
        QString cleanS = name;
        cleanS.remove(".qml");
        names.append(fromCamelCase(cleanS));
    }

    QDialog *dialog = new QDialog(Core::ICore::dialogParent());
    dialog->setMinimumWidth(480);
    dialog->setModal(true);

    dialog->setWindowTitle(QCoreApplication::translate("TemplateMerge","Merge With Template"));

    auto mainLayout = new QGridLayout(dialog);

    auto comboBox = new QComboBox;

    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    for (const QString &templateName :  names)
        comboBox->addItem(templateName);

    QString templateFile;

    auto setTemplate = [comboBox, &templateFile](const QString &newFile) {
        if (comboBox->findText(newFile) < 0)
            comboBox->addItem(newFile);

        comboBox->setCurrentText(newFile);
        templateFile = newFile;
    };

    QPushButton *browseButton = new QPushButton(QCoreApplication::translate("TemplateMerge", "&Browse..."), dialog);

    mainLayout->addWidget(new QLabel(QCoreApplication::translate("TemplateMerge", "Template:")), 0, 0);
    mainLayout->addWidget(comboBox, 1, 0, 1, 3);
    mainLayout->addWidget(browseButton, 1, 3, 1 , 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox, 2, 2, 1, 2);

    QObject::connect(browseButton, &QPushButton::clicked, dialog, [setTemplate, &projectPath]() {

        const QString newFile = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
                                                             QCoreApplication::translate("TemplateMerge", "Browse Template"),
                                                             projectPath.toString(),
                                                             "*.qml");
        if (!newFile.isEmpty())
            setTemplate(newFile);
    });

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, [dialog](){
        dialog->accept();
        dialog->deleteLater();
    });

    QString result;

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, [dialog](){
        dialog->reject();
        dialog->deleteLater();
    });

    QObject::connect(dialog, &QDialog::accepted, [&result, comboBox](){
        result = comboBox->currentText();
    });

    dialog->exec();

    if (!result.isEmpty() && !QFileInfo::exists(result)) {
        result = templateFiles.at(names.indexOf(result));
        result = templatesPath.pathAppended(result).toString();
    }

    return result;
}

void mergeWithTemplate(const SelectionContext &selectionContext, ExternalDependenciesInterface &externalDependencies)
{
    const Utils::FilePath projectPath = Utils::FilePath::fromString(baseDirectory(selectionContext.view()->model()->fileUrl()));

    const QString templateFile = getTemplateDialog(projectPath);

    if (QFileInfo::exists(templateFile)) {
        StylesheetMerger::styleMerge(Utils::FilePath::fromString(templateFile),
                                     selectionContext.view()->model(),
                                     externalDependencies);
    }
}

void removeGroup(const SelectionContext &selectionContext)
{
    if (!selectionContext.view() || !selectionContext.hasSingleSelectedModelNode())
        return;

    ModelNode group = selectionContext.currentSingleSelectedNode();

    if (!QmlItemNode::isValidQmlItemNode(group))
        return;

    QmlItemNode groupItem(group);

    QmlItemNode parent = groupItem.instanceParentItem();

    if (!parent.isValid())
        return;

    selectionContext.view()->executeInTransaction(
        "DesignerActionManager::removeGroup", [selectionContext, &groupItem, parent]() {
            for (const ModelNode &modelNode :
                 selectionContext.currentSingleSelectedNode().directSubModelNodes()) {
                if (QmlItemNode qmlItem = modelNode) {
                    QPointF pos = qmlItem.instancePosition();
                    pos = groupItem.instanceTransform().map(pos);
                    modelNode.variantProperty("x").setValue(pos.x());
                    modelNode.variantProperty("y").setValue(pos.y());

                    parent.modelNode().defaultNodeListProperty().reparentHere(modelNode);
                }
            }
            groupItem.destroy();
    });
}

void editAnnotation(const SelectionContext &selectionContext)
{
    ModelNode selectedNode = selectionContext.currentSingleSelectedNode();

    ModelNodeEditorProxy::fromModelNode<AnnotationEditor>(selectedNode);
}

void addMouseAreaFill(const SelectionContext &selectionContext)
{
    if (!selectionContext.isValid()) {
        return;
    }

    if (!selectionContext.singleNodeIsSelected()) {
        return;
    }

    selectionContext.view()->executeInTransaction("DesignerActionManager|addMouseAreaFill", [selectionContext]() {
        ModelNode modelNode = selectionContext.currentSingleSelectedNode();
        if (modelNode.isValid()) {
            NodeMetaInfo itemMetaInfo = selectionContext.view()->model()->metaInfo("QtQuick.MouseArea", -1, -1);
            QTC_ASSERT(itemMetaInfo.isValid(), return);

            QmlDesigner::ModelNode mouseAreaNode =
                selectionContext.view()->createModelNode("QtQuick.MouseArea", itemMetaInfo.majorVersion(), itemMetaInfo.minorVersion());
            mouseAreaNode.validId();

            modelNode.defaultNodeListProperty().reparentHere(mouseAreaNode);
            QmlItemNode mouseAreaItemNode(mouseAreaNode);
            if (mouseAreaItemNode.isValid()) {
                mouseAreaItemNode.anchors().fill();
            }
        }
    });
}

QVariant previewImageDataForGenericNode(const ModelNode &modelNode)
{
    if (modelNode.isValid())
        return modelNode.model()->nodeInstanceView()->previewImageDataForGenericNode(modelNode, {});
    return {};
}

QVariant previewImageDataForImageNode(const ModelNode &modelNode)
{
    if (modelNode.isValid())
        return modelNode.model()->nodeInstanceView()->previewImageDataForImageNode(modelNode);
    return {};
}

void openSignalDialog(const SelectionContext &selectionContext)
{
    if (!selectionContext.view() || !selectionContext.hasSingleSelectedModelNode())
        return;

    SignalList::showWidget(selectionContext.currentSingleSelectedNode());
}

void updateImported3DAsset(const SelectionContext &selectionContext)
{
    if (selectionContext.view()) {
        selectionContext.view()->emitCustomNotification(
                    "UpdateImported3DAsset", {selectionContext.currentSingleSelectedNode()});
    }
}

void openEffectMaker(const QString &filePath)
{
    const ProjectExplorer::Target *target = ProjectExplorer::ProjectTree::currentTarget();
    if (!target) {
        qWarning() << __FUNCTION__ << "No project open";
        return;
    }

    Utils::FilePath projectPath = target->project()->projectDirectory();
    QString effectName = QFileInfo(filePath).baseName();
    QString effectResDir = QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER) + "/Effects/" + effectName;
    Utils::FilePath effectResPath = projectPath.pathAppended(effectResDir);
    if (!effectResPath.exists())
        QDir().mkpath(effectResPath.toString());

    const QtSupport::QtVersion *baseQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (baseQtVersion) {
        Utils::Environment env = Utils::Environment::systemEnvironment();

        auto effectMakerPath = baseQtVersion->binPath().pathAppended("qqem").withExecutableSuffix();
        if (!effectMakerPath.exists() && env.osType() == Utils::OsTypeMac)
            effectMakerPath = baseQtVersion->binPath().pathAppended("qqem.app/Contents/MacOS/qqem");
        if (!effectMakerPath.exists()) {
            qWarning() << __FUNCTION__ << "Cannot find EffectMaker app";
            return;
        }

        Utils::FilePath effectPath = Utils::FilePath::fromString(filePath);
        QStringList arguments;
        arguments << filePath;
        if (effectPath.fileContents()->isEmpty())
            arguments << "--create";
        arguments << "--exportpath" << effectResPath.toString();

        if (env.osType() == Utils::OsTypeMac)
            env.appendOrSet("QSG_RHI_BACKEND", "metal");

        Utils::Process *qqemProcess = new Utils::Process();
        qqemProcess->setEnvironment(env);
        qqemProcess->setCommand({ effectMakerPath, arguments });
        QObject::connect(qqemProcess, &Utils::Process::done, [qqemProcess]() {
            qqemProcess->deleteLater();
        });
        qqemProcess->start();
    }
}

Utils::FilePath getEffectsImportDirectory()
{
    QString defaultDir = QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER) + "/Effects";
    Utils::FilePath projectPath = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath();
    Utils::FilePath effectsPath = projectPath.pathAppended(defaultDir);

    if (!effectsPath.exists()) {
        QDir dir(projectPath.toString());
        dir.mkpath(defaultDir);
    }

    return effectsPath;
}

QString getEffectsDefaultDirectory(const QString &defaultDir)
{
    return getAssetDefaultDirectory("effects", defaultDir);
}

QString getEffectIcon(const QString &effectPath)
{
    Utils::FilePath projectPath = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath();
    QString effectName = QFileInfo(effectPath).baseName();
    QString effectResDir = "asset_imports/Effects/" + effectName;
    Utils::FilePath effectResPath = projectPath.resolvePath(effectResDir + "/" + effectName + ".qml");

    return effectResPath.exists() ? QString("effectExported") : QString("effectClass");
}

bool useLayerEffect()
{
    QtcSettings *settings = Core::ICore::settings();
    const Key layerEffectEntry = "QML/Designer/UseLayerEffect";

    return settings->value(layerEffectEntry, true).toBool();
}

bool validateEffect(const QString &effectPath)
{
    const QString effectName = QFileInfo(effectPath).baseName();
    Utils::FilePath effectsResDir = ModelNodeOperations::getEffectsImportDirectory();
    Utils::FilePath qmlPath = effectsResDir.resolvePath(effectName + "/" + effectName + ".qml");
    if (!qmlPath.exists()) {
        QMessageBox msgBox;
        msgBox.setText(QObject::tr("Effect %1 is not complete.").arg(effectName));
        msgBox.setInformativeText(QObject::tr("Ensure that you have saved it in Qt Quick Effect Maker."
                                              "\nDo you want to edit this effect?"));
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setIcon(QMessageBox::Question);
        if (msgBox.exec() == QMessageBox::Yes)
            ModelNodeOperations::openEffectMaker(effectPath);
        return false;
    }
    return true;
}

Utils::FilePath getImagesDefaultDirectory()
{
    return Utils::FilePath::fromString(
                getAssetDefaultDirectory(
        "images", QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toString()));
}

} // namespace ModelNodeOperations

} //QmlDesigner
