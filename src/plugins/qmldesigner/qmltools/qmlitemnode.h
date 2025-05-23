// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

class QMLDESIGNER_EXPORT QmlItemNode : public QmlVisualNode
{
    friend QmlAnchors;

public:
    QmlItemNode() = default;

    QmlItemNode(const ModelNode &modelNode)
        : QmlVisualNode(modelNode)
    {}

    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlItemNode(const ModelNode &modelNode, SL sl = {});

    static bool isItemOrWindow(const ModelNode &modelNode, SL sl = {});

    static QmlItemNode create(const ModelNode &modelNode) { return QmlItemNode{modelNode}; }

    static QmlItemNode createQmlItemNode(AbstractView *view,
                                         const ItemLibraryEntry &itemLibraryEntry,
                                         const QPointF &position,
                                         QmlItemNode parentQmlItemNode,
                                         SL sl = {});

    static QmlItemNode createQmlItemNodeFromImage(AbstractView *view,
                                                  const QString &imageName,
                                                  const QPointF &position,
                                                  QmlItemNode parentQmlItemNode,
                                                  bool executeInTransaction = true,
                                                  SL sl = {});
    static QmlItemNode createQmlItemNodeFromImage(AbstractView *view,
                                                  const QString &imageName,
                                                  const QPointF &position,
                                                  NodeAbstractProperty parentproperty,
                                                  bool executeInTransaction = true,
                                                  SL sl = {});

    static QmlItemNode createQmlItemNodeFromFont(AbstractView *view,
                                                 const QString &fontFamily,
                                                 const QPointF &position,
                                                 QmlItemNode parentQmlItemNode,
                                                 bool executeInTransaction = true,
                                                 SL sl = {});
    static QmlItemNode createQmlItemNodeFromFont(AbstractView *view,
                                                 const QString &fontFamily,
                                                 const QPointF &position,
                                                 NodeAbstractProperty parentproperty,
                                                 bool executeInTransaction = true,
                                                 SL sl = {});

    static QmlItemNode createQmlItemNodeForEffect(AbstractView *view,
                                                  QmlItemNode parentQmlItemNode,
                                                  const QString &effectPath,
                                                  bool isLayerEffect,
                                                  SL sl = {});
    static QmlItemNode createQmlItemNodeForEffect(AbstractView *view,
                                                  NodeAbstractProperty parentProperty,
                                                  const QString &effectPath,
                                                  bool isLayerEffect,
                                                  SL sl = {});
    static void placeEffectNode(NodeAbstractProperty &parentProperty,
                                const QmlItemNode &effectNode,
                                bool isLayerEffect,
                                SL sl = {});

    QList<QmlItemNode> children(SL sl = {}) const;
    QList<QmlObjectNode> resources(SL sl = {}) const;
    QList<QmlObjectNode> allDirectSubNodes(SL sl = {}) const;
    QmlAnchors anchors(SL sl = {}) const;

    bool hasChildren(SL sl = {}) const;
    bool hasResources(SL sl = {}) const;
    bool instanceHasAnchor(AnchorLineType sourceAnchorLineType, SL sl = {}) const;
    bool instanceHasAnchors(SL sl = {}) const;
    bool instanceHasShowContent(SL sl = {}) const;

    bool instanceCanReparent(SL sl = {}) const;
    bool instanceIsAnchoredBySibling(SL sl = {}) const;
    bool instanceIsAnchoredByChildren(SL sl = {}) const;
    bool instanceIsMovable(SL sl = {}) const;
    bool instanceIsResizable(SL sl = {}) const;
    bool instanceIsInLayoutable(SL sl = {}) const;
    bool instanceHasScaleOrRotationTransform(SL sl = {}) const;

    bool modelIsMovable(SL sl = {}) const;
    bool modelIsResizable(SL sl = {}) const;
    bool modelIsInLayout(SL sl = {}) const;
    bool hasFormEditorItem(SL sl = {}) const;

    QRectF instanceBoundingRect(SL sl = {}) const;
    QRectF instanceSceneBoundingRect(SL sl = {}) const;
    QRectF instancePaintedBoundingRect(SL sl = {}) const;
    QRectF instanceContentItemBoundingRect(SL sl = {}) const;
    QTransform instanceTransform(SL sl = {}) const;
    QTransform instanceTransformWithContentTransform(SL sl = {}) const;
    QTransform instanceTransformWithContentItemTransform(SL sl = {}) const;
    QTransform instanceSceneTransform(SL sl = {}) const;
    QTransform instanceSceneContentItemTransform(SL sl = {}) const;
    QPointF instanceScenePosition(SL sl = {}) const;
    QPointF instancePosition(SL sl = {}) const;
    QSizeF instanceSize(SL sl = {}) const;
    int instancePenWidth(SL sl = {}) const;
    bool instanceIsRenderPixmapNull(SL sl = {}) const;
    bool instanceIsVisible(SL sl = {}) const;

    QPixmap instanceRenderPixmap(SL sl = {}) const;
    QPixmap instanceBlurredRenderPixmap(SL sl = {}) const;

    const QList<QmlItemNode> allDirectSubModelNodes(SL sl = {}) const;
    const QList<QmlItemNode> allSubModelNodes(SL sl = {}) const;
    bool hasAnySubModelNodes(SL sl = {}) const;

    void setPosition(const QPointF &position, SL sl = {});
    void setPostionInBaseState(const QPointF &position, SL sl = {});
    void setFlowItemPosition(const QPointF &position, SL sl = {});
    QPointF flowPosition(SL sl = {}) const;

    void setSize(const QSizeF &size, SL sl = {});
    bool isInLayout(SL sl = {}) const;
    bool canBereparentedTo(const ModelNode &potentialParent, SL sl = {}) const;

    void setRotation(const qreal &angle, SL sl = {});
    qreal rotation(SL sl = {}) const;
    QVariant transformOrigin(SL sl = {});

    bool isInStackedContainer(SL sl = {}) const;

    bool isFlowView(SL sl = {}) const;
    bool isFlowItem(SL sl = {}) const;
    bool isFlowActionArea(SL sl = {}) const;
    ModelNode rootModelNode(SL sl = {}) const;

    bool isEffectItem(SL sl = {}) const;

    friend auto qHash(const QmlItemNode &node) { return qHash(node.modelNode()); }
};

class QmlFlowItemNode;
class QmlFlowViewNode;

class QMLDESIGNER_EXPORT QmlFlowTargetNode final : public QmlItemNode
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

class QMLDESIGNER_EXPORT QmlFlowActionAreaNode final : public QmlItemNode
{
public:
    QmlFlowActionAreaNode(const ModelNode &modelNode)  : QmlItemNode(modelNode) {}
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlFlowActionAreaNode(const ModelNode &modelNode);
    ModelNode targetTransition() const;
    void assignTargetFlowItem(const QmlFlowTargetNode &flowItem);
    QmlFlowItemNode flowItemParent() const;
private:
    void destroyTarget();
};

class QMLDESIGNER_EXPORT QmlFlowItemNode final : public QmlItemNode
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

class QMLDESIGNER_EXPORT QmlFlowViewNode final : public QmlItemNode
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
    QList<ModelNode> transitionsForProperty(PropertyNameView propertyName, const ModelNode &modelNode);

private:
    static PropertyNameList s_mouseSignals;
};

QMLDESIGNER_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
QMLDESIGNER_EXPORT QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList);
QMLDESIGNER_EXPORT QList<QmlItemNode> toQmlItemNodeListKeppInvalid(const QList<ModelNode> &modelNodeList);

} //QmlDesigner
