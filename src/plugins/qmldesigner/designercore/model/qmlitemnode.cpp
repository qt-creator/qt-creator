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

#include "qmlitemnode.h"
#include <metainfo.h>
#include "qmlchangeset.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "invalidmodelnodeexception.h"
#include "itemlibraryinfo.h"

#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "modelmerger.h"
#include "rewritingexception.h"

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>

#include <utils/qtcassert.h>

namespace QmlDesigner {

bool QmlItemNode::isItemOrWindow(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Item"))
        return true;

    if (modelNode.metaInfo().isSubclassOf("FlowView.FlowDecision"))
        return true;

    if (modelNode.metaInfo().isSubclassOf("FlowView.FlowWildcard"))
        return true;

    if (modelNode.metaInfo().isGraphicalItem()  && modelNode.isRootNode())
        return true;

    return false;
}

QmlItemNode QmlItemNode::createQmlItemNode(AbstractView *view,
                                           const ItemLibraryEntry &itemLibraryEntry,
                                           const QPointF &position,
                                           QmlItemNode parentQmlItemNode)
{
    return QmlItemNode(createQmlObjectNode(view, itemLibraryEntry, position, parentQmlItemNode));
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view, const QString &imageName, const QPointF &position, QmlItemNode parentQmlItemNode)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlItemNode(view->rootModelNode());

    NodeAbstractProperty parentProperty = parentQmlItemNode.defaultNodeAbstractProperty();

    return QmlItemNode::createQmlItemNodeFromImage(view, imageName, position, parentProperty);
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view, const QString &imageName, const QPointF &position, NodeAbstractProperty parentproperty)
{
    QmlItemNode newQmlItemNode;

    if (parentproperty.isValid() && view->model()->hasNodeMetaInfo("QtQuick.Image")) {
        view->executeInTransaction("QmlItemNode::createQmlItemNodeFromImage", [=, &newQmlItemNode, &parentproperty](){
            NodeMetaInfo metaInfo = view->model()->metaInfo("QtQuick.Image");
            QList<QPair<PropertyName, QVariant> > propertyPairList;
            propertyPairList.append({PropertyName("x"), QVariant(qRound(position.x()))});
            propertyPairList.append({PropertyName("y"), QVariant(qRound(position.y()))});

            QString relativeImageName = imageName;

            //use relative path
            if (QFileInfo::exists(view->model()->fileUrl().toLocalFile())) {
                QDir fileDir(QFileInfo(view->model()->fileUrl().toLocalFile()).absolutePath());
                relativeImageName = fileDir.relativeFilePath(imageName);
                propertyPairList.append({PropertyName("source"), QVariant(relativeImageName)});
            }

            newQmlItemNode = QmlItemNode(view->createModelNode("QtQuick.Image", metaInfo.majorVersion(), metaInfo.minorVersion(), propertyPairList));
            parentproperty.reparentHere(newQmlItemNode);

            newQmlItemNode.setId(view->generateNewId(QLatin1String("image")));

            newQmlItemNode.modelNode().variantProperty("fillMode").setEnumeration("Image.PreserveAspectFit");

            Q_ASSERT(newQmlItemNode.isValid());
        });
    }

    return newQmlItemNode;
}

bool QmlItemNode::isValid() const
{
    return isValidQmlItemNode(modelNode());
}

bool QmlItemNode::isValidQmlItemNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isValid() && (isItemOrWindow(modelNode));
}

QList<QmlItemNode> QmlItemNode::children() const
{
    QList<ModelNode> childrenList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("children"))
                childrenList.append(modelNode().nodeListProperty("children").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            foreach (const ModelNode &node, modelNode().nodeListProperty("data").toModelNodeList()) {
                if (QmlItemNode::isValidQmlItemNode(node))
                    childrenList.append(node);
            }
        }
    }

    return toQmlItemNodeList(childrenList);
}

QList<QmlObjectNode> QmlItemNode::resources() const
{
    QList<ModelNode> resourcesList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("resources"))
                resourcesList.append(modelNode().nodeListProperty("resources").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            foreach (const ModelNode &node, modelNode().nodeListProperty("data").toModelNodeList()) {
                if (!QmlItemNode::isValidQmlItemNode(node))
                    resourcesList.append(node);
            }
        }
    }

    return toQmlObjectNodeList(resourcesList);
}

