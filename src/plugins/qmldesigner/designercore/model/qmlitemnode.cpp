// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlitemnode.h"
#include <metainfo.h>
#include "nodelistproperty.h"
#include "nodehints.h"
#include "nodeproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "itemlibraryinfo.h"

#include <model.h>
#include <abstractview.h>

#include <coreplugin/icore.h>

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

bool QmlItemNode::isItemOrWindow(const ModelNode &modelNode)
{
    auto metaInfo = modelNode.metaInfo();
    auto model = modelNode.model();

    if (metaInfo.isBasedOn(model->qtQuickItemMetaInfo(),
                           model->flowViewFlowDecisionMetaInfo(),
                           model->flowViewFlowWildcardMetaInfo())) {
        return true;
    }

    if (metaInfo.isGraphicalItem() && modelNode.isRootNode())
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

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view, const QString &imageName, const QPointF &position, QmlItemNode parentQmlItemNode, bool executeInTransaction)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlItemNode(view->rootModelNode());

    NodeAbstractProperty parentProperty = parentQmlItemNode.defaultNodeAbstractProperty();

    return QmlItemNode::createQmlItemNodeFromImage(view, imageName, position, parentProperty, executeInTransaction);
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view, const QString &imageName, const QPointF &position, NodeAbstractProperty parentproperty, bool executeInTransaction)
{
    QmlItemNode newQmlItemNode;

    auto doCreateQmlItemNodeFromImage = [=, &newQmlItemNode, &parentproperty]() {
        NodeMetaInfo metaInfo = view->model()->metaInfo("QtQuick.Image");
        QList<QPair<PropertyName, QVariant> > propertyPairList;
        if (const int intX = qRound(position.x()))
            propertyPairList.append({PropertyName("x"), QVariant(intX)});
        if (const int intY = qRound(position.y()))
            propertyPairList.append({PropertyName("y"), QVariant(intY)});

        QString relativeImageName = imageName;

        //use relative path
        if (QFileInfo::exists(view->model()->fileUrl().toLocalFile())) {
            QDir fileDir(QFileInfo(view->model()->fileUrl().toLocalFile()).absolutePath());
            relativeImageName = fileDir.relativeFilePath(imageName);
            propertyPairList.append({PropertyName("source"), QVariant(relativeImageName)});
        }

        TypeName type("QtQuick.Image");
        QImageReader reader(imageName);
        if (reader.supportsAnimation())
            type = "QtQuick.AnimatedImage";

        newQmlItemNode = QmlItemNode(view->createModelNode(type, metaInfo.majorVersion(), metaInfo.minorVersion(), propertyPairList));
        parentproperty.reparentHere(newQmlItemNode);

        QFileInfo fi(relativeImageName);
        newQmlItemNode.setId(view->model()->generateNewId(fi.baseName(), "image"));

        newQmlItemNode.modelNode().variantProperty("fillMode").setEnumeration("Image.PreserveAspectFit");

        Q_ASSERT(newQmlItemNode.isValid());
    };

    if (executeInTransaction)
        view->executeInTransaction("QmlItemNode::createQmlItemNodeFromImage", doCreateQmlItemNodeFromImage);
    else
        doCreateQmlItemNodeFromImage();

    return newQmlItemNode;
}

QmlItemNode QmlItemNode::createQmlItemNodeFromFont(AbstractView *view,
                                                   const QString &fontFamily,
                                                   const QPointF &position,
                                                   QmlItemNode parentQmlItemNode,
                                                   bool executeInTransaction)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlItemNode(view->rootModelNode());

    NodeAbstractProperty parentProperty = parentQmlItemNode.defaultNodeAbstractProperty();

    return QmlItemNode::createQmlItemNodeFromFont(view, fontFamily, position,
                                                  parentProperty, executeInTransaction);
}

QmlItemNode QmlItemNode::createQmlItemNodeFromFont(AbstractView *view,
                                                   const QString &fontFamily,
                                                   const QPointF &position,
                                                   NodeAbstractProperty parentproperty,
                                                   bool executeInTransaction)
{
    QmlItemNode newQmlItemNode;

    auto doCreateQmlItemNodeFromFont = [=, &newQmlItemNode, &parentproperty]() {
        NodeMetaInfo metaInfo = view->model()->metaInfo("QtQuick.Text");
        QList<QPair<PropertyName, QVariant>> propertyPairList;
        if (const int intX = qRound(position.x()))
            propertyPairList.append({PropertyName("x"), QVariant(intX)});
        if (const int intY = qRound(position.y()))
            propertyPairList.append({PropertyName("y"), QVariant(intY)});
        propertyPairList.append({PropertyName("font.family"), QVariant(fontFamily)});
        propertyPairList.append({PropertyName("font.pointSize"), 20});
        propertyPairList.append({PropertyName("text"), QVariant(fontFamily)});

        newQmlItemNode = QmlItemNode(view->createModelNode("QtQuick.Text", metaInfo.majorVersion(),
                                                           metaInfo.minorVersion(), propertyPairList));
        parentproperty.reparentHere(newQmlItemNode);

        newQmlItemNode.setId(view->model()->generateNewId("text", "text"));

        Q_ASSERT(newQmlItemNode.isValid());
    };

    if (executeInTransaction)
        view->executeInTransaction("QmlItemNode::createQmlItemNodeFromFont", doCreateQmlItemNodeFromFont);
    else
        doCreateQmlItemNodeFromFont();

    return newQmlItemNode;
}

