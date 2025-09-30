// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "groupitemaction.h"
#include "componentcoretracing.h"

#include "designermcumanager.h"
#include "modelnodeoperations.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"

#include <modelutils.h>
#include <utils/algorithm.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

namespace {

bool selectionsAreSiblings(const QList<ModelNode> &selectedItems)
{
    NanotraceHR::Tracer tracer{"group item action selections are siblings", category()};

    const QList<ModelNode> prunedSelectedItems = ModelUtils::pruneChildren(selectedItems);
    if (prunedSelectedItems.size() < 2)
        return false;

    ModelNode modelItemNode(prunedSelectedItems.first());
    if (!modelItemNode.isValid())
        return false;

    ModelNode parentNode = modelItemNode.parentProperty().parentModelNode();
    if (!parentNode.isValid())
        return false;

    for (const ModelNode &node : Utils::span(prunedSelectedItems).subspan(1)) {
        if (!node.isValid())
            return false;

        if (node.parentProperty().parentModelNode() != parentNode)
            return false;
    }

    return true;
}

ModelNode availableGroupNode(const SelectionContext &selection)
{
    NanotraceHR::Tracer tracer{"group item action available group node", category()};

    if (!selection.isValid())
        return {};

    if (selection.singleNodeIsSelected()) {
        const ModelNode node = selection.currentSingleSelectedNode();
        if (node.metaInfo().isQtQuickStudioComponentsGroupItem())
            return node;
    }

    const ModelNode parentNode = selection
            .firstSelectedModelNode()
            .parentProperty().parentModelNode();

    if (!parentNode.isValid())
        return {};

    QList<ModelNode> allSiblingNodes = parentNode.directSubModelNodes();

    QList<ModelNode> selectedNodes = ModelUtils::pruneChildren(selection.selectedModelNodes());
    if (selectedNodes.size() != allSiblingNodes.size())
        return {};

    Utils::sort(allSiblingNodes);
    Utils::sort(selectedNodes);

    if (allSiblingNodes != selectedNodes)
        return {};

    if (parentNode.metaInfo().isQtQuickStudioComponentsGroupItem())
        return parentNode;

    return {};
}

bool itemsAreGrouped(const SelectionContext &selection)
{
    NanotraceHR::Tracer tracer{"group item action items are grouped", category()};

    return availableGroupNode(selection).isValid();
}

bool groupingEnabled(const SelectionContext &selection)
{
    NanotraceHR::Tracer tracer{"group item action grouping enabled", category()};

    //StudioComponents.GroupItem is not available in Qt for MCUs
    if (DesignerMcuManager::instance().isMCUProject())
        return false;

    if (selection.singleNodeIsSelected())
        return itemsAreGrouped(selection);
    else
        return selectionsAreSiblings(selection.selectedModelNodes());
}

void removeGroup(const ModelNode &group)
{
    NanotraceHR::Tracer tracer{"group item action remove group", category()};

    QmlItemNode groupItem(group);
    QmlItemNode parent = groupItem.instanceParentItem();

    if (!groupItem || !parent)
        return;

    group.view()->executeInTransaction(
        "removeGroup", [group, &groupItem, parent]() {
            for (const ModelNode &modelNode : group.directSubModelNodes()) {
                if (QmlVisualNode qmlItem = modelNode) {
                    QPointF pos = qmlItem.position().toPointF();
                    pos = groupItem.instanceTransform().map(pos);
                    qmlItem.setPosition(pos);

                    parent.modelNode().defaultNodeListProperty().reparentHere(modelNode);
                }
            }
            groupItem.destroy();
    });
}

void toggleGrouping(const SelectionContext &selection)
{
    NanotraceHR::Tracer tracer{"group item action toggle grouping", category()};

    if (!selection.isValid())
        return;

    ModelNode groupNode = availableGroupNode(selection);

    if (groupNode.isValid())
        removeGroup(groupNode);
    else
        ModelNodeOperations::addToGroupItem(selection);
}

} // blank namespace

GroupItemAction::GroupItemAction(const QIcon &icon,
                                 const QKeySequence &key,
                                 int priority)
    : ModelNodeAction(ComponentCoreConstants::addToGroupItemCommandId,
                      {},
                      icon,
                      {},
                      {},
                      key,
                      priority,
                      &toggleGrouping,
                      &groupingEnabled)
{
    NanotraceHR::Tracer tracer{"group item action constructor", category()};

    setCheckable(true);
}

void GroupItemAction::updateContext()
{
    NanotraceHR::Tracer tracer{"group item action update context", category()};

    using namespace ComponentCoreConstants;
    ModelNodeAction::updateContext();
    action()->setText(QString::fromLatin1(action()->isChecked()
                                          ? removeGroupItemDisplayName
                                          : addToGroupItemDisplayName));
}

bool GroupItemAction::isChecked(const SelectionContext &selectionContext) const
{
    NanotraceHR::Tracer tracer{"group item action is checked", category()};

    return itemsAreGrouped(selectionContext);
}

} // namespace QmlDesigner
