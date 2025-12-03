// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlitemnode.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "nodeproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"

#include <abstractview.h>
#include <designeralgorithm.h>
#include <generatedcomponentutils.h>
#include <model.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/span.h>

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

bool QmlItemNode::isItemOrWindow(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node is item or window",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    auto metaInfo = modelNode.metaInfo();
    auto model = modelNode.model();
#ifdef QDS_USE_PROJECTSTORAGE

    auto qtQuickitem = model->qtQuickItemMetaInfo();
    auto qtQuickWindowWindow = model->qtQuickWindowMetaInfo();
    auto qtQuickDialogsDialog = model->qtQuickDialogsAbstractDialogMetaInfo();
    auto qtQuickControlsPopup = model->qtQuickTemplatesPopupMetaInfo();

    auto matched = metaInfo.basedOn(qtQuickitem,

                                    qtQuickWindowWindow,
                                    qtQuickDialogsDialog,
                                    qtQuickControlsPopup);

    if (!matched)
        return false;

    if (matched == qtQuickWindowWindow or matched == qtQuickDialogsDialog
        or matched == qtQuickControlsPopup)
        return modelNode.isRootNode();

    return true;
#else
    auto qtQuickitem = model->qtQuickItemMetaInfo();

    if (metaInfo.isBasedOn(model->qtQuickItemMetaInfo())) {
        return true;
    }

    if (metaInfo.isGraphicalItem() && modelNode.isRootNode())
        return true;

    return false;
#endif
}

QmlItemNode QmlItemNode::createQmlItemNode(AbstractView *view,
                                           const ItemLibraryEntry &itemLibraryEntry,
                                           const QPointF &position,
                                           QmlItemNode parentQmlItemNode,
                                           SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node",
                               category(),
                               keyValue("caller location", sl)};

    return QmlItemNode(createQmlObjectNode(view, itemLibraryEntry, position, parentQmlItemNode));
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view,
                                                    const QString &imageName,
                                                    const QPointF &position,
                                                    QmlItemNode parentQmlItemNode,
                                                    bool executeInTransaction,
                                                    SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node from image",
                               category(),
                               keyValue("caller location", sl)};

    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlItemNode(view->rootModelNode());

    NodeAbstractProperty parentProperty = parentQmlItemNode.defaultNodeAbstractProperty();

    return QmlItemNode::createQmlItemNodeFromImage(view,
                                                   imageName,
                                                   position,
                                                   parentProperty,
                                                   executeInTransaction);
}