QList<QmlObjectNode> QmlItemNode::allDirectSubNodes() const
{
    return toQmlObjectNodeList(modelNode().directSubModelNodes());
}

QmlAnchors QmlItemNode::anchors() const
{
    return QmlAnchors(*this);
}

bool QmlItemNode::hasChildren() const
{
    if (modelNode().hasNodeListProperty("children"))
        return true;

    return !children().isEmpty();
}

bool QmlItemNode::hasResources() const
{
    if (modelNode().hasNodeListProperty("resources"))
        return true;

    return !resources().isEmpty();
}

bool QmlItemNode::instanceHasAnchors() const
{
    return anchors().instanceHasAnchors();
}

bool QmlItemNode::instanceHasShowContent() const
{
    return nodeInstance().hasContent();
}

bool QmlItemNode::instanceCanReparent() const
{
    return QmlObjectNode::instanceCanReparent() && !anchors().instanceHasAnchors() && !instanceIsAnchoredBySibling();
}

bool QmlItemNode::instanceIsAnchoredBySibling() const
{
    return nodeInstance().isAnchoredBySibling();
}

bool QmlItemNode::instanceIsAnchoredByChildren() const
{
    return nodeInstance().isAnchoredByChildren();
}

bool QmlItemNode::instanceIsMovable() const
{
    if (modelNode().metaInfo().isValid()
            &&  (modelNode().metaInfo().isSubclassOf("FlowView.FlowDecision")
                 || modelNode().metaInfo().isSubclassOf("FlowView.FlowWildcard")
                 ))
        return true;

    return nodeInstance().isMovable();
}

bool QmlItemNode::instanceIsResizable() const
{
    return nodeInstance().isResizable();
}

bool QmlItemNode::instanceIsInLayoutable() const
{
     return nodeInstance().isInLayoutable();
}

bool QmlItemNode::instanceHasRotationTransform() const
{
    return nodeInstance().transform().type() > QTransform::TxScale;
}

bool itemIsMovable(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab"))
        return false;

    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    return NodeHints::fromModelNode(modelNode).isMovable();
}

bool itemIsResizable(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab"))
        return false;

    return NodeHints::fromModelNode(modelNode).isResizable();
}

bool QmlItemNode::modelIsMovable() const
{
    return !modelNode().hasBindingProperty("x")
            && !modelNode().hasBindingProperty("y")
            && itemIsMovable(modelNode())
            && !modelIsInLayout();
}

bool QmlItemNode::modelIsResizable() const
{
    return !modelNode().hasBindingProperty("width")
            && !modelNode().hasBindingProperty("height")
            && itemIsResizable(modelNode())
            && !modelIsInLayout();
}

bool QmlItemNode::modelIsInLayout() const
{
    if (modelNode().hasParentProperty()) {
        ModelNode parentModelNode = modelNode().parentProperty().parentModelNode();
        if (QmlItemNode::isValidQmlItemNode(parentModelNode)
                && parentModelNode.metaInfo().isLayoutable())
            return true;

        return NodeHints::fromModelNode(parentModelNode).doesLayoutChildren();
    }

    return false;
}

QRectF  QmlItemNode::instanceBoundingRect() const
{
    return QRectF(QPointF(0, 0), nodeInstance().size());
}

QRectF  QmlItemNode::instanceSceneBoundingRect() const
{
    return QRectF(instanceScenePosition(), nodeInstance().size());
}

QRectF QmlItemNode::instancePaintedBoundingRect() const
{
    return nodeInstance().boundingRect();
}

QRectF QmlItemNode::instanceContentItemBoundingRect() const
{
    return nodeInstance().contentItemBoundingRect();
}

QTransform  QmlItemNode::instanceTransform() const
{
    return nodeInstance().transform();
}

QTransform QmlItemNode::instanceTransformWithContentTransform() const
{
    return nodeInstance().transform() * nodeInstance().contentTransform();
}

QTransform QmlItemNode::instanceTransformWithContentItemTransform() const
{
    return nodeInstance().transform() * nodeInstance().contentItemTransform();
}

