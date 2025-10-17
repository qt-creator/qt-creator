// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodeoperations.h"

#include "addimagesdialog.h"
#include "addsignalhandlerdialog.h"
#include "componentcore_constants.h"
#include "componentcoretracing.h"
#include "createtexture.h"
#include "findimplementation.h"
#include "layoutingridlayout.h"
#include "modelnodecontextmenu_helper.h"
#include "utils3d.h"

#include <bindingproperty.h>
#include <choosefrompropertylistdialog.h>
#include <designdocumentview.h>
#include <designermcumanager.h>
#include <designmodewidget.h>
#include <documentmanager.h>
#include <itemlibraryentry.h>
#include <modelmerger.h>
#include <modelnode.h>
#include <modelnodeutils.h>
#include <modelutils.h>
#include <nodehints.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <qmlanchors.h>
#include <qmlitemnode.h>
#include <rewritertransaction.h>
#include <rewritingexception.h>
#include <signalhandlerproperty.h>
#include <stylesheetmerger.h>
#include <variantproperty.h>

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>

#include <annotationeditor/annotationeditor.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include "projectexplorer/target.h"

#include <qmljseditor/qmljsfindreferences.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/progressdialog.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/smallstring.h>

#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QThread>

#include <algorithm>
#include <functional>
#include <limits>

#include <bindingeditor/signallist.h>

using namespace Utils;

namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