QmlItemNode QmlItemNode::createQmlItemNodeFromImage(AbstractView *view,
                                                    const QString &imageName,
                                                    const QPointF &position,
                                                    NodeAbstractProperty parentproperty,
                                                    bool executeInTransaction,
                                                    SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node from image",
                               category(),
                               keyValue("caller location", sl)};

    QmlItemNode newQmlItemNode;

    auto doCreateQmlItemNodeFromImage = [=, &newQmlItemNode, &parentproperty]() {
        QList<QPair<PropertyName, QVariant> > propertyPairList;
        if (const int intX = qRound(position.x()))
            propertyPairList.emplace_back("x", intX);
        if (const int intY = qRound(position.y()))
            propertyPairList.emplace_back("y", intY);

        QString relativeImageName = imageName;

        //use relative path
        if (QFileInfo::exists(view->model()->fileUrl().toLocalFile())) {
            QDir fileDir(QFileInfo(view->model()->fileUrl().toLocalFile()).absolutePath());
            relativeImageName = fileDir.relativeFilePath(imageName);
            propertyPairList.emplace_back("source", relativeImageName);
        }

#ifdef QDS_USE_PROJECTSTORAGE
        TypeName type("Image");
        QImageReader reader(imageName);
        if (reader.supportsAnimation())
            type = "AnimatedImage";

        newQmlItemNode = QmlItemNode(view->createModelNode(type, propertyPairList));
#else

        TypeName type("QtQuick.Image");
        QImageReader reader(imageName);
        if (reader.supportsAnimation())
            type = "QtQuick.AnimatedImage";

        NodeMetaInfo metaInfo = view->model()->metaInfo(type, -1, -1);
        newQmlItemNode = QmlItemNode(view->createModelNode(type,
                                                           metaInfo.majorVersion(),
                                                           metaInfo.minorVersion(),
                                                           propertyPairList));
#endif
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
                                                   bool executeInTransaction,
                                                   SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node from font",
                               category(),
                               keyValue("caller location", sl)};

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
                                                   bool executeInTransaction,
                                                   SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node from font",
                               category(),
                               keyValue("caller location", sl)};

    QmlItemNode newQmlItemNode;

    auto doCreateQmlItemNodeFromFont = [=, &newQmlItemNode, &parentproperty]() {
        QList<QPair<PropertyName, QVariant>> propertyPairList;
        if (const int intX = qRound(position.x()))
            propertyPairList.emplace_back("x", intX);
        if (const int intY = qRound(position.y()))
            propertyPairList.emplace_back("y", intY);
        propertyPairList.emplace_back("font.family", fontFamily);
        propertyPairList.emplace_back("font.pointSize", 20);
        propertyPairList.emplace_back("text", fontFamily);
#ifdef QDS_USE_PROJECTSTORAGE
        newQmlItemNode = QmlItemNode(view->createModelNode("Text", propertyPairList));
#else
        NodeMetaInfo metaInfo = view->model()->metaInfo("QtQuick.Text");
        newQmlItemNode = QmlItemNode(view->createModelNode("QtQuick.Text", metaInfo.majorVersion(),
                                                           metaInfo.minorVersion(), propertyPairList));
#endif
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
                                                    bool isLayerEffect,
                                                    SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node for effect",
                               category(),
                               keyValue("caller location", sl)};

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
                                                    bool isLayerEffect,
                                                    SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node create qml item node for effect",
                               category(),
                               keyValue("caller location", sl)};

    QmlItemNode newQmlItemNode;

    auto createEffectNode = [=, &newQmlItemNode, &parentProperty]() {
        const QString effectName = QFileInfo(effectPath).baseName();
        Import import = Import::createLibraryImport(GeneratedComponentUtils(view->externalDependencies())
                                                            .composedEffectsTypePrefix()
                                                        + '.' + effectName, "1.0");
        try {
            if (!view->model()->hasImport(import, true, true))
                view->model()->changeImports({import}, {});
        } catch (const Exception &) {
            QTC_ASSERT(false, return);
        }

        TypeName type(effectName.toUtf8());
        ModelNode newModelNode = view->createModelNode(type, -1, -1);
        newModelNode.setIdWithoutRefactoring(view->model()->generateNewId(effectName));
        newQmlItemNode = QmlItemNode(newModelNode);

        placeEffectNode(parentProperty, newQmlItemNode, isLayerEffect);
    };

    view->executeInTransaction("QmlItemNode::createQmlItemNodeFromEffect", createEffectNode);
    return newQmlItemNode;
}

void QmlItemNode::placeEffectNode(NodeAbstractProperty &parentProperty,
                                  const QmlItemNode &effectNode,
                                  bool isLayerEffect,
                                  SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node place effect node",
                               category(),
                               keyValue("caller location", sl)};

    if (isLayerEffect && !parentProperty.isEmpty()) { // already contains a node
        ModelNode oldEffect = parentProperty.toNodeProperty().modelNode();
        QmlObjectNode(oldEffect).destroy();
    }

    if (!isLayerEffect) {
        // Delete previous effect child if one already exists
        ModelNode parentNode = parentProperty.parentModelNode();
        QList<ModelNode> children = parentNode.directSubModelNodes();
        for (ModelNode &child : children) {
            QmlItemNode qmlChild(child);
            if (qmlChild.isEffectItem())
                qmlChild.destroy();
        }
    }

    parentProperty.reparentHere(effectNode);

    if (isLayerEffect)
        parentProperty.parentModelNode().variantProperty("layer.enabled").setValue(true);

    if (effectNode.modelNode().metaInfo().hasProperty("timeRunning"))
        effectNode.modelNode().variantProperty("timeRunning").setValue(true);
}

bool QmlItemNode::isValid(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node is valid",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return isValidQmlItemNode(modelNode());
}

bool QmlItemNode::isValidQmlItemNode(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node is valid qml item node",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlObjectNode(modelNode) && modelNode.metaInfo().isValid()
           && (isItemOrWindow(modelNode));
}

