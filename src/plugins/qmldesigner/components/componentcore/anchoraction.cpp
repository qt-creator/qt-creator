// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "anchoraction.h"
#include "nodeabstractproperty.h"

#include <qmlanchors.h>

using namespace QmlDesigner;

static void setAnchorToTheSameOnTarget(AbstractView *view,
                                       const ModelNode &objectNode,
                                       const AnchorLineType &objectAnchorType,
                                       const ModelNode &targetNode,
                                       double margin = 0)
{
    QmlItemNode node = objectNode;
    QmlItemNode dstNode = targetNode;
    if (!node.isValid() || !dstNode.isValid())
        return;

    view->executeInTransaction("QmlAnchorAction|setAnchorToTheSameOnTarget", [&] {
        int maxFlagBits = 8 * sizeof(AnchorLineType);
        for (int i = 0; i < maxFlagBits; ++i) {
            AnchorLineType requiredAnchor = AnchorLineType(0x1 << i);
            if (requiredAnchor & objectAnchorType) {
                node.anchors().setAnchor(requiredAnchor, dstNode, requiredAnchor);
                if (qFuzzyIsNull(margin))
                    node.anchors().removeMargin(requiredAnchor);
                else
                    node.anchors().setMargin(requiredAnchor, margin);
            }
        }
    });
}

static void anchorToParent(const SelectionContext &selectionState, AnchorLineType anchorType)
{
    if (!selectionState.view())
        return;

    ModelNode modelNode = selectionState.currentSingleSelectedNode();
    ModelNode parentModelNode = modelNode.parentProperty().parentModelNode();
    setAnchorToTheSameOnTarget(selectionState.view(), modelNode, anchorType, parentModelNode);
}

static void removeAnchor(const SelectionContext &selectionState,
                         const AnchorLineType &objectAnchorType,
                         double margin = 0)
{
    ModelNode srcModelNode = selectionState.currentSingleSelectedNode();
    QmlItemNode node = srcModelNode;
    AbstractView *view = srcModelNode.view();
    if (!node.isValid() || !view)
        return;

    view->executeInTransaction("QmlAnchorAction|removeAnchor", [&] {

        int maxFlagBits = 8 * sizeof(AnchorLineType);
        for (int i = 0; i < maxFlagBits; ++i) {
            AnchorLineType singleAnchor = AnchorLineType(0x1 << i);
            if (singleAnchor & objectAnchorType) {
                node.anchors().removeAnchor(singleAnchor);
                if (qFuzzyIsNull(margin))
                    node.anchors().removeMargin(singleAnchor);
                else
                    node.anchors().setMargin(singleAnchor, margin);
            }
        }
    });
}

static bool singleSelectionIsAnchoredToParentBy(const SelectionContext &selectionState,
                                                const AnchorLineType &lineType)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    QmlItemNode itemNodeParent(itemNode.modelParentItem());
    if (itemNode.isValid() && itemNodeParent.isValid()) {
        bool allSet = false;
        int maxFlagBits = 8 * sizeof(AnchorLineType);
        for (int i = 0; i < maxFlagBits; ++i) {
            AnchorLineType singleAnchor = AnchorLineType(0x1 << i);
            if (singleAnchor & lineType) {
                if (itemNode.anchors().modelAnchor(singleAnchor).qmlItemNode() == itemNodeParent)
                    allSet = true;
                else
                    return false;
            }
        }
        return allSet;
    }
    return false;
}

static void toggleParentAnchor(const SelectionContext &selectionState, AnchorLineType anchorType)
{
    if (singleSelectionIsAnchoredToParentBy(selectionState, anchorType))
        removeAnchor(selectionState, anchorType);
    else
        anchorToParent(selectionState, anchorType);
}

static SelectionContextOperation getSuitableAnchorFunction(AnchorLineType lineType)
{
    return std::bind(toggleParentAnchor, std::placeholders::_1, lineType);
}

ParentAnchorAction::ParentAnchorAction(const QByteArray &id,
                                       const QString &description,
                                       const QIcon &icon,
                                       const QString &tooltip,
                                       const QByteArray &category,
                                       const QKeySequence &key,
                                       int priority,
                                       AnchorLineType lineType)
    : ModelNodeAction(id,
                      description,
                      icon,
                      tooltip,
                      category,
                      key,
                      priority,
                      getSuitableAnchorFunction(lineType),
                      &SelectionContextFunctors::singleSelectedItem)
    , m_lineType(lineType)
{
    setCheckable(true);
}

bool ParentAnchorAction::isChecked(const SelectionContext &selectionState) const
{
    return singleSelectionIsAnchoredToParentBy(selectionState, m_lineType);
}
