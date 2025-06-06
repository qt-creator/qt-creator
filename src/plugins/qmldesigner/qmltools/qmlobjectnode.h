// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

class QMLDESIGNER_EXPORT QmlObjectNode : public QmlModelNodeFacade
{
    friend QmlItemNode;
    friend MoveManipulator;

public:
    QmlObjectNode() = default;
    QmlObjectNode(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}

    static bool isValidQmlObjectNode(const ModelNode &modelNode, SL sl = {});
    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    bool hasError() const;
    QString error() const;
    bool hasNodeParent(SL sl = {}) const;
    bool hasInstanceParent(SL sl = {}) const;
    bool hasInstanceParentItem(SL sl = {}) const;
    void setParentProperty(const NodeAbstractProperty &parentProeprty, SL sl = {});
    QmlObjectNode instanceParent(SL sl = {}) const;
    QmlItemNode instanceParentItem(SL sl = {}) const;

    QmlItemNode modelParentItem(SL sl = {}) const;

    void setId(const QString &id, SL sl = {});
    void setNameAndId(const QString &newName, const QString &preferredId, SL sl = {});
    QString id(SL sl = {}) const;
    QString validId(SL sl = {});

    QmlModelState currentState(SL sl = {}) const;
    QmlTimeline currentTimeline(SL sl = {}) const;
    void setVariantProperty(PropertyNameView name, const QVariant &value, SL sl = {});
    void setBindingProperty(PropertyNameView name, const QString &expression, SL sl = {});
    NodeAbstractProperty nodeAbstractProperty(PropertyNameView name, SL sl = {}) const;
    NodeAbstractProperty defaultNodeAbstractProperty(SL sl = {}) const;
    NodeProperty nodeProperty(PropertyNameView name, SL sl = {}) const;
    NodeListProperty nodeListProperty(PropertyNameView name, SL sl = {}) const;

    bool instanceHasValue(PropertyNameView name, SL sl = {}) const;
    QVariant instanceValue(PropertyNameView name, SL sl = {}) const;
    TypeName instanceType(PropertyNameView name, SL sl = {}) const;

    bool hasProperty(PropertyNameView name, SL sl = {}) const;
    bool hasBindingProperty(PropertyNameView name, SL sl = {}) const;
    bool instanceHasBinding(PropertyNameView name, SL sl = {}) const;
    bool propertyAffectedByCurrentState(PropertyNameView name, SL sl = {}) const;
    QVariant modelValue(PropertyNameView name, SL sl = {}) const;
    bool isTranslatableText(PropertyNameView name, SL sl = {}) const;
    QString stripedTranslatableText(PropertyNameView name, SL sl = {}) const;
    BindingProperty bindingProperty(PropertyNameView name, SL sl = {}) const;
    QString expression(PropertyNameView name, SL sl = {}) const;
    bool isInBaseState(SL sl = {}) const;
    bool timelineIsActive(SL sl = {}) const;
    QmlPropertyChanges ensurePropertyChangeForCurrentState(SL sl = {}) const;
    QmlPropertyChanges propertyChangeForCurrentState(SL sl = {}) const;

    bool instanceCanReparent(SL sl = {}) const;

    bool isRootModelNode(SL sl = {}) const;

    void destroy(SL sl = {});

    void ensureAliasExport(SL sl = {});
    bool isAliasExported(SL sl = {}) const;

    QList<QmlModelState> allAffectingStates(SL sl = {}) const;
    QList<QmlModelStateOperation> allAffectingStatesOperations(SL sl = {}) const;

    void removeProperty(PropertyNameView name, SL sl = {});

    void setParent(const QmlObjectNode &newParent, SL sl = {});

    QmlItemNode toQmlItemNode(SL sl = {}) const;
    QmlVisualNode toQmlVisualNode(SL sl = {}) const;

    bool isAncestorOf(const QmlObjectNode &objectNode, SL sl = {}) const;

    bool hasDefaultPropertyName(SL sl = {}) const;
    PropertyName defaultPropertyName(SL sl = {}) const;

    static QVariant instanceValue(const ModelNode &modelNode, PropertyNameView name, SL sl = {});

    static QString generateTranslatableText(const QString &text,
                                            const DesignerSettings &designerSettings,
                                            SL sl = {});

    static QString stripedTranslatableTextFunction(const QString &text, SL sl = {});

    static QString convertToCorrectTranslatableFunction(const QString &text,
                                                        const DesignerSettings &designerSettings,
                                                        SL sl = {});

    QString simplifiedTypeName(SL sl = {}) const;

    QStringList allStateNames(SL sl = {}) const;

    friend auto qHash(const QmlObjectNode &node) { return qHash(node.modelNode()); }

    QList<QmlModelState> allDefinedStates(SL sl = {}) const;
    QList<QmlModelStateOperation> allInvalidStateOperations(SL sl = {}) const;

    QmlModelStateGroup states(SL sl = {}) const;

    QList<ModelNode> allTimelines(SL sl = {}) const;

    QList<ModelNode> getAllConnections(SL sl = {}) const;

protected:
    const NodeInstance &nodeInstance() const;
    QmlObjectNode nodeForInstance(const NodeInstance &instance) const;
    QmlItemNode itemForInstance(const NodeInstance &instance) const;
};

QMLDESIGNER_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &fxObjectNodeList);
QMLDESIGNER_EXPORT QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList);
}// QmlDesigner