QTransform QmlItemNode::instanceSceneTransform() const
{
    return nodeInstance().sceneTransform();
}

QTransform QmlItemNode::instanceSceneContentItemTransform() const
{
    return nodeInstance().sceneTransform() * nodeInstance().contentItemTransform();
}

QPointF QmlItemNode::instanceScenePosition() const
{
    if (hasInstanceParentItem())
        return instanceParentItem().instanceSceneTransform().map(nodeInstance().position());
     else if (modelNode().hasParentProperty() && QmlItemNode::isValidQmlItemNode(modelNode().parentProperty().parentModelNode()))
        return QmlItemNode(modelNode().parentProperty().parentModelNode()).instanceSceneTransform().map(nodeInstance().position());

    return {};
}

QPointF QmlItemNode::instancePosition() const
{
    return nodeInstance().position();
}

QSizeF QmlItemNode::instanceSize() const
{
    return nodeInstance().size();
}

int QmlItemNode::instancePenWidth() const
{
    return nodeInstance().penWidth();
}

bool QmlItemNode::instanceIsRenderPixmapNull() const
{
    return nodeInstance().renderPixmap().isNull();
}

QPixmap QmlItemNode::instanceRenderPixmap() const
{
    return nodeInstance().renderPixmap();
}

QPixmap QmlItemNode::instanceBlurredRenderPixmap() const
{
    return nodeInstance().blurredRenderPixmap();
}

uint qHash(const QmlItemNode &node)
{
    return qHash(node.modelNode());
}

QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &qmlItemNodeList)
{
    QList<ModelNode> modelNodeList;

    foreach (const QmlItemNode &qmlItemNode, qmlItemNodeList)
        modelNodeList.append(qmlItemNode.modelNode());

    return modelNodeList;
}

QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlItemNode> qmlItemNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        if (QmlItemNode::isValidQmlItemNode(modelNode))
            qmlItemNodeList.append(modelNode);
    }

    return qmlItemNodeList;
}

QList<QmlItemNode> toQmlItemNodeListKeppInvalid(const QList<ModelNode> &modelNodeList)
{
    QList<QmlItemNode> qmlItemNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        qmlItemNodeList.append(modelNode);
    }

    return qmlItemNodeList;
}

const QList<QmlItemNode> QmlItemNode::allDirectSubModelNodes() const
{
    return toQmlItemNodeList(modelNode().directSubModelNodes());
}

const QList<QmlItemNode> QmlItemNode::allSubModelNodes() const
{
    return toQmlItemNodeList(modelNode().allSubModelNodes());
}

bool QmlItemNode::hasAnySubModelNodes() const
{
    return modelNode().hasAnySubModelNodes();
}

void QmlItemNode::setPosition(const QPointF &position)
{
    if (!hasBindingProperty("x")
            && !anchors().instanceHasAnchor(AnchorLineLeft)
            && !anchors().instanceHasAnchor(AnchorLineHorizontalCenter))
        setVariantProperty("x", qRound(position.x()));

    if (!hasBindingProperty("y")
            && !anchors().instanceHasAnchor(AnchorLineTop)
            && !anchors().instanceHasAnchor(AnchorLineVerticalCenter))
        setVariantProperty("y", qRound(position.y()));
}

void QmlItemNode::setPostionInBaseState(const QPointF &position)
{
    modelNode().variantProperty("x").setValue(qRound(position.x()));
    modelNode().variantProperty("y").setValue(qRound(position.y()));
}

void QmlItemNode::setFlowItemPosition(const QPointF &position)
{
    modelNode().setAuxiliaryData("flowX", position.x());
    modelNode().setAuxiliaryData("flowY", position.y());
}

QPointF QmlItemNode::flowPosition() const
{
    if (!isValid())
        return QPointF();

    return QPointF(modelNode().auxiliaryData("flowX").toInt(),
                   modelNode().auxiliaryData("flowY").toInt());
}

