// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlstate.h"
#include "qmltimeline.h"
#include "qmlchangeset.h"

#include <nodeinstance.h>

namespace QmlDesigner {

class QmlItemNode;
class QmlPropertyChanges;
class MoveManipulator;
class QmlVisualNode;
class DesignerSettings;

class QMLDESIGNERCORE_EXPORT QmlObjectNode : public QmlModelNodeFacade
{
    friend QmlItemNode;
    friend MoveManipulator;

public:
    QmlObjectNode() = default;
    QmlObjectNode(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}
    virtual ~QmlObjectNode() = default;

    static bool isValidQmlObjectNode(const ModelNode &modelNode);
    bool isValid() const;
    explicit operator bool() const { return isValid(); }

    bool hasError() const;
    QString error() const;
    bool hasNodeParent() const;
    bool hasInstanceParent() const;
    bool hasInstanceParentItem() const;
    void setParentProperty(const NodeAbstractProperty &parentProeprty);
    QmlObjectNode instanceParent() const;
    QmlItemNode instanceParentItem() const;

    QmlItemNode modelParentItem() const;

    void setId(const QString &id);
    QString id() const;
    QString validId();

    QmlModelState currentState() const;
    QmlTimeline currentTimeline() const;
    void setVariantProperty(const PropertyName &name, const QVariant &value);
    void setBindingProperty(const PropertyName &name, const QString &expression);
    NodeAbstractProperty nodeAbstractProperty(const PropertyName &name) const;
    NodeAbstractProperty defaultNodeAbstractProperty() const;
    NodeProperty nodeProperty(const PropertyName &name) const;
    NodeListProperty nodeListProperty(const PropertyName &name) const;

    bool instanceHasValue(const PropertyName &name) const;
    QVariant instanceValue(const PropertyName &name) const;
    TypeName instanceType(const PropertyName &name) const;

    bool hasProperty(const PropertyName &name) const;
    bool hasBindingProperty(const PropertyName &name) const;
    bool instanceHasBinding(const PropertyName &name) const;
    bool propertyAffectedByCurrentState(const PropertyName &name) const;
    QVariant modelValue(const PropertyName &name) const;
    bool isTranslatableText(const PropertyName &name) const;
    QString stripedTranslatableText(const PropertyName &name) const;
    BindingProperty bindingProperty(const PropertyName &name) const;
    QString expression(const PropertyName &name) const;
    bool isInBaseState() const;
    bool timelineIsActive() const;
    QmlPropertyChanges propertyChangeForCurrentState() const;

    bool instanceCanReparent() const;

    bool isRootModelNode() const;

    void destroy();

    void ensureAliasExport();
    bool isAliasExported() const;

    QList<QmlModelState> allAffectingStates() const;
    QList<QmlModelStateOperation> allAffectingStatesOperations() const;

    void removeProperty(const PropertyName &name);

    void setParent(const QmlObjectNode &newParent);

    QmlItemNode toQmlItemNode() const;
    QmlVisualNode toQmlVisualNode() const;

    bool isAncestorOf(const QmlObjectNode &objectNode) const;

    bool hasDefaultPropertyName() const;
    PropertyName defaultPropertyName() const;

    static  QVariant instanceValue(const ModelNode &modelNode, const PropertyName &name);

    static QString generateTranslatableText(const QString &text,
                                            const DesignerSettings &designerSettings);

    static QString stripedTranslatableTextFunction(const QString &text);

    static QString convertToCorrectTranslatableFunction(const QString &text,
                                                        const DesignerSettings &designerSettings);

    QString simplifiedTypeName() const;

    QStringList allStateNames() const;

    friend auto qHash(const QmlObjectNode &node) { return qHash(node.modelNode()); }
    QList<QmlModelState> allDefinedStates() const;
    QList<QmlModelStateOperation> allInvalidStateOperations() const;

    QmlModelStateGroup states() const;

    QList<ModelNode> allTimelines() const;

    QList<ModelNode> getAllConnections() const;

protected:
    NodeInstance nodeInstance() const;
    QmlObjectNode nodeForInstance(const NodeInstance &instance) const;
    QmlItemNode itemForInstance(const NodeInstance &instance) const;
};

QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &fxObjectNodeList);
QMLDESIGNERCORE_EXPORT QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList);
}// QmlDesigner
