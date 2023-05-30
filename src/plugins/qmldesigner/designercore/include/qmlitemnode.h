// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include "qmlobjectnode.h"
#include "qmlstate.h"
#include "qmlvisualnode.h"
#include "qmlconnections.h"

#include <QStringList>
#include <QRectF>
#include <QTransform>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;
class ItemLibraryEntry;

class QMLDESIGNERCORE_EXPORT QmlItemNode : public QmlVisualNode
{
    friend QmlAnchors;

public:
    QmlItemNode() = default;
    QmlItemNode(const ModelNode &modelNode)  : QmlVisualNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlItemNode(const ModelNode &modelNode);

    static bool isItemOrWindow(const ModelNode &modelNode);

    static QmlItemNode createQmlItemNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             const QPointF &position,
                                             QmlItemNode parentQmlItemNode);


    static QmlItemNode createQmlItemNodeFromImage(AbstractView *view,
                                                  const QString &imageName,
                                                  const QPointF &position,
                                                  QmlItemNode parentQmlItemNode,
                                                  bool executeInTransaction = true);
    static QmlItemNode createQmlItemNodeFromImage(AbstractView *view,
                                                  const QString &imageName,
                                                  const QPointF &position,
                                                  NodeAbstractProperty parentproperty,
                                                  bool executeInTransaction = true);

    static QmlItemNode createQmlItemNodeFromFont(AbstractView *view,
                                                 const QString &fontFamily,
                                                 const QPointF &position,
                                                 QmlItemNode parentQmlItemNode,
                                                 bool executeInTransaction = true);
    static QmlItemNode createQmlItemNodeFromFont(AbstractView *view,
                                                 const QString &fontFamily,
                                                 const QPointF &position,
                                                 NodeAbstractProperty parentproperty,
                                                 bool executeInTransaction = true);

    static QmlItemNode createQmlItemNodeForEffect(AbstractView *view,
                                                  QmlItemNode parentQmlItemNode,
                                                  const QString &effectPath,
                                                  bool isLayerEffect);
    static QmlItemNode createQmlItemNodeForEffect(AbstractView *view,
                                                  NodeAbstractProperty parentProperty,
                                                  const QString &effectPath,
                                                  bool isLayerEffect);
    static void placeEffectNode(NodeAbstractProperty &parentProperty,
                                const QmlItemNode &effectNode,
                                bool isLayerEffect);

    QList<QmlItemNode> children() const;
    QList<QmlObjectNode> resources() const;
    QList<QmlObjectNode> allDirectSubNodes() const;
    QmlAnchors anchors() const;

    bool hasChildren() const;
    bool hasResources() const;
    bool instanceHasAnchor(AnchorLineType sourceAnchorLineType) const;
    bool instanceHasAnchors() const;
    bool instanceHasShowContent() const;

    bool instanceCanReparent() const;
    bool instanceIsAnchoredBySibling() const;
    bool instanceIsAnchoredByChildren() const;
    bool instanceIsMovable() const;
    bool instanceIsResizable() const;
    bool instanceIsInLayoutable() const;
    bool instanceHasScaleOrRotationTransform() const;

    bool modelIsMovable() const;
    bool modelIsResizable() const;
    bool modelIsInLayout() const;
    bool hasFormEditorItem() const;

    QRectF instanceBoundingRect() const;
    QRectF instanceSceneBoundingRect() const;
    QRectF instancePaintedBoundingRect() const;
    QRectF instanceContentItemBoundingRect() const;
    QTransform instanceTransform() const;
    QTransform instanceTransformWithContentTransform() const;
    QTransform instanceTransformWithContentItemTransform() const;
    QTransform instanceSceneTransform() const;
    QTransform instanceSceneContentItemTransform() const;
    QPointF instanceScenePosition() const;
    QPointF instancePosition() const;
    QSizeF instanceSize() const;
    int instancePenWidth() const;
    bool instanceIsRenderPixmapNull() const;

    QPixmap instanceRenderPixmap() const;
    QPixmap instanceBlurredRenderPixmap() const;

    const QList<QmlItemNode> allDirectSubModelNodes() const;
    const QList<QmlItemNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;

    void setPosition(const QPointF &position);
    void setPostionInBaseState(const QPointF &position);
    void setFlowItemPosition(const QPointF &position);
    QPointF flowPosition() const;

    void setSize(const QSizeF &size);
    bool isInLayout() const;
    bool canBereparentedTo(const ModelNode &potentialParent) const;

    void setRotation(const qreal &angle);
    qreal rotation() const;
    QVariant transformOrigin();

    bool isInStackedContainer() const;

    bool isFlowView() const;
    bool isFlowItem() const;
    bool isFlowActionArea() const;
    ModelNode rootModelNode() const;

    friend auto qHash(const QmlItemNode &node) { return qHash(node.modelNode()); }
};

