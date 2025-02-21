// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "anchoraction.h"
#include "nodeabstractproperty.h"

#include <modelnodeutils.h>
#include <qmlanchors.h>
#include <qmlitemnode.h>

using namespace QmlDesigner;

namespace {
const Utils::SmallString auxDataString("anchors_");

Utils::SmallString auxPropertyString(Utils::SmallStringView name)
{
    return auxDataString + name;
}

bool hasAnchor(const ModelNode &modelNode, const AnchorLineType &objectAnchorType)
{
    QmlItemNode node(modelNode);
    return node.isValid() && node.anchors().instanceHasAnchor(objectAnchorType);
}

void backupPropertyAndRemove(const ModelNode &modelNode, const PropertyName &propertyName)
{
    ModelNodeUtils::backupPropertyAndRemove(modelNode, propertyName, auxPropertyString(propertyName));
}

void backupPropertiesAndRemove(const ModelNode &modelNode, const AnchorLineType &objectAnchorType)
{
    if (objectAnchorType & AnchorLineTopBottom) {
        backupPropertyAndRemove(modelNode, "y");
        if (hasAnchor(modelNode, AnchorLineType(objectAnchorType ^ AnchorLineTopBottom)))
            backupPropertyAndRemove(modelNode, "height");
    }
    if ((objectAnchorType & AnchorLineTopBottom) == AnchorLineTopBottom) {
        backupPropertyAndRemove(modelNode, "y");
        backupPropertyAndRemove(modelNode, "height");
    }
    if (objectAnchorType & AnchorLineLeftRight) {
        backupPropertyAndRemove(modelNode, "x");
        if (hasAnchor(modelNode, AnchorLineType(objectAnchorType ^ AnchorLineLeftRight)))
            backupPropertyAndRemove(modelNode, "width");
    }

    if ((objectAnchorType & AnchorLineLeftRight) == AnchorLineLeftRight) {
        backupPropertyAndRemove(modelNode, "x");
        backupPropertyAndRemove(modelNode, "width");
    }

    if (objectAnchorType & AnchorLineVerticalCenter)
        backupPropertyAndRemove(modelNode, "y");
    if (objectAnchorType & AnchorLineHorizontalCenter)
        backupPropertyAndRemove(modelNode, "x");
}

void restoreProperty(const ModelNode &modelNode, const PropertyName &propertyName)
{
    ModelNodeUtils::restoreProperty(modelNode, propertyName, auxPropertyString(propertyName));
}

void restoreProperties(const ModelNode &modelNode, const AnchorLineType &objectAnchorType)
{
    if (objectAnchorType & AnchorLineTopBottom) {
        restoreProperty(modelNode, "height");
        if (!(hasAnchor(modelNode, AnchorLineType(objectAnchorType ^ AnchorLineTopBottom))
              || hasAnchor(modelNode, AnchorLineVerticalCenter))) {
            restoreProperty(modelNode, "y");
        }
    }
    if ((objectAnchorType & AnchorLineTopBottom) == AnchorLineTopBottom) {
        restoreProperty(modelNode, "height");
        restoreProperty(modelNode, "y");
    }
    if (objectAnchorType & AnchorLineLeftRight) {
        restoreProperty(modelNode, "width");
        if (!(hasAnchor(modelNode, AnchorLineType(objectAnchorType ^ AnchorLineLeftRight))
              || hasAnchor(modelNode, AnchorLineHorizontalCenter))) {
            restoreProperty(modelNode, "x");
        }
    }
    if ((objectAnchorType & AnchorLineLeftRight) == AnchorLineLeftRight) {
        restoreProperty(modelNode, "width");
        restoreProperty(modelNode, "x");
    }

    if (objectAnchorType & AnchorLineVerticalCenter) {
        if (!(hasAnchor(modelNode, AnchorLineTop) || hasAnchor(modelNode, AnchorLineBottom)))
            restoreProperty(modelNode, "y");
    }
    if (objectAnchorType & AnchorLineHorizontalCenter) {
        if (!(hasAnchor(modelNode, AnchorLineLeft) || hasAnchor(modelNode, AnchorLineRight)))
            restoreProperty(modelNode, "x");
    }
}
} // namespace

static void removeAnchor(const ModelNode &objectNode,
                         const AnchorLineType &objectAnchorType,
                         double margin = 0)
{
    QmlItemNode node = objectNode;
    AbstractView *view = objectNode.view();
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

        restoreProperties(node, objectAnchorType);
    });
}

static void removeAnchor(const SelectionContext &selectionState,
                         const AnchorLineType &objectAnchorType,
                         double margin = 0)
{
    removeAnchor(selectionState.currentSingleSelectedNode(), objectAnchorType, margin);
}

static void removeConflictingAnchors(const ModelNode &modelNode, const AnchorLineType &objectAnchorType)
{
    if ((objectAnchorType & AnchorLineTop) && hasAnchor(modelNode, AnchorLineBottom))
        removeAnchor(modelNode, AnchorLineVerticalCenter);
    if ((objectAnchorType & AnchorLineBottom) && hasAnchor(modelNode, AnchorLineTop))
        removeAnchor(modelNode, AnchorLineVerticalCenter);
    if ((objectAnchorType & AnchorLineTopBottom) == AnchorLineTopBottom)
        removeAnchor(modelNode, AnchorLineVerticalCenter);
    if ((objectAnchorType & AnchorLineLeft) && hasAnchor(modelNode, AnchorLineRight))
        removeAnchor(modelNode, AnchorLineHorizontalCenter);
    if ((objectAnchorType & AnchorLineRight) && hasAnchor(modelNode, AnchorLineLeft))
        removeAnchor(modelNode, AnchorLineHorizontalCenter);
    if ((objectAnchorType & AnchorLineLeftRight) == AnchorLineLeftRight)
        removeAnchor(modelNode, AnchorLineHorizontalCenter);

    if (objectAnchorType & AnchorLineVerticalCenter) {
        removeAnchor(modelNode, AnchorLineTop);
        removeAnchor(modelNode, AnchorLineBottom);
    }
    if (objectAnchorType & AnchorLineHorizontalCenter) {
        removeAnchor(modelNode, AnchorLineLeft);
        removeAnchor(modelNode, AnchorLineRight);
    }
}

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
        removeConflictingAnchors(objectNode, objectAnchorType);
        backupPropertiesAndRemove(objectNode, objectAnchorType);

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
    QmlItemNode node{modelNode};

    // centerIn support
    if ((anchorType & AnchorLineCenter)
        && (hasAnchor(modelNode, AnchorLineType(anchorType ^ AnchorLineCenter)))) {
        selectionState.view()->executeInTransaction("QmlAnchorAction|centerIn", [&] {
            backupPropertiesAndRemove(modelNode, AnchorLineCenter);
            restoreProperty(modelNode, "width");
            restoreProperty(modelNode, "height");
            node.anchors().centerIn();
            node.anchors().removeMargin(AnchorLineRight);
            node.anchors().removeMargin(AnchorLineLeft);
            node.anchors().removeMargin(AnchorLineTop);
            node.anchors().removeMargin(AnchorLineBottom);
        });
        return;
    }

    setAnchorToTheSameOnTarget(selectionState.view(), modelNode, anchorType, parentModelNode);
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