bool QmlItemNode::isInLayout() const
{
    if (isValid() && hasNodeParent()) {

        ModelNode parent = modelNode().parentProperty().parentModelNode();

        if (parent.isValid() && parent.metaInfo().isValid())
            return parent.metaInfo().isSubclassOf("QtQuick.Layouts.Layout");
    }

    return false;
}

bool QmlItemNode::canBereparentedTo(const ModelNode &potentialParent) const
{
    if (!NodeHints::fromModelNode(potentialParent).canBeContainerFor(modelNode()))
        return false;
    return NodeHints::fromModelNode(modelNode()).canBeReparentedTo(potentialParent);
}

bool QmlItemNode::isInStackedContainer() const
{
    if (hasInstanceParent())
        return NodeHints::fromModelNode(instanceParent()).isStackedContainer();
    return false;
}

bool QmlItemNode::isFlowView() const
{
    return modelNode().isValid()
            && modelNode().metaInfo().isSubclassOf("FlowView.FlowView");
}

bool QmlItemNode::isFlowItem() const
{
    return modelNode().isValid()
            && modelNode().metaInfo().isSubclassOf("FlowView.FlowItem");
}

bool QmlItemNode::isFlowActionArea() const
{
    return modelNode().isValid()
            && modelNode().metaInfo().isSubclassOf("FlowView.FlowActionArea");
}

ModelNode QmlItemNode::rootModelNode() const
{
    if (view())
        return view()->rootModelNode();
    return {};
}

void QmlItemNode::setSize(const QSizeF &size)
{
    if (!hasBindingProperty("width") && !(anchors().instanceHasAnchor(AnchorLineRight)
                                          && anchors().instanceHasAnchor(AnchorLineLeft)))
        setVariantProperty("width", qRound(size.width()));

    if (!hasBindingProperty("height") && !(anchors().instanceHasAnchor(AnchorLineBottom)
                                           && anchors().instanceHasAnchor(AnchorLineTop)))
        setVariantProperty("height", qRound(size.height()));
}

bool QmlFlowItemNode::isValid() const
{
    return isValidQmlFlowItemNode(modelNode());
}

bool QmlFlowItemNode::isValidQmlFlowItemNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isValid()
            && modelNode.metaInfo().isSubclassOf("FlowView.FlowItem");
}

QList<QmlFlowActionAreaNode> QmlFlowItemNode::flowActionAreas() const
{
    QList<QmlFlowActionAreaNode> list;
    for (const ModelNode &node : allDirectSubModelNodes())
        if (QmlFlowActionAreaNode::isValidQmlFlowActionAreaNode(node))
            list.append(node);
    return list;
}

QmlFlowViewNode QmlFlowItemNode::flowView() const
{
    if (modelNode().isValid() && modelNode().hasParentProperty())
        return modelNode().parentProperty().parentModelNode();
    return QmlFlowViewNode({});
}

bool QmlFlowActionAreaNode::isValid() const
{
     return isValidQmlFlowActionAreaNode(modelNode());
}

bool QmlFlowActionAreaNode::isValidQmlFlowActionAreaNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isValid()
           && modelNode.metaInfo().isSubclassOf("FlowView.FlowActionArea");
}

ModelNode QmlFlowActionAreaNode::targetTransition() const
{
    if (!modelNode().hasBindingProperty("target"))
        return {};

    return modelNode().bindingProperty("target").resolveToModelNode();
}

void QmlFlowActionAreaNode::assignTargetFlowItem(const QmlFlowTargetNode &flowItem)
{
     QTC_ASSERT(isValid(), return);
     QTC_ASSERT(flowItem.isValid(), return);

     QmlFlowViewNode flowView = flowItem.flowView();

     QTC_ASSERT(flowView.isValid(), return);

     QmlFlowItemNode flowParent = flowItemParent();

     QTC_ASSERT(flowParent.isValid(), return);

     destroyTarget();

     ModelNode transition = flowView.addTransition(flowParent.modelNode(),
                                                   flowItem.modelNode());

     modelNode().bindingProperty("target").setExpression(transition.validId());
}

QmlFlowItemNode QmlFlowActionAreaNode::flowItemParent() const
{
    QTC_ASSERT(modelNode().hasParentProperty(), return QmlFlowItemNode({}));
    return modelNode().parentProperty().parentModelNode();
}