class QmlFlowItemNode;
class QmlFlowViewNode;

class QMLDESIGNERCORE_EXPORT QmlFlowTargetNode final : public QmlItemNode
{
public:
    QmlFlowTargetNode(const ModelNode &modelNode)  : QmlItemNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }

    void assignTargetItem(const QmlFlowTargetNode &node);
    void destroyTargets();
    ModelNode targetTransition() const;
    QmlFlowViewNode flowView() const;
    ModelNode findSourceForDecisionNode() const;
    static bool isFlowEditorTarget(const ModelNode &modelNode);
    void removeTransitions();
};

class QMLDESIGNERCORE_EXPORT QmlFlowActionAreaNode final : public QmlItemNode
{
public:
    QmlFlowActionAreaNode(const ModelNode &modelNode)  : QmlItemNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlFlowActionAreaNode(const ModelNode &modelNode);
    ModelNode targetTransition() const;
    void assignTargetFlowItem(const QmlFlowTargetNode &flowItem);
    QmlFlowItemNode flowItemParent() const;
    void destroyTarget();
};

class QMLDESIGNERCORE_EXPORT QmlFlowItemNode final : public QmlItemNode
{
public:
    QmlFlowItemNode(const ModelNode &modelNode)  : QmlItemNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlFlowItemNode(const ModelNode &modelNode);
    QList<QmlFlowActionAreaNode> flowActionAreas() const;
    QmlFlowViewNode flowView() const;

    static ModelNode decisionNodeForTransition(const ModelNode &transition);
};

class QMLDESIGNERCORE_EXPORT QmlFlowViewNode final : public QmlItemNode
{
public:
    QmlFlowViewNode(const ModelNode &modelNode)  : QmlItemNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlFlowViewNode(const ModelNode &modelNode);
    QList<QmlFlowItemNode> flowItems() const;
    ModelNode addTransition(const QmlFlowTargetNode &from, const QmlFlowTargetNode &to);
    QList<ModelNode> transitions() const;
    QList<ModelNode> wildcards() const;
    QList<ModelNode> decicions() const;
    QList<ModelNode> transitionsForTarget(const ModelNode &modelNode);
    QList<ModelNode> transitionsForSource(const ModelNode &modelNode);
    void removeDanglingTransitions();
    void removeAllTransitions();
    void setStartFlowItem(const QmlFlowItemNode &flowItem);
    ModelNode createTransition();

    static QList<QmlConnections> getAssociatedConnections(const ModelNode &node);
    static const PropertyNameList &mouseSignals() { return s_mouseSignals; }

protected:
    QList<ModelNode> transitionsForProperty(const PropertyName &propertyName, const ModelNode &modelNode);

private:
    static PropertyNameList s_mouseSignals;
};


QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
QMLDESIGNERCORE_EXPORT QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList);
QMLDESIGNERCORE_EXPORT QList<QmlItemNode> toQmlItemNodeListKeppInvalid(const QList<ModelNode> &modelNodeList);

} //QmlDesigner
