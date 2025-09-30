// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodecontextmenu_helper.h"
#include "componentcoretracing.h"

#include <bindingproperty.h>
#include <modelutils.h>
#include <modelnode.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>

#include <QFile>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

static bool itemsHaveSameParent(const QList<ModelNode> &siblingList)
{
    NanotraceHR::Tracer tracer{"model node context menu items have same parent", category()};

    if (siblingList.isEmpty())
        return false;


    const QmlItemNode &item = siblingList.constFirst();
    if (!item.isValid())
        return false;

    if (item.isRootModelNode())
        return false;

    QmlItemNode parent = item.instanceParent().toQmlItemNode();
    if (!parent.isValid())
        return false;

    for (const ModelNode &node : siblingList) {
        QmlItemNode currentItem(node);
        if (!currentItem.isValid())
            return false;
        QmlItemNode currentParent = currentItem.instanceParent().toQmlItemNode();
        if (!currentParent.isValid())
            return false;
        if (currentParent != parent)
            return false;
    }
    return true;
}

namespace SelectionContextFunctors {

bool singleSelectionItemIsAnchored(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu single selection item is anchored",
                               category()};

    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return anchored;
    }
    return false;
}

bool singleSelectionItemIsNotAnchored(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu single selection item is not anchored",
                               category()};

    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return !anchored;
    }
    return false;
}

bool singleSelectionItemHasAnchor(const SelectionContext &selectionState, AnchorLineType anchor)
{
    NanotraceHR::Tracer tracer{"model node context menu single selection item has anchor", category()};

    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool hasAnchor = itemNode.instanceHasAnchor(anchor);
        return hasAnchor;
    }
    return false;
}

bool selectionHasSameParent(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu selection has same parent", category()};

    return !selectionState.selectedModelNodes().isEmpty()
           && itemsHaveSameParent(selectionState.selectedModelNodes());
}

static bool fileComponentExists(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node context menu file component exists", category()};

    if (!modelNode.metaInfo().isFileComponentInProject()) {
        return true;
    }

    const QString fileName = ModelUtils::componentFilePath(modelNode);

    if (fileName.contains("qml/QtQuick"))
        return false;

    return QFileInfo::exists(fileName);
}

bool selectionIsEditableComponent(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu selection is editable component", category()};

    ModelNode node = selectionState.currentSingleSelectedNode();
    return node.isComponent() && !QmlItemNode(node).isEffectItem() && fileComponentExists(node);
}

bool selectionIsImported3DAsset(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file is imported 3d asset", category()};

    ModelNode node = selectionState.currentSingleSelectedNode();
    if (selectionState.view() && node.hasMetaInfo()) {
        QString fileName = ModelUtils::componentFilePath(node);

        if (fileName.isEmpty()) {
            // Node is not a file component, so we have to check if the current doc itself is
            fileName = node.model()->fileUrl().toLocalFile();
        }
        if (QmlDesignerPlugin::instance()->documentManager()
                .generatedComponentUtils().isImport3dPath(fileName)) {
            return true;
        }
    }
    return false;
}

bool always(const SelectionContext &)
{
    NanotraceHR::Tracer tracer{"model node context menu file always", category()};

    return true;
}

bool inBaseState(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file in base state", category()};

    return selectionState.isInBaseState();
}

bool isFileComponent(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node context menu file is file component", category()};

    if (selectionContext.isValid() && selectionContext.singleNodeIsSelected()) {
        ModelNode node = selectionContext.currentSingleSelectedNode();
        if (node.hasMetaInfo()) {
            NodeMetaInfo nodeInfo = node.metaInfo();
            return nodeInfo.isFileComponentInProject();
        }
    }
    return false;
}

bool singleSelection(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file single selection", category()};

    return selectionState.singleNodeIsSelected();
}

bool addMouseAreaFillCheck(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"model node context menu file add mouse area fill check", category()};

    if (selectionContext.isValid() && selectionContext.singleNodeIsSelected()) {
        ModelNode node = selectionContext.currentSingleSelectedNode();
        if (node.hasMetaInfo()) {
            NodeMetaInfo nodeInfo = node.metaInfo();
            return nodeInfo.isSuitableForMouseAreaFill();
        }
    }
    return false;
}

bool isModelOrMaterial(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file is model or material", category()};

    ModelNode node = selectionState.currentSingleSelectedNode();
    return node.metaInfo().isQtQuick3DModel() || node.metaInfo().isQtQuick3DMaterial();
}