void QmlFlowActionAreaNode::destroyTarget()
{
    QTC_ASSERT(isValid(), return);

    if (targetTransition().isValid()) {
        QmlObjectNode(targetTransition()).destroy();
        modelNode().removeProperty("target");
    }
}

bool QmlFlowViewNode::isValid() const
{
    return isValidQmlFlowViewNode(modelNode());
}

bool QmlFlowViewNode::isValidQmlFlowViewNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isValid()
           && modelNode.metaInfo().isSubclassOf("FlowView.FlowView");
}

QList<QmlFlowItemNode> QmlFlowViewNode::flowItems() const
{
    QList<QmlFlowItemNode> list;
    for (const ModelNode &node : allDirectSubModelNodes())
        if (QmlFlowItemNode::isValidQmlFlowItemNode(node)
                || QmlVisualNode::isFlowDecision(node)
                || QmlVisualNode::isFlowWildcard(node))
            list.append(node);

    return list;
}

ModelNode QmlFlowViewNode::addTransition(const QmlFlowTargetNode &from, const QmlFlowTargetNode &to)
{
    ModelNode transition = createTransition();

    QmlFlowTargetNode f = from;
    QmlFlowTargetNode t = to;

    if (f.isValid())
        transition.bindingProperty("from").setExpression(f.validId());
    transition.bindingProperty("to").setExpression(t.validId());

    return transition;
}

const QList<ModelNode> QmlFlowViewNode::transitions() const
{
    if (modelNode().nodeListProperty("flowTransitions").isValid())
        return modelNode().nodeListProperty("flowTransitions").toModelNodeList();

    return {};
}

const QList<ModelNode> QmlFlowViewNode::wildcards() const
{
    if (modelNode().nodeListProperty("flowWildcards").isValid())
        return modelNode().nodeListProperty("flowWildcards").toModelNodeList();

    return {};
}

const QList<ModelNode> QmlFlowViewNode::decicions() const
{
    if (modelNode().nodeListProperty("flowDecisions").isValid())
        return modelNode().nodeListProperty("flowDecisions").toModelNodeList();

    return {};
}

QList<ModelNode> QmlFlowViewNode::transitionsForTarget(const ModelNode &modelNode)
{
    return transitionsForProperty("to", modelNode);
}

QList<ModelNode> QmlFlowViewNode::transitionsForSource(const ModelNode &modelNode)
{
    return transitionsForProperty("from", modelNode);
}

void QmlFlowViewNode::removeDanglingTransitions()
{
    for (const ModelNode &transition : transitions()) {
        if (!transition.hasBindingProperty("to"))
            QmlObjectNode(transition).destroy();
    }
}

bool QmlFlowTargetNode::isValid() const
{
     return isFlowEditorTarget(modelNode());
}

void QmlFlowTargetNode::assignTargetItem(const QmlFlowTargetNode &node)
{
    if (QmlFlowActionAreaNode::isValidQmlFlowActionAreaNode(modelNode())) {
        QmlFlowActionAreaNode(modelNode()).assignTargetFlowItem(node);

    } else if (isFlowItem()) {
        flowView().addTransition(modelNode(), node);
    } else if (isFlowWildcard()) {
        destroyTargets();
        ModelNode transition = flowView().addTransition(ModelNode(),
                                                        node);
        modelNode().bindingProperty("target").setExpression(transition.validId());
    } else if (isFlowDecision()) {
        ModelNode sourceNode = modelNode();

        if (QmlVisualNode::isFlowDecision(sourceNode))
            sourceNode = findSourceForDecisionNode();

        if (sourceNode.isValid()) {
            ModelNode transition = flowView().addTransition(sourceNode,
                                                            node);
            modelNode().bindingProperty("targets").addModelNodeToArray(transition);
        }
    }
}

void QmlFlowTargetNode::destroyTargets()
{
    QTC_ASSERT(isValid(), return);

    if (targetTransition().isValid()) {
        QmlObjectNode(targetTransition()).destroy();
        modelNode().removeProperty("target");
    }

    if (hasBindingProperty("targets")) {
        for (ModelNode &node : modelNode().bindingProperty("targets").resolveToModelNodeList()) {
            QmlObjectNode(node).destroy();
        }
        modelNode().removeProperty("targets");
    }

}