QmlItemNode QmlItemNode::createQmlItemNodeForEffect(AbstractView *view,
                                                    QmlItemNode parentQmlItemNode,
                                                    const QString &effectPath,
                                                    bool isLayerEffect)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlItemNode(view->rootModelNode());

    NodeAbstractProperty parentProperty = isLayerEffect
            ? parentQmlItemNode.nodeAbstractProperty("layer.effect")
            : parentQmlItemNode.defaultNodeAbstractProperty();

    return createQmlItemNodeForEffect(view, parentProperty, effectPath, isLayerEffect);
}

QmlItemNode QmlItemNode::createQmlItemNodeForEffect(AbstractView *view,
                                                    NodeAbstractProperty parentProperty,
                                                    const QString &effectPath,
                                                    bool isLayerEffect)
{
    QmlItemNode newQmlItemNode;

    auto createEffectNode = [=, &newQmlItemNode, &parentProperty]() {
        const QString effectName = QFileInfo(effectPath).baseName();
        Import import = Import::createLibraryImport("Effects." + effectName, "1.0");
        try {
            if (!view->model()->hasImport(import, true, true))
                view->model()->changeImports({import}, {});
        } catch (const Exception &) {
            QTC_ASSERT(false, return);
        }

        TypeName type(effectName.toUtf8());
        newQmlItemNode = QmlItemNode(view->createModelNode(type, -1, -1));

        placeEffectNode(parentProperty, newQmlItemNode, isLayerEffect);
    };

    view->executeInTransaction("QmlItemNode::createQmlItemNodeFromEffect", createEffectNode);
    return newQmlItemNode;
}