bool enableAddToContentLib(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file enable add to content lib", category()};

    const QList<ModelNode> nodes = selectionState.selectedModelNodes();
    if (nodes.isEmpty())
        return false;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QString user2DBundlePath = compUtils.userBundlePath(compUtils.user2DBundleId()).toFSPathString();
    QString user3DBundlePath = compUtils.userBundlePath(compUtils.user3DBundleId()).toFSPathString();

    return std::all_of(nodes.cbegin(), nodes.cend(), [&](const ModelNode &node) {
        QString nodePath = ModelUtils::componentFilePath(node);

        if (nodePath.isEmpty())
            return true;

        bool isIn2DBundle = nodePath.startsWith(user2DBundlePath);
        bool isIn3DBundle = nodePath.startsWith(user3DBundlePath);

        return !isIn2DBundle && !isIn3DBundle;
    });
}

bool are3DNodes(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file are 3d nodes", category()};

    const QList<ModelNode> nodes = selectionState.selectedModelNodes();
    if (nodes.isEmpty())
        return false;

    return std::all_of(nodes.cbegin(), nodes.cend(), [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DNode();
    });
}

bool hasEditableMaterial(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file has editable material", category()};

    ModelNode node = selectionState.currentSingleSelectedNode();

    if (node.metaInfo().isQtQuick3DMaterial())
        return true;

    BindingProperty prop = node.bindingProperty("materials");

    return prop.exists() && (!prop.expression().isEmpty() || !prop.resolveListToModelNodes().empty());
}

bool selectionEnabled(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file selection enabled", category()};

    return selectionState.showSelectionTools();
}

bool selectionNotEmpty(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file selection not empty", category()};

    return !selectionState.selectedModelNodes().isEmpty();
}

bool selectionNot2D3DMix(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file selection not 2d 3d mix", category()};

    const QList<ModelNode> selectedNodes = selectionState.view()->selectedModelNodes();
    if (selectedNodes.size() <= 1)
        return true;

    ModelNode active3DScene = Utils3D::active3DSceneNode(selectionState.view());
    bool isFirstNode3D = active3DScene.isAncestorOf(selectedNodes.first());

    for (const ModelNode &node : selectedNodes) {
        if (active3DScene.isAncestorOf(node) != isFirstNode3D)
            return false;
    }

    return true;
}

bool singleSelectionNotRoot(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file single selection not root", category()};

    return selectionState.singleNodeIsSelected()
           && !selectionState.currentSingleSelectedNode().isRootNode();
}

bool singleSelectionView3D(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file single selection view 3d", category()};

    if (selectionState.singleNodeIsSelected()
        && selectionState.currentSingleSelectedNode().metaInfo().isQtQuick3DView3D()) {
        return true;
    }

    // If currently selected node is not View3D, check if there is a View3D under the cursor.
    if (!selectionState.scenePosition().isNull()) {
        // Assumption is that last match in allModelNodes() list is the topmost one.
        const QList<ModelNode> allNodes = selectionState.view()->allModelNodes();
        for (int i = allNodes.size() - 1; i >= 0; --i) {
            if (SelectionContextHelpers::contains(allNodes[i], selectionState.scenePosition()))
                return allNodes[i].metaInfo().isQtQuick3DView3D();
        }
    }

    return false;
}

bool singleSelectionEffectComposer(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file single selection effect composer",
                               category()};

    if (!Core::ICore::isQtDesignStudio())
        return false;

    if (selectionState.hasSingleSelectedModelNode()) {
        QmlItemNode targetNode = selectionState.currentSingleSelectedNode();
        return targetNode.isEffectItem();
    }
    return false;
}

bool selectionHasProperty(const SelectionContext &selectionState, const char *property)
{
    NanotraceHR::Tracer tracer{"model node context menu file selection has property",
                               category(),
                               keyValue("property", property)};

    for (const ModelNode &modelNode : selectionState.selectedModelNodes())
        if (modelNode.hasProperty(PropertyName(property)))
            return true;
    return false;
}

bool selectionHasSlot(const SelectionContext &selectionState, const char *property)
{
    NanotraceHR::Tracer tracer{"model node context menu file selection has slot",
                               category(),
                               keyValue("property", property)};

    for (const ModelNode &modelNode : selectionState.selectedModelNodes()) {
        for (const PropertyName &slotName : modelNode.metaInfo().slotNames()) {
            if (slotName == property)
                return true;
        }
    }

    return false;
}

bool singleSelectedItem(const SelectionContext &selectionState)
{
    NanotraceHR::Tracer tracer{"model node context menu file single selected item", category()};

    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    return itemNode.isValid();
}

} // namespace SelectionContextFunctors

namespace SelectionContextHelpers {

bool contains(const QmlItemNode &node, const QPointF &position)
{
    NanotraceHR::Tracer tracer{"model node context menu file contains", category()};

    return node.isValid()
           && node.instanceSceneTransform().mapRect(node.instanceBoundingRect()).contains(position);
}

} // namespace SelectionContextHelpers