ModelNode QmlFlowTargetNode::targetTransition() const
{
    if (!modelNode().hasBindingProperty("target"))
        return {};

    return modelNode().bindingProperty("target").resolveToModelNode();
}

QmlFlowViewNode QmlFlowTargetNode::flowView() const
{
    return view()->rootModelNode();
}

ModelNode QmlFlowItemNode::decisionNodeForTransition(const ModelNode &transition)
{
    ModelNode target = transition;

    if (target.isValid() && target.hasMetaInfo() && QmlVisualNode::isFlowTransition(target)) {

        ModelNode finalTarget = target.bindingProperty("to").resolveToModelNode();

        if (finalTarget.isValid() && finalTarget.hasMetaInfo() && QmlVisualNode::isFlowDecision(finalTarget)) {
            if (finalTarget.hasBindingProperty("targets")
                    && finalTarget.bindingProperty("targets").resolveToModelNodeList().contains(transition))
                return finalTarget;
        }
        QmlFlowViewNode flowView(transition.view()->rootModelNode());
        if (flowView.isValid()) {
            for (const ModelNode target : flowView.decicions()) {
                if (target.hasBindingProperty("targets")
                        && target.bindingProperty("targets").resolveToModelNodeList().contains(transition))
                    return target;
            }
        }
    }

    return {};
}

ModelNode QmlFlowTargetNode::findSourceForDecisionNode() const
{
    if (!isFlowDecision())
        return {};

    for (const ModelNode transition : flowView().transitionsForTarget(modelNode())) {
        if (transition.hasBindingProperty("from")) {
            const ModelNode source = transition.bindingProperty("from").resolveToModelNode();
            if (source.isValid()) {
                if (QmlVisualNode::isFlowDecision(source))
                    return QmlFlowTargetNode(source).findSourceForDecisionNode();
                else if (QmlItemNode(source).isFlowItem())
                    return source;
            }
        }
    }

    return {};
}

bool QmlFlowTargetNode::isFlowEditorTarget(const ModelNode &modelNode)
{
    return QmlItemNode(modelNode).isFlowItem()
            || QmlItemNode(modelNode).isFlowActionArea()
            || QmlVisualNode::isFlowDecision(modelNode)
            || QmlVisualNode::isFlowWildcard(modelNode);
}

void QmlFlowTargetNode::removeTransitions()
{
    if (!modelNode().hasId())
        return;

    for (const BindingProperty &property : BindingProperty::findAllReferencesTo(modelNode())) {
        if (property.isValid() && QmlVisualNode::isFlowTransition(property.parentModelNode()))
                QmlObjectNode(property.parentModelNode()).destroy();
    }
}

void QmlFlowViewNode::removeAllTransitions()
{
    if (!isValid())
        return;

    if (hasProperty("flowTransitions"))
        removeProperty("flowTransitions");
}

void QmlFlowViewNode::setStartFlowItem(const QmlFlowItemNode &flowItem)
{
    QTC_ASSERT(flowItem.isValid(), return);
    QmlFlowItemNode item = flowItem;

    ModelNode transition;

    for (const ModelNode &node : transitionsForSource(modelNode()))
        transition = node;
    if (!transition.isValid())
        transition = createTransition();

    transition.bindingProperty("from").setExpression(modelNode().validId());
    transition.bindingProperty("to").setExpression(item.validId());
}

ModelNode QmlFlowViewNode::createTransition()
{
    ModelNode transition = view()->createModelNode("FlowView.FlowTransition", 1, 0);
    nodeListProperty("flowTransitions").reparentHere(transition);

    return transition;
}

QList<ModelNode> QmlFlowViewNode::transitionsForProperty(const PropertyName &propertyName,
                                                         const ModelNode &modelNode)
{
    QList<ModelNode> list;
    for (const ModelNode &transition : transitions()) {
        if (transition.hasBindingProperty(propertyName)
                && transition.bindingProperty(propertyName).resolveToModelNode() == modelNode)
            list.append(transition);
    }
    return list;

}

} //QmlDesigner