namespace {
const Utils::SmallString auxDataString("anchors_");

Utils::SmallString auxPropertyString(Utils::SmallStringView name)
{
    return auxDataString + name;
}

QString relativePathToQmlFile(const QString &absolutePath)
{
    NanotraceHR::Tracer tracer{"model node operations relative path to qml file", category()};

    return DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(absolutePath);
}

void reparentTo(const ModelNode &node, const QmlItemNode &parent)
{
    NanotraceHR::Tracer tracer{"model node operations reparent to", category()};

    if (parent.isValid() && node.isValid()) {
        NodeAbstractProperty parentProperty;

        if (parent.hasDefaultPropertyName())
            parentProperty = parent.defaultNodeAbstractProperty();
        else
            parentProperty = parent.nodeAbstractProperty("data");

        parentProperty.reparentHere(node);
    }
}

QPointF getUpperLeftPosition(const QList<ModelNode> &modelNodeList)
{
    NanotraceHR::Tracer tracer{"model node operations get upper left position", category()};

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

void setUpperLeftPostionToNode(const ModelNode &layoutNode, const QList<ModelNode> &modelNodeList)
{
    NanotraceHR::Tracer tracer{"model node operations set upper left position to node", category()};

    QPointF upperLeftPosition = getUpperLeftPosition(modelNodeList);
    layoutNode.variantProperty("x").setValue(qRound(upperLeftPosition.x()));
    layoutNode.variantProperty("y") .setValue(qRound(upperLeftPosition.y()));
}

} // namespace

namespace ModelNodeOperations {

bool goIntoComponent(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node operations go into component", category()};

    return DocumentManager::goIntoComponent(modelNode);
}

void select(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations select", category()};

    if (selectionState.view())
        selectionState.view()->setSelectedModelNodes({selectionState.targetNode()});
}

void deSelect(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations deselect", category()};

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

void toFront(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations to front", category()};

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
    NanotraceHR::Tracer tracer{"model node operations to back", category()};

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

static void changeOrder(const SelectionContext &selectionState, OrderAction orderAction)
{
    NanotraceHR::Tracer tracer{"model node operations change order", category()};

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
    NanotraceHR::Tracer tracer{"model node operations raise", category()};

    changeOrder(selectionState, RaiseItem);
}

void lower(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations lower", category()};

    changeOrder(selectionState, LowerItem);
}

void setVisible(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations set visible", category()};

    if (!selectionState.view())
        return;

    try {
        selectionState.selectedModelNodes().constFirst().variantProperty("visible").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

static QSet<ModelNode> collectAncestorsAndDescendants(AbstractView *view, const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"model node operations collect ancestors and descendants", category()};

    QSet<ModelNode> keepVisible;

    ModelNode ancestor = node.parentProperty().parentModelNode();
    while (ancestor && ancestor != view->rootModelNode()) {
        keepVisible.insert(ancestor);
        if (!ancestor.hasParentProperty())
            break;
        ancestor = ancestor.parentProperty().parentModelNode();
    }

    const QList<ModelNode> descendants = node.allSubModelNodes();
    for (const ModelNode &subNode : descendants)
        keepVisible.insert(subNode);

    return keepVisible;
}

void isolateSelectedNodes(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations isolate selected nodes", category()};

    AbstractView *view = selectionState.view();
    const QList<ModelNode> selectedNodes = view->selectedModelNodes();

    if (selectedNodes.isEmpty() || view->rootModelNode().isSelected())
        return;

    const QList<ModelNode> allModelNodes = view->allModelNodes();
    ModelNode active3DScene = Utils3D::active3DSceneNode(view);
    QSet<ModelNode> nodesToKeepVisible({view->rootModelNode()});

    for (const ModelNode &node : selectedNodes) {
        nodesToKeepVisible.insert(node);
        nodesToKeepVisible.unite(collectAncestorsAndDescendants(view, node));
    }

    auto hideNode = [](const ModelNode &node) {
        QmlVisualNode(node).setVisibilityOverride(true);
    };

    auto doNotHideSubNodes = [&nodesToKeepVisible](const ModelNode &node) {
        if (node.hasAnySubModelNodes()) {
            const QList<ModelNode> allSubModelNodes = node.allSubModelNodes();
            for (const ModelNode &subNode : allSubModelNodes)
                nodesToKeepVisible.insert(subNode);
        }
    };

    const bool is3DSelection = active3DScene.isAncestorOf(selectedNodes.first());
    const QList<ModelNode> nodesToProcess = is3DSelection ? active3DScene.allSubModelNodes()
                                                          : allModelNodes;

    for (const ModelNode &node : nodesToProcess) {
        if (nodesToKeepVisible.contains(node))
            continue;

        if (!is3DSelection) {
            NodeHints hint = NodeHints::fromModelNode(node);
            if (!((node && !hint.hideInNavigator()) || hint.visibleInNavigator())
                || node.id() == Constants::MATERIAL_LIB_ID) {
                continue;
            }
        }

        doNotHideSubNodes(node); // makes sure only the top-most node in the hierarchy is hidden
        hideNode(node);
    }
}

void showAllNodes(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations show all nodes", category()};

    const QList<ModelNode> allModelNodes = selectionState.view()->allModelNodes();
    for (const ModelNode &node : allModelNodes)
        QmlVisualNode(node).setVisibilityOverride(false);
}

void setFillWidth(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations set fill width", category()};

    if (!selectionState.view() || !selectionState.hasSingleSelectedModelNode())
        return;

    try {
        selectionState.firstSelectedModelNode().variantProperty("Layout.fillWidth").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void setFillHeight(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations set fill height", category()};

    if (!selectionState.view() || !selectionState.hasSingleSelectedModelNode())
        return;

    try {
        selectionState.firstSelectedModelNode().variantProperty("Layout.fillHeight").setValue(selectionState.toggled());
    } catch (const RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void resetSize(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations reset size", category()};

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
    NanotraceHR::Tracer tracer{"model node operations reset position", category()};

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
    NanotraceHR::Tracer tracer{"model node operations go into component operation", category()};

    goIntoComponent(selectionState.currentSingleSelectedNode());
}

void resetZ(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations reset z", category()};

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
    NanotraceHR::Tracer tracer{"model node operations reverse", category()};

    if (!selectionState.view())
        return;

    selectionState.view()->executeInTransaction("DesignerActionManager|reverse", [selectionState](){
        NodeListProperty::reverseModelNodes(selectionState.selectedModelNodes());
    });
}

static void backupPropertyAndRemove(const ModelNode &node, const PropertyName &propertyName)
{
    NanotraceHR::Tracer tracer{"model node operations backup property and remove", category()};

    ModelNodeUtils::backupPropertyAndRemove(node, propertyName, auxPropertyString(propertyName));
}

static void restoreProperty(const ModelNode &node, const PropertyName &propertyName)
{
    NanotraceHR::Tracer tracer{"model node operations restore property", category()};

    ModelNodeUtils::restoreProperty(node, propertyName, auxPropertyString(propertyName));
}

void anchorsFill(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations anchors fill", category()};

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

            node.anchors().removeMargin(AnchorLineRight);
            node.anchors().removeMargin(AnchorLineLeft);
            node.anchors().removeMargin(AnchorLineTop);
            node.anchors().removeMargin(AnchorLineBottom);
        }
    });
}

void anchorsReset(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations anchors reset", category()};

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

static bool compareByX(const ModelNode &node1, const ModelNode &node2)
{
    NanotraceHR::Tracer tracer{"model node operations compare by x", category()};

    QmlItemNode itemNode1 = QmlItemNode(node1);
    QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return itemNode1.instancePosition().x() < itemNode2.instancePosition().x();
    return false;
}

static bool compareByY(const ModelNode &node1, const ModelNode &node2)
{
    NanotraceHR::Tracer tracer{"model node operations compare by y", category()};

    QmlItemNode itemNode1 = QmlItemNode(node1);
    QmlItemNode itemNode2 = QmlItemNode(node2);
    if (itemNode1.isValid() && itemNode2.isValid())
        return itemNode1.instancePosition().y() < itemNode2.instancePosition().y();
    return false;
}

static bool compareByGrid(const ModelNode &node1, const ModelNode &node2)
{
    NanotraceHR::Tracer tracer{"model node operations compare by grid", category()};

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
    NanotraceHR::Tracer tracer{"model node operations layout helper function", category()};

    if (!selectionContext.view() || !selectionContext.view()->model()->hasNodeMetaInfo(layoutType))
        return;

    if (QmlItemNode::isValidQmlItemNode(selectionContext.firstSelectedModelNode())) {
        const QmlItemNode qmlItemNode = QmlItemNode(selectionContext.firstSelectedModelNode());

        if (qmlItemNode.hasInstanceParentItem()) {

            selectionContext.view()->executeInTransaction("DesignerActionManager|layoutHelperFunction",[=](){

                QmlItemNode parentNode = qmlItemNode.instanceParentItem();
                const ModelNode layoutNode = selectionContext.view()->createModelNode(layoutType);

                reparentTo(layoutNode, parentNode);
                layoutNode.ensureIdExists();

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
    NanotraceHR::Tracer tracer{"model node operations layout row positioner", category()};

    layoutHelperFunction(selectionContext, "Row", compareByX);
}

void layoutColumnPositioner(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations layout column positioner", category()};

    layoutHelperFunction(selectionContext, "Column", compareByY);
}

void layoutGridPositioner(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations layout grid positioner", category()};

    layoutHelperFunction(selectionContext, "Grid", compareByGrid);
}

void layoutFlowPositioner(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations layout flow positioner", category()};

    layoutHelperFunction(selectionContext, "Flow", compareByGrid);
}

void layoutRowLayout(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations layout row layout", category()};

    try {
        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        layoutHelperFunction(selectionContext, "RowLayout", compareByX);
    } catch (RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void layoutColumnLayout(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations layout column layout", category()};

    try {
        LayoutInGridLayout::ensureLayoutImport(selectionContext);
        layoutHelperFunction(selectionContext, "ColumnLayout", compareByY);
    } catch (RewritingException &e) { //better safe than sorry
        e.showException();
    }
}

void layoutGridLayout(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations layout grid layout", category()};

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
    NanotraceHR::Tracer tracer{"model node operations sorted property name list", category()};

    auto propertyNames = Utils::transform<PropertyNameList>(properties, &PropertyMetaInfo::name);

    std::sort(propertyNames.begin(), propertyNames.end());

    propertyNames.erase(std::unique(propertyNames.begin(), propertyNames.end()), propertyNames.end());

    return propertyNames;
}

static QString toUpper(const QString &signal)
{
    NanotraceHR::Tracer tracer{"model node operations to upper", category()};

    QString ret = signal;
    ret[0] = signal.at(0).toUpper();
    return ret;
}

static void addSignal(const QString &typeName,
                      const QString &itemId,
                      const QString &signalName,
                      bool isRootModelNode,
                      ExternalDependenciesInterface &externanDependencies,
                      [[maybe_unused]] Model *otherModel)
{
    NanotraceHR::Tracer tracer{"model node operations add signal", category()};

    auto model = otherModel->createModel({"Item"});

    RewriterView rewriterView(externanDependencies,
                              otherModel->projectStorageDependencies().modulesStorage,
                              RewriterView::Amend);

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
        if (modelNode.documentTypeRepresentation() == typeName.toUtf8()) {
            modelNode.signalHandlerProperty(signalHandlerName).setSource(QLatin1String("{\n}"));
        }
    }
}

static QStringList cleanSignalNames(const QStringList &input)
{
    NanotraceHR::Tracer tracer{"model node operations clean signal names", category()};

    QStringList output;

    for (const QString &signal : input)
        if (!signal.startsWith(QLatin1String("__")) && !output.contains(signal))
            output.append(signal);

    return output;
}

static QStringList getSortedSignalNameList(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node operations get sorted signal name list", category()};

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
    NanotraceHR::Tracer tracer{"model node operations add signal handler or goto implementation",
                               category()};

    ModelNode modelNode;
    if (selectionState.singleNodeIsSelected())
        modelNode = selectionState.selectedModelNodes().constFirst();

    bool isModelNodeRoot = true;

    QmlObjectNode qmlObjectNode(modelNode);

    if (!qmlObjectNode.isValid()) {
        QString title = Tr::tr("Go to Implementation");
        QString description = Tr::tr("Invalid component.");
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
    const QString fileName = currentDesignDocument.toUrlishString();
    const QString typeName = currentDesignDocument.baseName();

    QStringList signalNames = cleanSignalNames(getSortedSignalNameList(selectionState.selectedModelNodes().constFirst()));

    QList<QmlJSEditor::FindReferences::Usage> usages
        = QmlJSEditor::FindReferences::findUsageOfType(currentDesignDocument, typeName);

    if (usages.isEmpty()) {
        QString title = Tr::tr("Go to Implementation");
        QString description = Tr::tr("Cannot find an implementation.");
        Core::AsynchronousMessageBox::warning(title, description);
        return;
    }

    usages = FindImplementation::run(usages.constFirst().path.toUrlishString(), typeName, itemId);

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
                              selectionState.view()->externalDependencies(),
                              selectionState.view()->model());
                });

                addSignal(typeName,
                          itemId,
                          dialog->signal(),
                          isModelNodeRoot,
                          selectionState.view()->externalDependencies(),
                          selectionState.view()->model());

                //Move cursor to correct curser position
                const QString filePath = Core::EditorManager::currentDocument()->filePath().toUrlishString();
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
    NanotraceHR::Tracer tracer{"model node operations remove layout", category()};

    if (!selectionContext.view() || !selectionContext.hasSingleSelectedModelNode())
        return;

    ModelNode layout = selectionContext.currentSingleSelectedNode();

    if (!QmlItemNode::isValidQmlItemNode(layout))
        return;

    QmlItemNode layoutItem(layout);

    QmlItemNode parent = layoutItem.instanceParentItem();

    if (!parent.isValid())
        return;

    auto model = selectionContext.model();

    selectionContext.view()->executeInTransaction("DesignerActionManager|removeLayout", [&]() {
        const QList<ModelNode> modelNodes = selectionContext.currentSingleSelectedNode()
                                                .directSubModelNodes();
        for (const ModelNode &modelNode : modelNodes) {
            if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                QmlItemNode qmlItem(modelNode);
                if (modelNode.metaInfo() == model->qtQuickItemMetaInfo()
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
    NanotraceHR::Tracer tracer{"model node operations remove positioner", category()};

    removeLayout(selectionContext);
}

void moveToComponent(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations move to component", category()};

    ModelNode modelNode;
    if (selectionContext.singleNodeIsSelected())
        modelNode = selectionContext.selectedModelNodes().constFirst();

    if (modelNode.isValid()) {
        QHash<ModelNode, QSet<ModelNode>> matAndTexNodes = Utils3D::allBoundMaterialsAndTextures(modelNode);
        QString newFilePath = selectionContext.view()->model()->rewriterView()->moveToComponent(modelNode);
        Utils3D::createMatLibForFile(newFilePath, matAndTexNodes, selectionContext.view());
    }
}

void extractComponent(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations extract component", category()};

    ModelNode selectedNode = selectionContext.currentSingleSelectedNode();
    AbstractView *contextView = selectionContext.view();

    // Get the path of the qml component
    Utils::FilePath filePath = Utils::FilePath::fromString(ModelUtils::componentFilePath(selectedNode));
    if (filePath.isEmpty()) {
        qWarning() << "Qml file for component " << selectedNode.displayName() << "not found!";
        return;
    }

    // Store properties to reset extracted comp properties later to their original values
    QList<VariantProperty> originalProperties = selectedNode.variantProperties();

    // Read the content of the qml component
    QString componentText;
    const Result<QByteArray> res = filePath.fileContents();
    if (!res) {
        qWarning() << "Cannot open component file " << filePath;
        return;
    }
    componentText = QString::fromUtf8(*res);

    Model *model = contextView->model();
    ModulesStorage &modulesStorage = model->projectStorageDependencies().modulesStorage;

    ModelPointer inputModel = model->createModel({"Rectangle"});

    // Create ModelNodes from qml string
    // This is not including the root node by default
    QPlainTextEdit textEdit;
    QString imports;
    const QList<Import> modelImports = model->imports();
    for (const Import &import : modelImports)
        imports += "import " + import.toString(true) + QLatin1Char('\n');

    textEdit.setPlainText(imports + componentText);
    NotIndentingTextEditModifier modifier(textEdit.document());

    RewriterView rewriterView{contextView->externalDependencies(), modulesStorage};
    rewriterView.setCheckSemanticErrors(false);
    rewriterView.setPossibleImportsEnabled(false);
    rewriterView.setTextModifier(&modifier);
    inputModel->setRewriterView(&rewriterView);
    rewriterView.restoreAuxiliaryData();

    // Merge the nodes in to the current document model
    ModelPointer pasteModel = DesignDocumentView::pasteToModel(contextView->externalDependencies(),
                                                               modulesStorage);
    QTC_ASSERT(pasteModel, return);

    DesignDocumentView view{contextView->externalDependencies(), modulesStorage};
    pasteModel->attachView(&view);
    QTC_ASSERT(view.rootModelNode().isValid(), return);

    pasteModel->detachView(&view);
    contextView->model()->attachView(&view);
    ModelNode originalNode = rewriterView.rootModelNode();
    view.executeInTransaction("DesignerActionManager::extractComponent", [=, &view]() {
        // Move component's materials/textures to the main material library
        QList<ModelNode> componentNodes = originalNode.allSubModelNodesAndThisNode();
        Utils::FilePath compDir = filePath.parentDir();

        // Reset root node to its original properties
        for (VariantProperty prop : originalProperties)
            originalNode.variantProperty(prop.name()).setValue(prop.value());

        for (ModelNode &node : componentNodes) {
            // Correct node assets paths if needed
            QString sourceValue = node.variantProperty("source").value().toString();
            if (!sourceValue.isEmpty() && !sourceValue.startsWith("#")) {
                Utils::FilePath assetPath = compDir.pathAppended(sourceValue); // full asset path
                QString assetPathRelative = assetPath
                                                .relativePathFromDir(DocumentManager::currentFilePath()
                                                                         .absolutePath())
                                                .toFSPathString();
                node.variantProperty("source").setValue(QUrl(assetPathRelative));
            }

            // TODO: Move root node and its children to the main material library if root node is a material/texture
            if (node.isRootNode())
                continue;

            if (node.metaInfo().isQtQuick3DMaterial() || node.metaInfo().isQtQuick3DTexture()) {
                Utils3D::ensureMaterialLibraryNode(&view);
                ModelNode mainMaterialLib = Utils3D::materialLibraryNode(&view);

                // Create copy of the node, reparent under main mat library, and delete the original
                ModelNode matOrTexture = view.insertModel(node);
                mainMaterialLib.defaultNodeListProperty().reparentHere(matOrTexture);
                node.destroy();
            }
        }

        // Delete the extracted component's material library if present
        ModelNode componentMaterialLibrary = originalNode.view()
                                                 ->modelNodeForId(Constants::MATERIAL_LIB_ID);
        if (componentMaterialLibrary.isValid())
            componentMaterialLibrary.destroy();

        // Acquire the root of selected node
        const ModelNode rootOfSelection = selectedNode.parentProperty().parentModelNode();
        QTC_ASSERT(rootOfSelection.isValid(), return);

        ModelNode newNode = view.insertModel(originalNode);
        const QList<ModelNode> selectedNodeChildren = selectedNode.directSubModelNodes();

        for (const ModelNode &child : selectedNodeChildren)
            newNode.defaultNodeListProperty().reparentHere(child);

        rootOfSelection.defaultNodeListProperty().reparentHere(newNode);

        // Delete current selected node
        QmlDesignerPlugin::instance()->currentDesignDocument()->deleteSelected();

        // Set selection to inserted nodes
        contextView->setSelectedModelNode(newNode);
    });
}

void addNodeToContentLibrary(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations add node to content library", category()};

    ModelNode node = selectionContext.currentSingleSelectedNode();

    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("ContentLibrary");

    QmlDesignerPlugin::viewManager().view()->emitCustomNotification("add_node_to_content_lib",
                                                                    {node});
}

void goImplementation(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations go implementation", category()};

    addSignalHandlerOrGotoImplementation(selectionState, false);
}

void addNewSignalHandler(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations add new signal handler", category()};

    addSignalHandlerOrGotoImplementation(selectionState, true);
}

// Open a model's material in the material editor
void editMaterial(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations edit material", category()};

    ModelNode modelNode = selectionContext.targetNode();

    if (!modelNode.isValid())
        modelNode = selectionContext.currentSingleSelectedNode();

    QTC_ASSERT(modelNode.isValid(), return);

    AbstractView *view = selectionContext.view();

    ModelNode material;

    if (modelNode.metaInfo().isQtQuick3DMaterial()) {
        material = modelNode;
    } else {
        BindingProperty prop = modelNode.bindingProperty("materials");
        if (!prop.exists())
            return;

        if (view->hasId(prop.expression())) {
            material = view->modelNodeForId(prop.expression());
        } else {
            QList<ModelNode> materials = prop.resolveListToModelNodes();

            if (materials.size() > 0)
                material = materials.first();
        }
    }

    if (material.isValid())
        Utils3D::openNodeInPropertyEditor(material);
}

void addItemToStackedContainer(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations add item to stacked container", category()};

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

            if (!potentialTabBar.metaInfo().isQtQuickTemplatesTabBar())
                potentialTabBar = ModelNode();
        }
    }

    view->executeInTransaction("DesignerActionManager:addItemToStackedContainer", [=](){
        QmlDesigner::ModelNode itemNode = view->createModelNode("Item");

        container.defaultNodeListProperty().reparentHere(itemNode);

        if (potentialTabBar.isValid()) {// The stacked container is hooked up to a TabBar
            const int buttonIndex = potentialTabBar.directSubModelNodes().size();
            ModelNode tabButtonNode = view->createModelNode("TabButton");

            tabButtonNode.variantProperty("text").setValue(
                QString::fromLatin1("Tab %1").arg(buttonIndex));
            potentialTabBar.defaultNodeListProperty().reparentHere(tabButtonNode);

        }
    });
}

PropertyName getIndexPropertyName(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node operations get index property name", category()};

    const PropertyName propertyName = NodeHints::fromModelNode(modelNode)
                                          .indexPropertyForStackedContainer()
                                          .toUtf8();

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
    NanotraceHR::Tracer tracer{"model node operations set index property", category()};

    if (!property.exists() || property.isVariantProperty()) {
        /* Using QmlObjectNode ensures we take states into account. */
        QmlObjectNode{property.parentModelNode()}.setVariantProperty(property.name(), value);
        return;
    } else if (property.isBindingProperty()) {
        /* Track one binding to the original source, incase a TabBar is attached */
        const AbstractProperty orignalProperty = property.toBindingProperty().resolveToProperty();
        if (orignalProperty.isValid() && (orignalProperty.isVariantProperty() || !orignalProperty.exists())) {
            QmlObjectNode{orignalProperty.parentModelNode()}.setVariantProperty(orignalProperty.name(),
                                                                                value);
            return;
        }
    }

    const QString propertyName = QString::fromUtf8(property.name());

    QString title = Tr::tr("Cannot Set Property %1").arg(propertyName);
    QString description = Tr::tr("The property %1 is bound to an expression.").arg(propertyName);
    Core::AsynchronousMessageBox::warning(title, description);
}

static std::optional<int> getIndexProperty(const AbstractProperty &property)
{
    NanotraceHR::Tracer tracer{"model node operations get index property", category()};

    if (property.isBindingProperty()) {
        const AbstractProperty resolvedProperty = property.toBindingProperty().resolveToProperty();
        if (resolvedProperty.isValid() && resolvedProperty.isVariantProperty()) {
            const auto variantProperty = resolvedProperty.toVariantProperty();
            if (!variantProperty.isValid())
                return std::nullopt;

            auto variant = variantProperty.value();
            if (!variant.isValid())
                return std::nullopt;

            bool ok = false;
            int value = variant.toInt(&ok);
            if (!ok)
                return std::nullopt;

            return value;
        }
    } else {
        QmlItemNode itemNode(property.parentModelNode());
        if (!itemNode.isValid())
            return std::nullopt;

        QVariant modelValue(itemNode.modelValue(property.name()));
        if (!modelValue.isValid())
            return std::nullopt;

        bool ok = false;
        int value = modelValue.toInt(&ok);
        if (!ok)
            return std::nullopt;

        return value;
    }

    return std::nullopt;
}

void increaseIndexOfStackedContainer(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations increase index of stacked container",
                               category()};

    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    const PropertyName propertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(propertyName), return);

    std::optional<int> value = getIndexProperty(container.property(propertyName));
    QTC_ASSERT(value, return);

    ++*value;

    const int maxValue = container.directSubModelNodes().size();

    QTC_ASSERT(value < maxValue, return);

    setIndexProperty(container.property(propertyName), *value);
}

void decreaseIndexOfStackedContainer(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations decrease index of stacked container",
                               category()};

    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    const PropertyName propertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(propertyName), return);

    std::optional<int> value = getIndexProperty(container.property(propertyName));
    QTC_ASSERT(value, return);

    --*value;

    QTC_ASSERT(value > -1, return);

    setIndexProperty(container.property(propertyName), *value);
}

void addTabBarToStackedContainer(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations add tab bar to stacked container", category()};

    AbstractView *view = selectionContext.view();

    QTC_ASSERT(view && selectionContext.hasSingleSelectedModelNode(), return);
    ModelNode container = selectionContext.currentSingleSelectedNode();
    QTC_ASSERT(container.isValid(), return);
    QTC_ASSERT(container.metaInfo().isValid(), return);

    QmlItemNode containerItemNode(container);
    QTC_ASSERT(containerItemNode.isValid(), return);

    const PropertyName indexPropertyName = getIndexPropertyName(container);
    QTC_ASSERT(container.metaInfo().hasProperty(indexPropertyName), return);

    view->executeInTransaction("DesignerActionManager:addItemToStackedContainer", [&]() {
        ModelNode tabBarNode = view->createModelNode("TabBar");

        container.parentProperty().reparentHere(tabBarNode);

        const int maxValue = container.directSubModelNodes().size();

        QmlItemNode tabBarItem(tabBarNode);

        tabBarItem.anchors().setAnchor(AnchorLineLeft, containerItemNode, AnchorLineLeft);
        tabBarItem.anchors().setAnchor(AnchorLineRight, containerItemNode, AnchorLineRight);
        tabBarItem.anchors().setAnchor(AnchorLineBottom, containerItemNode, AnchorLineTop);

        for (int i = 0; i < maxValue; ++i) {
            ModelNode tabButtonNode = view->createModelNode("TabButton");

            tabButtonNode.variantProperty("text").setValue(QString::fromLatin1("Tab %1").arg(i));
            tabBarNode.defaultNodeListProperty().reparentHere(tabButtonNode);
        }

        const QString id = tabBarNode.validId();

        container.removeProperty(indexPropertyName);
        const QString expression = id + "." + QString::fromLatin1(indexPropertyName);
        container.bindingProperty(indexPropertyName).setExpression(expression);
    });
}

struct CopyData
{
    FilePath source;
    FilePath target;
};

class CopyAssetsDispatcher
{
public:
    CopyAssetsDispatcher(const QList<CopyData> &copyList)
        : m_tempDir(DocumentManager::currentProjectDirPath().parentDir().toFSPathString())
    {
        NanotraceHR::Tracer tracer{"copy assets dispatcher constructor", category()};

        FilePath projectDir = DocumentManager::currentProjectDirPath();
        FilePath tempDirPath = FilePath::fromString(m_tempDir.path());

        for (const CopyData &copyData : std::as_const(copyList)) {
            if (copyData.target.exists())
                m_tasks.append(DeleteTask(copyData.target));
        }

        QList<RenameTask> renames;
        // Move it to a temp directory to prevent lots of calls on FileSystemWatcher
        for (const CopyData &copyData : std::as_const(copyList)) {
            const QString relPath = copyData.target.relativePathFromDir(projectDir).toFSPathString();
            const FilePath tempFile = tempDirPath.pathAppended(relPath);
            m_tasks.append(CopyTask{{copyData.source, tempFile}, this});
            renames.append(RenameTask{{tempFile, copyData.target}, this});
            m_totalSize += 2 * copyData.source.fileSize(); // Copy and Rename
        }

        for (const RenameTask &renameData : std::as_const(renames))
            m_tasks.append(renameData);
    }

    void cancel()
    {
        NanotraceHR::Tracer tracer{"copy assets dispatcher cancel", category()};

        m_canceled = true;
    }

    bool jobCanceled() const
    {
        NanotraceHR::Tracer tracer{"copy assets dispatcher job canceled", category()};

        return m_canceled;
    }

    FilePaths doneFiles() const
    {
        NanotraceHR::Tracer tracer{"copy assets dispatcher done files", category()};

        return FilePaths(m_doneFiles.begin(), m_doneFiles.end());
    }

    static void execute(std::invocable<int> auto &&onProgressChanged,
                        std::invocable auto &&onDone,
                        std::unique_ptr<CopyAssetsDispatcher> &&dispatcher)
    {
        NanotraceHR::Tracer tracer{"copy assets dispatcher execute", category()};

        CopyAssetsDispatcher *dispatcherPtr = dispatcher.get();

        if (dispatcherPtr->m_thread)
            return;

        dispatcherPtr->m_thread.reset(QThread::create([onProgressChanged = std::move(onProgressChanged),
                                                       onDone = std::move(onDone),
                                                       dispatcher = std::move(dispatcher)] {
            for (Task &task : dispatcher->m_tasks) {
                if (dispatcher->jobCanceled())
                    break;
                std::visit([](auto &task) { task.execute(); }, task);
                if (dispatcher->updateProgress())
                    onProgressChanged(dispatcher->m_progress);
            }
            onDone();
        }));

        dispatcherPtr->m_thread->start();
    }

    int progress()
    {
        NanotraceHR::Tracer tracer{"copy assets dispatcher progress", category()};

        return m_progress;
    }

private:
    // returns true if is updated
    bool updateProgress()
    {
        int newProgress = m_totalSize ? std::ceil(100. * m_copiedSize / m_totalSize) : 0;
        if (newProgress != m_progress) {
            m_progress = newProgress;
            return true;
        }
        return false;
    }

    struct DeleteTask
    {
        FilePath file;

        void execute() { file.removeFile(); }
    };

    struct CopyTask
    {
        CopyData data;
        CopyAssetsDispatcher *p;

        void execute()
        {
            if (auto dir = data.target.parentDir(); !dir.exists())
                dir.createDir();

            data.source.copyFile(data.target);

            p->m_copiedSize += data.target.fileSize();
        }
    };

    struct RenameTask
    {
        CopyData data;
        CopyAssetsDispatcher *p;

        void execute()
        {
            if (auto dir = data.target.parentDir(); !dir.exists())
                dir.createDir();

            data.source.renameFile(data.target);
            p->m_doneFiles.append(data.target);

            p->m_copiedSize += data.target.fileSize();
        }
    };

    using Task = std::variant<DeleteTask, CopyTask, RenameTask>;
    friend CopyTask;
    friend RenameTask;

private: // variables
    qint64 m_totalSize = 0;
    qint64 m_copiedSize = 0;
    int m_progress = 0;
    QTemporaryDir m_tempDir;
    QVarLengthArray<Task, 100> m_tasks;
    QVarLengthArray<FilePath, 100> m_doneFiles;
    std::atomic_bool m_canceled;
    Utils::UniqueObjectLatePtr<QThread> m_thread;
};

AddFilesResult addFilesToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    NanotraceHR::Tracer tracer{"model node operations add files to project", category()};

    QString directory = showDialog ? AddImagesDialog::getDirectory(fileNames, defaultDir) : defaultDir;
    if (directory.isEmpty())
        return AddFilesResult::cancelled(directory);

    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    QTC_ASSERT(document, return AddFilesResult::failed(directory));

    QList<CopyData> copyList;

    bool yesToAll = false;
    bool noToAll = false;

    for (const QString &fileName : fileNames) {
        const QString targetFile = directory + "/" + QFileInfo(fileName).fileName();
        Utils::FilePath srcFilePath = Utils::FilePath::fromString(fileName);
        Utils::FilePath targetFilePath = Utils::FilePath::fromString(targetFile);
        if (targetFilePath.exists()) {
            if (noToAll)
                continue;
            if (srcFilePath.lastModified() == targetFilePath.lastModified())
                continue;

            int overwriteFile;

            if (yesToAll) {
                overwriteFile = QMessageBox::Yes;
            } else {
                const QString title = Tr::tr("Overwrite Existing File?");
                const QString question = Tr::tr("File already exists. Overwrite?\n\"%1\"").arg(targetFile);
                overwriteFile = QMessageBox::question(Core::ICore::dialogParent(),
                                                      title,
                                                      question,
                                                      QMessageBox::Yes | QMessageBox::No
                                                          | QMessageBox::YesAll
                                                          | QMessageBox::NoToAll);
            }

            if (overwriteFile == QMessageBox::No)
                continue;

            if (overwriteFile == QMessageBox::NoToAll) {
                noToAll = true;
                continue;
            }

            if (overwriteFile == QMessageBox::YesToAll)
                yesToAll = true;
        }
        copyList.append({FilePath::fromString(fileName), FilePath::fromString(targetFile)});
    }

    const auto updateFiles = [document](const FilePaths &files) {
        ProjectExplorer::Node *node = ProjectExplorer::ProjectTree::nodeForFile(document->fileName());
        if (node) {
            ProjectExplorer::FolderNode *containingFolder = node->parentFolderNode();
            if (containingFolder)
                containingFolder->addFiles(files);
        }
    };

    QProgressDialog *progressDialog = Utils::createProgressDialog(100,
                                                                  Tr::tr("Asset import progress"),
                                                                  Tr::tr("Copying Assets"));
    progressDialog->setAutoReset(false);

    std::unique_ptr<CopyAssetsDispatcher> dispatcherPtr = std::make_unique<CopyAssetsDispatcher>(
        copyList);
    CopyAssetsDispatcher *dispatcher = dispatcherPtr.get();
    QObject *delayedObject = new QObject;

    QObject::connect(progressDialog, &QProgressDialog::canceled, [dispatcher] {
        dispatcher->cancel();
    });

    auto onProgressChanged = [progressDialog](int v) {
        QMetaObject::invokeMethod(progressDialog, &QProgressDialog::setValue, v);
    };
    auto onDone = [delayedObject, directory, progressDialog, dispatcher, updateFiles] {
        if (!dispatcher->jobCanceled())
            delayedObject->setProperty(AddFilesResult::directoryPropName, directory);
        QMetaObject::invokeMethod(progressDialog, &QWidget::setEnabled, false);
        QMetaObject::invokeMethod(Core::ICore::instance(), updateFiles, dispatcher->doneFiles());
        progressDialog->deleteLater();
        delayedObject->deleteLater();
    };

    CopyAssetsDispatcher::execute(onProgressChanged, onDone, std::move(dispatcherPtr));

    return AddFilesResult::delayed(delayedObject);
}

static QString getAssetDefaultDirectory(const QString &assetDir, const QString &defaultDirectory)
{
    NanotraceHR::Tracer tracer{"model node operations get asset default directory", category()};

    QString adjustedDefaultDirectory = defaultDirectory;

    Utils::FilePath contentPath = QmlDesignerPlugin::instance()->documentManager().currentResourcePath();

    Utils::FilePath assetPath = contentPath.pathAppended(assetDir);

    if (!assetPath.exists())
        assetPath.createDir();

    if (assetPath.exists() && assetPath.isDir())
        adjustedDefaultDirectory = assetPath.toUrlishString();

    return adjustedDefaultDirectory;
}

AddFilesResult addFontToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    NanotraceHR::Tracer tracer{"model node operations add font to project", category()};

    const AddFilesResult result = addFilesToProject(fileNames,
                                                    getAssetDefaultDirectory("fonts", defaultDir),
                                                    showDialog);
    QmlDesignerPlugin::viewManager().view()->resetPuppet();
    return result;
}

AddFilesResult addSoundToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    NanotraceHR::Tracer tracer{"model node operations add sound to project", category()};

    return addFilesToProject(fileNames, getAssetDefaultDirectory("sounds", defaultDir), showDialog);
}

AddFilesResult addShaderToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    NanotraceHR::Tracer tracer{"model node operations add shader to project", category()};

    return addFilesToProject(fileNames, getAssetDefaultDirectory("shaders", defaultDir), showDialog);
}

AddFilesResult addImageToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    NanotraceHR::Tracer tracer{"model node operations add image to project", category()};

    return addFilesToProject(fileNames, getAssetDefaultDirectory("images", defaultDir), showDialog);
}

AddFilesResult addVideoToProject(const QStringList &fileNames, const QString &defaultDir, bool showDialog)
{
    NanotraceHR::Tracer tracer{"model node operations add video to project", category()};

    return addFilesToProject(fileNames, getAssetDefaultDirectory("videos", defaultDir), showDialog);
}

static bool hasStudioComponentsImport(const SelectionContext &context)
{
    NanotraceHR::Tracer tracer{"model node operations has studio components import", category()};

    if (context.view() && context.view()->model()) {
        Import import = Import::createLibraryImport("QtQuick.Studio.Components", "1.0");
        return context.view()->model()->hasImport(import, true, true);
    }

    return false;
}

static void setAdjustedPos(const QmlDesigner::ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node operations set adjusted pos", category()};

    if (modelNode.hasParentProperty()) {
        ModelNode parentNode = modelNode.parentProperty().parentModelNode();

        const QPointF instancePos = QmlItemNode(modelNode).instancePosition();
        const int x = instancePos.x() - parentNode.variantProperty("x").value().toInt();
        const int y = instancePos.y() - parentNode.variantProperty("y").value().toInt();

        modelNode.variantProperty("x").setValue(x);
        modelNode.variantProperty("y").setValue(y);
    }
}

static void reparentToNodeAndAdjustPosition(const ModelNode &parentModelNode,
                                            const QList<ModelNode> &modelNodeList)
{
    NanotraceHR::Tracer tracer{"model node operations reparent to node and adjust position",
                               category()};

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
    NanotraceHR::Tracer tracer{"model node operations add to group item", category()};

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
                selectionContext.view()
                    ->executeInTransaction("DesignerActionManager|addToGroupItem1", [&]() {
                        QmlItemNode parentNode = qmlItemNode.instanceParentItem();
                        groupNode = selectionContext.view()->createModelNode("GroupItem");

                        reparentTo(groupNode, parentNode);
                    });
                selectionContext.view()
                    ->executeInTransaction("DesignerActionManager|addToGroupItem2", [&]() {
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

static QString baseDirectory(const QUrl &url)
{
    NanotraceHR::Tracer tracer{"model node operations base directory", category()};

    QString filePath = url.toLocalFile();
    return QFileInfo(filePath).absoluteDir().path();
}

static QString fromCamelCase(const QString &s)
{
    NanotraceHR::Tracer tracer{"model node operations from camel case", category()};

    static QRegularExpression regExp1{"(.)([A-Z][a-z]+)"};
    static QRegularExpression regExp2 {"([a-z0-9])([A-Z])"};

    QString result = s;
    result.replace(regExp1, "\\1 \\2");
    result.replace(regExp2, "\\1 \\2");

    return result;
}

static QString getTemplateDialog(const Utils::FilePath &projectPath)
{
    NanotraceHR::Tracer tracer{"model node operations get template dialog", category()};

    const Utils::FilePath templatesPath = projectPath.pathAppended("templates");

    const QStringList templateFiles = QDir(templatesPath.toUrlishString()).entryList({"*.qml"});

    QStringList names;

    for (const QString &name : templateFiles) {
        QString cleanS = name;
        cleanS.remove(".qml");
        names.append(fromCamelCase(cleanS));
    }

    QDialog *dialog = new QDialog(Core::ICore::dialogParent());
    dialog->setMinimumWidth(480);
    dialog->setModal(true);

    dialog->setWindowTitle(Tr::tr("Merge With Template"));

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

    QPushButton *browseButton = new QPushButton(Utils::PathChooser::browseButtonLabel(), dialog);

    mainLayout->addWidget(new QLabel(Tr::tr("Template:")), 0, 0);
    mainLayout->addWidget(comboBox, 1, 0, 1, 3);
    mainLayout->addWidget(browseButton, 1, 3, 1 , 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox, 2, 2, 1, 2);

    QObject::connect(browseButton, &QPushButton::clicked, dialog, [setTemplate, &projectPath]() {
        const QString newFile = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
                                                             Tr::tr("Browse Template"),
                                                             projectPath.toUrlishString(),
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
        result = templatesPath.pathAppended(result).toUrlishString();
    }

    return result;
}

void mergeWithTemplate(const SelectionContext &selectionContext,
                       ExternalDependenciesInterface &externalDependencies,
                       ModulesStorage &modulesStorage)
{
    NanotraceHR::Tracer tracer{"model node operations merge with template", category()};

    const Utils::FilePath projectPath = Utils::FilePath::fromString(
        baseDirectory(selectionContext.view()->model()->fileUrl()));

    const QString templateFile = getTemplateDialog(projectPath);

    if (QFileInfo::exists(templateFile)) {
        StylesheetMerger::styleMerge(Utils::FilePath::fromString(templateFile),
                                     selectionContext.view()->model(),
                                     modulesStorage,
                                     externalDependencies);
    }
}

void removeGroup(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations remove group", category()};

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
    NanotraceHR::Tracer tracer{"model node operations edit annotation", category()};

    ModelNode selectedNode = selectionContext.currentSingleSelectedNode();

    ModelNodeEditorProxy::fromModelNode<AnnotationEditor>(selectedNode);
}

void addMouseAreaFill(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations add mouse area fill", category()};

    if (!selectionContext.isValid()) {
        return;
    }

    if (!selectionContext.singleNodeIsSelected()) {
        return;
    }

    selectionContext.view()
        ->executeInTransaction("DesignerActionManager|addMouseAreaFill", [selectionContext]() {
            ModelNode modelNode = selectionContext.currentSingleSelectedNode();
            if (modelNode.isValid()) {
                QmlDesigner::ModelNode mouseAreaNode = selectionContext.view()->createModelNode(
                    "MouseArea");

                mouseAreaNode.ensureIdExists();

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
    NanotraceHR::Tracer tracer{"model node operations preview image data for generic node",
                               category()};

    if (auto model = modelNode.model()) {
        if (auto view = model->nodeInstanceView())
            return static_cast<const NodeInstanceView *>(view)->previewImageDataForGenericNode(modelNode,
                                                                                               {});
    }
    return {};
}

QVariant previewImageDataForImageNode(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node operations preview image data for image node", category()};

    if (auto model = modelNode.model()) {
        if (auto view = model->nodeInstanceView())
            return static_cast<const NodeInstanceView *>(view)->previewImageDataForImageNode(modelNode);
    }
    return {};
}

void openSignalDialog(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations open signal dialog", category()};

    if (!selectionContext.view() || !selectionContext.hasSingleSelectedModelNode())
        return;

    SignalList::showWidget(selectionContext.currentSingleSelectedNode());
}

void updateImported3DAsset(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations update imported 3d asset", category()};

    if (selectionContext.view()) {
        selectionContext.view()->emitCustomNotification(
                    "UpdateImported3DAsset", {selectionContext.currentSingleSelectedNode()});
    }
}

void editIn3dView(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations edit in 3d view", category()};

    if (!selectionContext.view())
        return;

    ModelNode targetNode;

    if (selectionContext.hasSingleSelectedModelNode()
        && selectionContext.currentSingleSelectedNode().metaInfo().isQtQuick3DView3D()) {
        targetNode = selectionContext.currentSingleSelectedNode();
    }

    const QPointF scenePos = selectionContext.scenePosition();
    if (!targetNode.isValid() && !scenePos.isNull()) {
        // If currently selected node is not View3D, check if there is a View3D under the cursor.
        // Assumption is that last match in allModelNodes() list is the topmost one.
        const QList<ModelNode> allNodes = selectionContext.view()->allModelNodes();
        for (int i = allNodes.size() - 1; i >= 0; --i) {
            if (SelectionContextHelpers::contains(allNodes[i], selectionContext.scenePosition())) {
                if (allNodes[i].metaInfo().isQtQuick3DView3D())
                    targetNode = allNodes[i];
                break;
            }
        }
    }

    if (targetNode.isValid()) {
        qint32 id = targetNode.internalId();
        Model *model = selectionContext.model();
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("Editor3D", true);
        if (scenePos.isNull()) {
            model->emitView3DAction(View3DActionType::AlignViewToCamera, true);
        } else {
            model->emitCustomNotification(selectionContext.view(),
                                          "pick_3d_node_from_2d_scene",
                                          {}, {scenePos, id});
        }
    }
}

Utils::FilePath findEffectFile(const ModelNode &effectNode)
{
    NanotraceHR::Tracer tracer{"model node operations find effect file", category()};

    const QString effectFile = effectNode.exportedTypeName().name.toQString() + ".qep";
    Utils::FilePath effectPath = Utils::FilePath::fromString(getEffectsDefaultDirectory()
                                                             + '/' + effectFile);
    if (!effectPath.exists()) {
        // Scan the project's content folder for a matching effect
        Utils::FilePath contentPath = QmlDesignerPlugin::instance()->documentManager().currentResourcePath();
        const Utils::FilePaths matches = contentPath.dirEntries({{effectFile}, QDir::Files,
                                                                 QDirIterator::Subdirectories});
        if (matches.isEmpty()) {
            QMessageBox msgBox;
            msgBox.setText(
                ::QmlDesigner::Tr::tr("Effect file \"%1\" not found in the project.").arg(effectFile));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            return {};
        }
        effectPath = matches[0];
    }

    return effectPath;
}

void editInEffectComposer(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node operations edit in effect composer", category()};

    if (!selectionContext.view())
        return;

    QmlItemNode targetNode;

    if (selectionContext.hasSingleSelectedModelNode()) {
        targetNode = selectionContext.currentSingleSelectedNode();
        if (!targetNode.isEffectItem())
            return;
    }

    if (targetNode.isValid()) {
        Utils::FilePath effectPath = findEffectFile(targetNode);
        if (!effectPath.isEmpty())
            openEffectComposer(effectPath.toFSPathString());
    }
}

bool isEffectComposerActivated()
{
    NanotraceHR::Tracer tracer{"model node operations is effect composer activated", category()};

    return ExtensionSystem::PluginManager::specExistsAndIsEnabled("effectcomposer");
}

void openEffectComposer(const QString &filePath)
{
    NanotraceHR::Tracer tracer{"model node operations open effect composer", category()};

    if (ModelNodeOperations::isEffectComposerActivated()) {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("EffectComposer", true);
        QmlDesignerPlugin::instance()->viewManager()
            .emitCustomNotification("open_effectcomposer_composition", {}, {filePath});
    } else {
        ModelNodeOperations::openOldEffectMaker(filePath);
    }
}

void openOldEffectMaker(const QString &filePath)
{
    NanotraceHR::Tracer tracer{"model node operations open old effect maker", category()};

    const ProjectExplorer::Kit *kit = ProjectExplorer::activeKitForCurrentProject();
    if (!kit) {
        qWarning() << __FUNCTION__ << "No project open";
        return;
    }

    Utils::FilePath effectResPath = QmlDesignerPlugin::instance()->documentManager()
                                        .generatedComponentUtils().composedEffectsBasePath()
                                        .pathAppended(QFileInfo(filePath).baseName());

    if (!effectResPath.exists())
        effectResPath.createDir();

    const QtSupport::QtVersion *baseQtVersion = QtSupport::QtKitAspect::qtVersion(kit);
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
        arguments << "--exportpath" << effectResPath.toUrlishString();

        if (env.osType() == Utils::OsTypeMac)
            env.set("QSG_RHI_BACKEND", "metal");

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
    NanotraceHR::Tracer tracer{"model node operations get effects import directory", category()};

    Utils::FilePath effectsPath = QmlDesignerPlugin::instance()
                                      ->documentManager()
                                      .generatedComponentUtils()
                                      .composedEffectsBasePath();

    if (!effectsPath.exists())
        effectsPath.createDir();

    return effectsPath;
}

QString getEffectsDefaultDirectory(const QString &defaultDir)
{
    NanotraceHR::Tracer tracer{"model node operations get effects default directory", category()};

    if (defaultDir.isEmpty()) {
        return Utils::FilePath::fromString(getAssetDefaultDirectory(
            "effects",
            QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toUrlishString())).toUrlishString();
    }

    return getAssetDefaultDirectory("effects", defaultDir);
}

QString getEffectIcon(const QString &effectPath)
{
    NanotraceHR::Tracer tracer{"model node operations get effect icon", category()};

    Utils::FilePath effectFile = QmlDesignerPlugin::instance()
                                     ->documentManager()
                                     .generatedComponentUtils()
                                     .composedEffectPath(effectPath);
    return effectFile.exists() ? QString("effectExported") : QString("effectClass");
}

bool useLayerEffect()
{
    NanotraceHR::Tracer tracer{"model node operations use layer effect", category()};

    QtcSettings *settings = Core::ICore::settings();
    const Key layerEffectEntry = "QML/Designer/UseLayerEffect";

    return settings->value(layerEffectEntry, false).toBool();
}

bool validateEffect(const QString &effectPath)
{
    NanotraceHR::Tracer tracer{"model node operations validate effect", category()};

    const QString effectName = QFileInfo(effectPath).baseName();
    Utils::FilePath effectsResDir = ModelNodeOperations::getEffectsImportDirectory();
    Utils::FilePath qmlPath = effectsResDir.resolvePath(effectName + "/" + effectName + ".qml");
    if (!qmlPath.exists()) {
        QMessageBox msgBox;
        msgBox.setText(Tr::tr("Effect %1 is not complete.").arg(effectName));
        msgBox.setInformativeText(Tr::tr("Ensure that you have saved it in the Effect Composer."
                                         "\nDo you want to edit this effect?"));
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setIcon(QMessageBox::Question);
        if (msgBox.exec() == QMessageBox::Yes)
            ModelNodeOperations::openEffectComposer(effectPath);
        return false;
    }
    return true;
}

Utils::FilePath getImagesDefaultDirectory()
{
    NanotraceHR::Tracer tracer{"model node operations get images default directory", category()};

    return Utils::FilePath::fromString(getAssetDefaultDirectory(
        "images",
        QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toUrlishString()));
}

FilePath getImported3dDefaultDirectory()
{
    NanotraceHR::Tracer tracer{"model node operations get imported 3d default directory", category()};

    return Utils::FilePath::fromString(getAssetDefaultDirectory(
        "3d",
        QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toUrlishString()));
}

void jumpToCode(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node operations jump to code", category()};

    QmlDesignerPlugin::instance()->viewManager().jumpToCodeInTextEditor(modelNode);
}

void jumpToCodeOperation(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node operations jump to code operation", category()};

    jumpToCode(selectionState.currentSingleSelectedNode());
}

static bool moveNodeToParent(const NodeAbstractProperty &targetProperty, const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"model node operations move node to parent", category()};

    NodeAbstractProperty parentProp = targetProperty.parentProperty();
    if (parentProp.isValid()) {
        ModelNode targetModel = parentProp.parentModelNode();
        parentProp.reparentHere(node);
        return true;
    }
    return false;
}

ModelNode createTextureNode(AbstractView *view, const QString &imagePath)
{
    NanotraceHR::Tracer tracer{"model node operations create texture node", category()};

    QTC_ASSERT(view, return {});

    CreateTexture textureCreator(view);
    return textureCreator.execute(imagePath, AddTextureMode::Texture);
}

bool dropAsImage3dTexture(const ModelNode &targetNode,
                          const QString &imagePath,
                          ModelNode &newNode,
                          bool &outMoveNodesAfter)
{
    NanotraceHR::Tracer tracer{"model node operations drop as image 3d texture", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    auto bindToProperty = [&](const PropertyName &propName) {
        view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
            newNode = createTextureNode(view, imagePath);
            if (newNode.isValid()) {
                BindingProperty bindProp = targetNode.bindingProperty(propName);
                bindProp.setExpression(newNode.validId());
                outMoveNodesAfter = false;
            }
        });
    };

    if (targetNode.metaInfo().isQtQuick3DDefaultMaterial()
        || targetNode.metaInfo().isQtQuick3DPrincipledMaterial()
        || targetNode.metaInfo().isQtQuick3DSpecularGlossyMaterial()) {
        // if dropping an image on a material, create a texture instead of image
        // Show texture property selection dialog
        auto dialog = ChooseFromPropertyListDialog::createIfNeeded(targetNode,
                                                                   view->model()->metaInfo("Texture"),
                                                                   Core::ICore::dialogParent());
        if (!dialog)
            return false;

        dialog->exec();

        if (dialog->result() == QDialog::Accepted) {
            view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
                newNode = createTextureNode(view, imagePath);
                if (newNode.isValid()) {
                    outMoveNodesAfter = false;
                    targetNode.bindingProperty(dialog->selectedProperty())
                        .setExpression(newNode.validId());
                }
            });
        }

        delete dialog;
        return true;
    } else if (targetNode.metaInfo().isQtQuick3DMaterial()
               || targetNode == Utils3D::materialLibraryNode(view)) {
        view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
            newNode = createTextureNode(view, imagePath);
            outMoveNodesAfter = false;
        });
        return true;
    } else if (targetNode.metaInfo().isQtQuick3DTextureInput()) {
        bindToProperty("texture");
        return newNode.isValid();
    } else if (targetNode.metaInfo().isQtQuick3DParticles3DSpriteParticle3D()) {
        bindToProperty("sprite");
        return newNode.isValid();
    } else if (targetNode.metaInfo().isQtQuick3DSceneEnvironment()) {
        bindToProperty("lightProbe");
        return newNode.isValid();
    } else if (targetNode.metaInfo().isQtQuick3DTexture()) {
        // if dropping an image on an existing texture, set the source
        targetNode.variantProperty("source").setValue(QUrl(relativePathToQmlFile(imagePath)));
        return true;
    } else if (targetNode.metaInfo().isQtQuick3DModel()) {
        const QString relImagePath = relativePathToQmlFile(imagePath);
        QTimer::singleShot(0, view, [targetNode, relImagePath, view]() {
            if (view && targetNode.isValid()) {
                // To MaterialBrowserView. Done async to avoid custom notification in transaction
                QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser");
                view->emitCustomNotification("apply_asset_to_model3D",
                                             {targetNode},
                                             {DocumentManager::currentFilePath()
                                                  .absolutePath()
                                                  .pathAppended(relImagePath)
                                                  .cleanPath()
                                                  .toUrlishString()});
            }
        });
        return true;
    }

    return false;
}

ModelNode handleItemLibraryEffectDrop(const QString &effectPath, const ModelNode &targetNode)
{
    NanotraceHR::Tracer tracer{"model node operations handle item library effect drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    ModelNode newModelNode;

    if ((targetNode.hasParentProperty() && targetNode.parentProperty().name() == "layer.effect")
        || !targetNode.metaInfo().isQtQuickItem()) {
        return newModelNode;
    }

    if (ModelNodeOperations::validateEffect(effectPath)) {
        bool layerEffect = ModelNodeOperations::useLayerEffect();
        newModelNode = QmlItemNode::createQmlItemNodeForEffect(view,
                                                               targetNode,
                                                               effectPath,
                                                               layerEffect);
    }

    return newModelNode;
}

ModelNode handleImported3dAssetDrop(const QString &assetPath, const ModelNode &targetNode,
                                    const QVector3D &position)
{
    NanotraceHR::Tracer tracer{"model node operations handle imported 3d asset drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});
    QTC_ASSERT(targetNode.isValid(), return {});

    ModelNode newModelNode;

    const GeneratedComponentUtils &compUtils = QmlDesignerPlugin::instance()->documentManager()
                                             .generatedComponentUtils();

    Utils::FilePath qmlFile = compUtils.getImported3dQml(assetPath);
    if (qmlFile.exists()) {
        TypeName qmlType = qmlFile.baseName().toUtf8();
        QString importName = compUtils.getImported3dImportName(qmlFile);
        if (!importName.isEmpty() && !qmlType.isEmpty())
            newModelNode = QmlVisualNode::createQml3DNode(view, qmlType, targetNode, importName, position);
    } else {
        QMessageBox msgBox;
        msgBox.setText(Tr::tr("Asset %1 is not complete.").arg(qmlFile.baseName()));
        msgBox.setInformativeText(Tr::tr("Please reimport the asset."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }

    return newModelNode;
}

void handleTextureDrop(const QMimeData *mimeData, const ModelNode &targetModelNode)
{
    NanotraceHR::Tracer tracer{"model node operations handle texture drop", category()};

    AbstractView *view = targetModelNode.view();
    QTC_ASSERT(view, return );

    QmlObjectNode targetNode(targetModelNode);

    if (!targetNode.isValid())
        return;

    qint32 internalId = mimeData->data(Constants::MIME_TYPE_TEXTURE).toInt();
    ModelNode texNode = view->modelNodeForInternalId(internalId);
    QTC_ASSERT(texNode.isValid(), return );

    if (targetNode.modelNode().metaInfo().isQtQuick3DModel()) {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser");
        view->emitCustomNotification("apply_texture_to_model3D", {targetNode, texNode});
    } else {
        auto *dialog = ChooseFromPropertyListDialog::createIfNeeded(targetNode,
                                                                    texNode,
                                                                    Core::ICore::dialogParent());
        if (dialog) {
            bool soloProperty = dialog->isSoloProperty();
            if (!soloProperty)
                dialog->exec();

            if (soloProperty || dialog->result() == QDialog::Accepted)
                targetNode.setBindingProperty(dialog->selectedProperty(), texNode.id());

            delete dialog;
        }
    }
}

void handleMaterialDrop(const QMimeData *mimeData, const ModelNode &targetNode)
{
    NanotraceHR::Tracer tracer{"model node operations handle material drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return );

    if (!targetNode.metaInfo().isQtQuick3DModel())
        return;

    qint32 internalId = mimeData->data(Constants::MIME_TYPE_MATERIAL).toInt();
    ModelNode matNode = view->modelNodeForInternalId(internalId);

    view->executeInTransaction(__FUNCTION__, [&] {
        Utils3D::assignMaterialTo3dModel(view, targetNode, matNode);
    });
}

ModelNode handleItemLibraryImageDrop(const QString &imagePath,
                                     NodeAbstractProperty targetProperty,
                                     const ModelNode &targetNode,
                                     bool &outMoveNodesAfter)
{
    NanotraceHR::Tracer tracer{"model node operations handle item library image drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    ModelNode newModelNode;
    Utils::FilePath origImagePath = Utils::FilePath::fromString(imagePath);
    Utils::FilePath newImagePath;

    if (!origImagePath.isChildOf(DocumentManager::currentResourcePath())) {
        AddFilesResult result = addImageToProject(
            {imagePath}, getImagesDefaultDirectory().toUrlishString(), false);

        if (result.status() == AddFilesResult::Failed) {
            Core::AsynchronousMessageBox::warning(Tr::tr("Failed to Add Image"),
                                                  Tr::tr("Could not add %1 to project.").arg(imagePath));
            return {};
        }

        newImagePath = getImagesDefaultDirectory().pathAppended(origImagePath.fileName());
    } else {
        newImagePath = origImagePath;
    }

    if (!dropAsImage3dTexture(targetNode, newImagePath.toUrlishString(), newModelNode, outMoveNodesAfter)) {
        if (targetNode.metaInfo().isBasedOn(targetNode.model()->qtQuickImageMetaInfo(),
                                            targetNode.model()->qtQuickBorderImageMetaInfo())) {

            QString relImagePath = relativePathToQmlFile(newImagePath.toFSPathString());

            targetNode.variantProperty("source").setValue(QUrl(relImagePath));
        } else {
            // create an image
            QmlItemNode newItemNode = QmlItemNode::createQmlItemNodeFromImage(view,
                                                                              newImagePath.toUrlishString(),
                                                                              QPointF(),
                                                                              targetProperty,
                                                                              false);
            if (NodeHints::fromModelNode(targetProperty.parentModelNode())
                    .canBeContainerFor(newItemNode.modelNode())) {
                newModelNode = newItemNode.modelNode();
            } else {
                newItemNode.destroy();
            }
        }
    }

    return newModelNode;
}

ModelNode handleItemLibraryFontDrop(const QString &fontFamily,
                                    NodeAbstractProperty targetProperty,
                                    const ModelNode &targetNode)
{
    NanotraceHR::Tracer tracer{"model node operations handle item library font drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    ModelNode newModelNode;

    if (targetNode.metaInfo().isQtQuickText()) {
        // if dropping into an existing Text, update font
        targetNode.variantProperty("font.family").setValue(fontFamily);
    } else {
        // create a Text node
        QmlItemNode newItemNode = QmlItemNode::createQmlItemNodeFromFont(view,
                                                                         fontFamily,
                                                                         QPointF(),
                                                                         targetProperty,
                                                                         false);
        if (NodeHints::fromModelNode(targetProperty.parentModelNode())
                .canBeContainerFor(newItemNode.modelNode())) {
            newModelNode = newItemNode.modelNode();
        } else {
            newItemNode.destroy();
        }
    }

    return newModelNode;
}

ModelNode handleItemLibraryShaderDrop(const QString &shaderPath,
                                      bool isFragShader,
                                      NodeAbstractProperty targetProperty,
                                      const ModelNode &targetNode,
                                      bool &outMoveNodesAfter)
{
    NanotraceHR::Tracer tracer{"model node operations handle item library shader drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    ModelNode newModelNode;

    const QString relPath = relativePathToQmlFile(shaderPath);

    if (targetNode.metaInfo().isQtQuick3DShader()) {
        // if dropping into an existing Shader, update
        targetNode.variantProperty("stage").setEnumeration(isFragShader ? "Shader.Fragment"
                                                                        : "Shader.Vertex");
        targetNode.variantProperty("shader").setValue(relPath);
    } else {
        view->executeInTransaction("NavigatorTreeModel::handleItemLibraryShaderDrop", [&] {
            // create a new Shader
            ItemLibraryEntry itemLibraryEntry;
            itemLibraryEntry.setName("Shader");
            itemLibraryEntry.setType("Shader");

            // set shader properties
            PropertyName prop = "shader";
            QString type = "QUrl";
            QVariant val = relPath;
            itemLibraryEntry.addProperty(prop, type, val);
            prop = "stage";
            type = "enum";
            val = isFragShader ? "Shader.Fragment" : "Shader.Vertex";
            itemLibraryEntry.addProperty(prop, type, val);

            // create a texture
            newModelNode = QmlItemNode::createQmlObjectNode(view,
                                                            itemLibraryEntry,
                                                            {},
                                                            targetProperty,
                                                            false);

            // Rename the node based on shader source
            QFileInfo fi(relPath);
            newModelNode.setIdWithoutRefactoring(
                view->model()->generateNewId(fi.baseName(), "shader"));
            // Passes can't have children, so move shader node under parent
            if (targetProperty.parentModelNode().metaInfo().isQtQuick3DPass()) {
                BindingProperty listProp = targetNode.bindingProperty("shaders");
                listProp.addModelNodeToArray(newModelNode);
                outMoveNodesAfter = !moveNodeToParent(targetProperty, newModelNode);
            }
        });
    }

    return newModelNode;
}

ModelNode handleItemLibrarySoundDrop(const QString &soundPath,
                                     NodeAbstractProperty targetProperty,
                                     const ModelNode &targetNode)
{
    NanotraceHR::Tracer tracer{"model node operations handle item library sound drop", category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    ModelNode newModelNode;

    const QString relPath = relativePathToQmlFile(soundPath);

    if (targetNode.metaInfo().isQtMultimediaSoundEffect()) {
        // if dropping into on an existing SoundEffect, update
        targetNode.variantProperty("source").setValue(QUrl(relPath));
    } else {
        // create a new SoundEffect
        ItemLibraryEntry itemLibraryEntry;
        itemLibraryEntry.setName("SoundEffect");
        itemLibraryEntry.setType("SoundEffect");

        // set source property
        PropertyName prop = "source";
        QString type = "QUrl";
        QVariant val = relPath;
        itemLibraryEntry.addProperty(prop, type, val);

        // create a texture
        newModelNode = QmlItemNode::createQmlObjectNode(view,
                                                        itemLibraryEntry,
                                                        {},
                                                        targetProperty,
                                                        false);

        // Rename the node based on source
        QFileInfo fi(relPath);
        newModelNode.setIdWithoutRefactoring(
            view->model()->generateNewId(fi.baseName(), "soundEffect"));
    }

    return newModelNode;
}

ModelNode handleItemLibraryTexture3dDrop(const QString &tex3DPath,
                                         const ModelNode &targetNode,
                                         bool &outMoveNodesAfter)
{
    NanotraceHR::Tracer tracer{"model node operations handle item library texture 3d drop",
                               category()};

    AbstractView *view = targetNode.view();
    QTC_ASSERT(view, return {});

    Import import = Import::createLibraryImport(QStringLiteral("QtQuick3D"));
    if (!view->model()->hasImport(import, true, true))
        return {};

    ModelNode newModelNode;

    dropAsImage3dTexture(targetNode, tex3DPath, newModelNode, outMoveNodesAfter);

    return newModelNode;
}

} // namespace ModelNodeOperations

} //QmlDesigner