QList<QmlItemNode> QmlItemNode::children(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node children",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

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

QList<QmlObjectNode> QmlItemNode::resources(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node resources",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

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

QList<QmlObjectNode> QmlItemNode::allDirectSubNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node all direct sub nodes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return toQmlObjectNodeList(modelNode().directSubModelNodes());
}

QmlAnchors QmlItemNode::anchors(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node anchors",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return QmlAnchors(*this);
}

bool QmlItemNode::hasChildren(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node has children",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().hasNodeListProperty("children"))
        return true;

    return !children().isEmpty();
}

bool QmlItemNode::hasResources(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node has resources",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().hasNodeListProperty("resources"))
        return true;

    return !resources().isEmpty();
}

bool QmlItemNode::instanceHasAnchor(AnchorLineType sourceAnchorLineType, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance has anchor",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return anchors().instanceHasAnchor(sourceAnchorLineType);
}

bool QmlItemNode::instanceHasAnchors(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance has anchors",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return anchors().instanceHasAnchors();
}

bool QmlItemNode::instanceHasShowContent(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance has show content",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().hasContent();
}

bool QmlItemNode::instanceCanReparent(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance can reparent",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return isInBaseState() && !anchors().instanceHasAnchors() && !instanceIsAnchoredBySibling();
}

bool QmlItemNode::instanceIsAnchoredBySibling(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is anchored by sibling",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().isAnchoredBySibling();
}

bool QmlItemNode::instanceIsAnchoredByChildren(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is anchored by children",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().isAnchoredByChildren();
}

bool QmlItemNode::instanceIsMovable(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is movable",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().isMovable();
}

bool QmlItemNode::instanceIsResizable(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is resizable",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().isResizable();
}

bool QmlItemNode::instanceIsInLayoutable(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is in layoutable",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().isInLayoutable();
}

bool QmlItemNode::instanceHasScaleOrRotationTransform(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance has scale or rotation transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().transform().type() > QTransform::TxTranslate;
}

static bool itemIsMovable(const ModelNode &modelNode)
{
    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    return NodeHints::fromModelNode(modelNode).isMovable();
}

static bool itemIsResizable(const ModelNode &modelNode)
{
    return NodeHints::fromModelNode(modelNode).isResizable();
}

bool QmlItemNode::modelIsMovable(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node model is movable",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return !modelNode().hasBindingProperty("x") && !modelNode().hasBindingProperty("y")
           && itemIsMovable(modelNode()) && !modelIsInLayout();
}

bool QmlItemNode::modelIsResizable(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node model is resizable",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return !modelNode().hasBindingProperty("width") && !modelNode().hasBindingProperty("height")
           && itemIsResizable(modelNode()) && !modelIsInLayout();
}

bool QmlItemNode::modelIsInLayout(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node model is in layout",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().hasParentProperty()) {
        ModelNode parentModelNode = modelNode().parentProperty().parentModelNode();
        if (QmlItemNode::isValidQmlItemNode(parentModelNode)
                && parentModelNode.metaInfo().isLayoutable())
            return true;

        return NodeHints::fromModelNode(parentModelNode).doesLayoutChildren();
    }

    return false;
}

bool QmlItemNode::hasFormEditorItem(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node has form editor item",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return NodeHints::fromModelNode(modelNode()).hasFormEditorItem();
}

QRectF QmlItemNode::instanceBoundingRect(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance bounding rect",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return QRectF(QPointF(0, 0), nodeInstance().size());
}

QRectF QmlItemNode::instanceSceneBoundingRect(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance scene bounding rect",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return QRectF(instanceScenePosition(), nodeInstance().size());
}

QRectF QmlItemNode::instancePaintedBoundingRect(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance painted bounding rect",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().boundingRect();
}

QRectF QmlItemNode::instanceContentItemBoundingRect(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance content item bounding rect",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().contentItemBoundingRect();
}

QTransform QmlItemNode::instanceTransform(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().transform();
}

QTransform QmlItemNode::instanceTransformWithContentTransform(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance transform with content transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().transform() * nodeInstance().contentTransform();
}

QTransform QmlItemNode::instanceTransformWithContentItemTransform(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance transform with content item transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().transform() * nodeInstance().contentItemTransform();
}

QTransform QmlItemNode::instanceSceneTransform(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance scene transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().sceneTransform();
}

QTransform QmlItemNode::instanceSceneContentItemTransform(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance scene content item transform",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().sceneTransform() * nodeInstance().contentItemTransform();
}

QPointF QmlItemNode::instanceScenePosition(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance scene position",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (hasInstanceParentItem())
        return instanceParentItem().instanceSceneTransform().map(nodeInstance().position());
     else if (modelNode().hasParentProperty() && QmlItemNode::isValidQmlItemNode(modelNode().parentProperty().parentModelNode()))
        return QmlItemNode(modelNode().parentProperty().parentModelNode()).instanceSceneTransform().map(nodeInstance().position());

    return {};
}