void ActionTemplate::actionTriggered(bool b)
{
    NanotraceHR::Tracer tracer{"action template action triggered", category()};

    QmlDesignerPlugin::emitUsageStatisticsContextAction(QString::fromUtf8(m_id));
    m_selectionContext.setToggled(b);
    m_action(m_selectionContext);
}

ActionGroup::ActionGroup(const QString &displayName,
                         const QByteArray &menuId,
                         const QIcon &icon,
                         int priority,
                         SelectionContextPredicate enabled,
                         SelectionContextPredicate visibility)
    : AbstractActionGroup(displayName)
    , m_menuId(menuId)
    , m_priority(priority)
    , m_enabled(enabled)
    , m_visibility(visibility)
{
    NanotraceHR::Tracer tracer{"action group constructor", ComponentCoreTracing::category()};

    menu()->setIcon(icon);
}

SeparatorDesignerAction::SeparatorDesignerAction(const QByteArray &category, int priority)
    : AbstractAction()
    , m_category(category)
    , m_priority(priority)
    , m_visibility(&SelectionContextFunctors::always)
{
    NanotraceHR::Tracer tracer{"separator designer action constructor",
                               ComponentCoreTracing::category()};

    action()->setSeparator(true);
    action()->setIcon({});
}

CommandAction::CommandAction(Core::Command *command,
                             const QByteArray &category,
                             int priority,
                             const QIcon &overrideIcon)
    : m_action(overrideIcon.isNull()
                   ? command->action()
                   : Utils::ProxyAction::proxyActionWithIcon(command->action(), overrideIcon))
    , m_category(category)
    , m_priority(priority)
{
    NanotraceHR::Tracer tracer{"command action constructor", ComponentCoreTracing::category()};
}

ModelNodeContextMenuAction::ModelNodeContextMenuAction(const QByteArray &id,
                                                       const QString &description,
                                                       const QIcon &icon,
                                                       const QByteArray &category,
                                                       const QKeySequence &key,
                                                       int priority,
                                                       SelectionContextOperation selectionAction,
                                                       SelectionContextPredicate enabled,
                                                       SelectionContextPredicate visibility)
    : AbstractAction(new ActionTemplate(id, description, selectionAction))
    , m_id(id)
    , m_category(category)
    , m_priority(priority)
    , m_enabled(enabled)
    , m_visibility(visibility)
{
    NanotraceHR::Tracer tracer{"model node context menu action",
                               ComponentCoreTracing::category(),
                               keyValue("id", id),
                               keyValue("description", description),
                               keyValue("icon", icon.name()),
                               keyValue("category", category),
                               keyValue("priority", priority)};

    action()->setShortcut(key);
    action()->setIcon(icon);
}

ModelNodeAction::ModelNodeAction(const QByteArray &id,
                                 const QString &description,
                                 const QIcon &icon,
                                 const QString &tooltip,
                                 const QByteArray &category,
                                 const QKeySequence &key,
                                 int priority,
                                 SelectionContextOperation selectionAction,
                                 SelectionContextPredicate enabled)
    : ModelNodeContextMenuAction(id,
                                 description,
                                 icon,
                                 category,
                                 key,
                                 priority,
                                 selectionAction,
                                 enabled,
                                 &SelectionContextFunctors::always)
{
    NanotraceHR::Tracer tracer{"model node action",
                               ComponentCoreTracing::category(),
                               keyValue("id", id),
                               keyValue("description", description),
                               keyValue("icon", icon.name()),
                               keyValue("tooltip", tooltip),
                               keyValue("category", category),
                               keyValue("priority", priority)};

    action()->setIcon(icon);
    action()->setToolTip(tooltip);
}

ModelNodeFormEditorAction::ModelNodeFormEditorAction(const QByteArray &id,
                                                     const QString &description,
                                                     const QIcon &icon,
                                                     const QString &tooltip,
                                                     const QByteArray &category,
                                                     const QKeySequence &key,
                                                     int priority,
                                                     SelectionContextOperation selectionAction,
                                                     SelectionContextPredicate enabled,
                                                     SelectionContextPredicate visible)
    : ModelNodeContextMenuAction(
          id, description, icon, category, key, priority, selectionAction, enabled, visible)
{
    NanotraceHR::Tracer tracer{"model node form editor action",
                               ComponentCoreTracing::category(),
                               keyValue("id", id),
                               keyValue("description", description),
                               keyValue("icon", icon.name()),
                               keyValue("tooltip", tooltip),
                               keyValue("category", category),
                               keyValue("priority", priority)};

    action()->setIcon(icon);
    action()->setToolTip(tooltip);
}

// namespace SelectionContextHelpers

} //QmlDesigner
