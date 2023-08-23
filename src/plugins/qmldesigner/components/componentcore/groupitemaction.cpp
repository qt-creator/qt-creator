// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "groupitemaction.h"

#include "nodeabstractproperty.h"
#include "nodelistproperty.h"

#include <model/modelutils.h>
#include <utils/algorithm.h>

using namespace QmlDesigner;

namespace {

bool selectionsAreSiblings(const QList<ModelNode> &selectedItems)
{
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

inline bool itemsAreGrouped(const SelectionContext &selection)
{
    return availableGroupNode(selection).isValid();
}

bool groupingEnabled(const SelectionContext &selection)
{
    if (selection.singleNodeIsSelected())
        return itemsAreGrouped(selection);
    else
        return selectionsAreSiblings(selection.selectedModelNodes());
}

void removeGroup(const ModelNode &group)
{
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
    setCheckable(true);
}

void GroupItemAction::updateContext()
{
    using namespace ComponentCoreConstants;
    ModelNodeAction::updateContext();
    action()->setText(QString::fromLatin1(action()->isChecked()
                                          ? removeGroupItemDisplayName
                                          : addToGroupItemDisplayName));
}

bool GroupItemAction::isChecked(const SelectionContext &selectionContext) const
{
    return itemsAreGrouped(selectionContext);
}