QPointF QmlItemNode::instancePosition(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance position",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().position();
}

QSizeF QmlItemNode::instanceSize(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance size",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().size();
}

int QmlItemNode::instancePenWidth(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance pen width",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().penWidth();
}

bool QmlItemNode::instanceIsRenderPixmapNull(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is render pixmap null",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().renderPixmap().isNull();
}

bool QmlItemNode::instanceIsVisible(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance is visible",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().property("visible").toBool();
}

QPixmap QmlItemNode::instanceRenderPixmap(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance render pixmap",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().renderPixmap();
}

QPixmap QmlItemNode::instanceBlurredRenderPixmap(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node instance blurred render pixmap",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

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

const QList<QmlItemNode> QmlItemNode::allDirectSubModelNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node all direct sub model nodes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return toQmlItemNodeList(modelNode().directSubModelNodes());
}

const QList<QmlItemNode> QmlItemNode::allSubModelNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node all sub model nodes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return toQmlItemNodeList(modelNode().allSubModelNodes());
}

bool QmlItemNode::hasAnySubModelNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node has any sub model nodes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().hasAnySubModelNodes();
}

void QmlItemNode::setPosition(const QPointF &position, SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node set position",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!hasBindingProperty("x") && !anchors().instanceHasAnchor(AnchorLineLeft)
        && !anchors().instanceHasAnchor(AnchorLineHorizontalCenter))
        setVariantProperty("x", qRound(position.x()));

    if (!hasBindingProperty("y")
            && !anchors().instanceHasAnchor(AnchorLineTop)
            && !anchors().instanceHasAnchor(AnchorLineVerticalCenter))
        setVariantProperty("y", qRound(position.y()));
}

void QmlItemNode::setPostionInBaseState(const QPointF &position, SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node set position in base state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    modelNode().variantProperty("x").setValue(qRound(position.x()));
    modelNode().variantProperty("y").setValue(qRound(position.y()));
}

bool QmlItemNode::isInLayout(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node is in layout",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isValid() && hasNodeParent()) {
        ModelNode parent = modelNode().parentProperty().parentModelNode();

        if (parent.isValid() && parent.metaInfo().isValid())
            return parent.metaInfo().isQtQuickLayoutsLayout();
    }

    return false;
}

bool QmlItemNode::canBereparentedTo(const ModelNode &potentialParent, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node can be reparented to",
                               category(),
                               keyValue("model node", *this),
                               keyValue("potential parent", potentialParent),
                               keyValue("caller location", sl)};

    if (!NodeHints::fromModelNode(potentialParent).canBeContainerFor(modelNode()))
        return false;
    return NodeHints::fromModelNode(modelNode()).canBeReparentedTo(potentialParent);
}

bool QmlItemNode::isInStackedContainer(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node is in stacked container",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (hasInstanceParent())
        return NodeHints::fromModelNode(instanceParent()).isStackedContainer();
    return false;
}


ModelNode QmlItemNode::rootModelNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node root model node",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (view())
        return view()->rootModelNode();
    return {};
}

bool QmlItemNode::isEffectItem(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node is effect item",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().metaInfo().hasProperty("_isEffectItem");
}

void QmlItemNode::setSize(const QSizeF &size, SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node set size",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!hasBindingProperty("width")
        && !(anchors().instanceHasAnchor(AnchorLineRight)
             && anchors().instanceHasAnchor(AnchorLineLeft)))
        setVariantProperty("width", qRound(size.width()));

    if (!hasBindingProperty("height") && !(anchors().instanceHasAnchor(AnchorLineBottom)
                                           && anchors().instanceHasAnchor(AnchorLineTop)))
        setVariantProperty("height", qRound(size.height()));
}

void QmlItemNode::setRotation(const qreal &angle, SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node set rotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!hasBindingProperty("rotation"))
        setVariantProperty("rotation", angle);
}

qreal QmlItemNode::rotation(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml item node rotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (hasProperty("rotation") && !hasBindingProperty("rotation")) {
        return modelNode().variantProperty("rotation").value().toReal();
    }

    return 0.0;
}

QVariant QmlItemNode::transformOrigin(SL sl)
{
    NanotraceHR::Tracer tracer{"qml item node transform origin",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (hasProperty("transformOrigin")) {
        return modelNode().variantProperty("transformOrigin").value();
    }

    return {};
}

} //QmlDesigner