void QmlItemNode::placeEffectNode(NodeAbstractProperty &parentProperty, const QmlItemNode &effectNode, bool isLayerEffect) {
    if (isLayerEffect && !parentProperty.isEmpty()) { // already contains a node
        ModelNode oldEffect = parentProperty.toNodeProperty().modelNode();
        QmlObjectNode(oldEffect).destroy();
    }

    parentProperty.reparentHere(effectNode);

    if (!isLayerEffect) {
        effectNode.modelNode().bindingProperty("source").setExpression("parent");
        effectNode.modelNode().bindingProperty("anchors.fill").setExpression("parent");
    } else {
        parentProperty.parentModelNode().variantProperty("layer.enabled").setValue(true);
    }

    if (effectNode.modelNode().metaInfo().hasProperty("timeRunning"))
        effectNode.modelNode().variantProperty("timeRunning").setValue(true);
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
            const QList<ModelNode> nodes = modelNode().nodeListProperty("data").toModelNodeList();
            for (const ModelNode &node : nodes) {
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
            const QList<ModelNode> nodes = modelNode().nodeListProperty("data").toModelNodeList();
            for (const ModelNode &node : nodes) {
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
    return isInBaseState() && !anchors().instanceHasAnchors() && !instanceIsAnchoredBySibling();
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
    auto metaInfo = modelNode().metaInfo();
    auto m = model();
    if (metaInfo.isBasedOn(m->flowViewFlowDecisionMetaInfo(), m->flowViewFlowWildcardMetaInfo()))
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

bool QmlItemNode::instanceHasScaleOrRotationTransform() const
{
    return nodeInstance().transform().type() > QTransform::TxTranslate;
}

bool itemIsMovable(const ModelNode &modelNode)
{
    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    return NodeHints::fromModelNode(modelNode).isMovable();
}

bool itemIsResizable(const ModelNode &modelNode)
{
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

bool QmlItemNode::hasFormEditorItem() const
{
    return NodeHints::fromModelNode(modelNode()).hasFormEditorItem();
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

QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &qmlItemNodeList)
{
    QList<ModelNode> modelNodeList;

    for (const QmlItemNode &qmlItemNode : qmlItemNodeList)
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

namespace {
constexpr AuxiliaryDataKeyView flowXProperty{AuxiliaryDataType::Document, "flowX"};
constexpr AuxiliaryDataKeyView flowYProperty{AuxiliaryDataType::Document, "flowY"};
} // namespace

void QmlItemNode::setFlowItemPosition(const QPointF &position)
{
    modelNode().setAuxiliaryData(flowXProperty, position.x());
    modelNode().setAuxiliaryData(flowYProperty, position.y());
}

QPointF QmlItemNode::flowPosition() const
{
    if (!isValid())
        return QPointF();

    return QPointF(modelNode().auxiliaryDataWithDefault(flowXProperty).toInt(),
                   modelNode().auxiliaryDataWithDefault(flowYProperty).toInt());
}

bool QmlItemNode::isInLayout() const
{
    if (isValid() && hasNodeParent()) {

        ModelNode parent = modelNode().parentProperty().parentModelNode();

        if (parent.isValid() && parent.metaInfo().isValid())
            return parent.metaInfo().isQtQuickLayoutsLayout();
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
    return modelNode().isValid() && modelNode().metaInfo().isFlowViewFlowView();
}

bool QmlItemNode::isFlowItem() const
{
    return modelNode().isValid() && modelNode().metaInfo().isFlowViewFlowItem();
}

bool QmlItemNode::isFlowActionArea() const
{
    return modelNode().isValid() && modelNode().metaInfo().isFlowViewFlowActionArea();
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

void QmlItemNode::setRotation(const qreal &angle)
{
    if (!hasBindingProperty("rotation"))
        setVariantProperty("rotation", angle);
}

qreal QmlItemNode::rotation() const
{
    if (hasProperty("rotation") && !hasBindingProperty("rotation")) {
        return modelNode().variantProperty("rotation").value().toReal();
    }

    return 0.0;
}

QVariant QmlItemNode::transformOrigin()
{
    if (hasProperty("transformOrigin")) {
        return modelNode().variantProperty("transformOrigin").value();
    }

    return {};
}

bool QmlFlowItemNode::isValid() const
{
    return isValidQmlFlowItemNode(modelNode());
}

bool QmlFlowItemNode::isValidQmlFlowItemNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isFlowViewFlowItem();
}

QList<QmlFlowActionAreaNode> QmlFlowItemNode::flowActionAreas() const
{
    QList<QmlFlowActionAreaNode> list;
    for (const ModelNode node : allDirectSubModelNodes())
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
    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isFlowViewFlowActionArea();
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
           && modelNode.metaInfo().isFlowViewFlowView();
}

QList<QmlFlowItemNode> QmlFlowViewNode::flowItems() const
{
    QList<QmlFlowItemNode> list;
    for (const ModelNode node : allDirectSubModelNodes())
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

QList<ModelNode> QmlFlowViewNode::transitions() const
{
    if (modelNode().nodeListProperty("flowTransitions").isValid())
        return modelNode().nodeListProperty("flowTransitions").toModelNodeList();

    return {};
}

QList<ModelNode> QmlFlowViewNode::wildcards() const
{
    if (modelNode().nodeListProperty("flowWildcards").isValid())
        return modelNode().nodeListProperty("flowWildcards").toModelNodeList();

    return {};
}

QList<ModelNode> QmlFlowViewNode::decicions() const
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
            for (const ModelNode &target : flowView.decicions()) {
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

    for (const ModelNode &transition : flowView().transitionsForTarget(modelNode())) {
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

PropertyNameList QmlFlowViewNode::s_mouseSignals = []() {
    PropertyNameList mouseSignals = {
        "clicked", "doubleClicked", "pressAndHold", "pressed", "released", "wheel"};

    Q_ASSERT(std::is_sorted(mouseSignals.begin(), mouseSignals.end()));

    return mouseSignals;
}();

QList<QmlConnections> QmlFlowViewNode::getAssociatedConnections(const ModelNode &node)
{
    if (!node.isValid())
        return {};

    AbstractView *view = node.view();

    return Utils::transform<QList<QmlConnections>>(Utils::filtered(view->allModelNodes(),
                                                                   [&node](const ModelNode &n) {
        const QmlConnections connection(n);
        if (!connection.isValid())
            return false;

        const QList<SignalHandlerProperty> &signalProperties = connection.signalProperties();
        for (const SignalHandlerProperty &property : signalProperties) {
            auto signalWithoutPrefix = SignalHandlerProperty::prefixRemoved(property.name());
            const QStringList sourceComponents = property.source().split(".");
            QString sourceId;
            QString sourceProperty;
            if (sourceComponents.size() > 1) {
                sourceId = sourceComponents[0];
                sourceProperty = sourceComponents[1];
            }

            if (mouseSignals().contains(signalWithoutPrefix) && sourceId == node.id()
                && sourceProperty == "trigger()")
                return true;
        }

        return false;
    }), [](const ModelNode &n) {
        return QmlConnections(n);
    });
}

} //QmlDesigner
