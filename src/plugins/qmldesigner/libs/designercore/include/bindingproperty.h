// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "abstractproperty.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT BindingProperty final : public QmlDesigner::AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

    using SL = ModelTracing::SourceLocation;

public:
    void setExpression(const QString &expression, SL sl = {});
    const QString &expression(SL sl = {}) const;

    BindingProperty();
    BindingProperty(const BindingProperty &property, AbstractView *view);

    void setDynamicTypeNameAndExpression(const TypeName &type, const QString &expression, SL sl = {});

    ModelNode resolveToModelNode(SL sl = {}) const;
    AbstractProperty resolveToProperty(SL sl = {}) const;
    bool isList(SL sl = {}) const;
    QList<ModelNode> resolveListToModelNodes(SL sl = {}) const;
    QList<ModelNode> resolveToModelNodes(SL sl = {}) const;
    void addModelNodeToArray(const ModelNode &modelNode, SL sl = {});
    void removeModelNodeFromArray(const ModelNode &modelNode, SL sl = {});

    static QList<BindingProperty> findAllReferencesTo(const ModelNode &modelNode, SL sl = {});
    static void deleteAllReferencesTo(const ModelNode &modelNode, SL sl = {});

    bool canBeReference(SL sl = {}) const;

    bool isAlias(SL sl = {}) const;
    bool isAliasExport(SL sl = {}) const;

    static QVariant convertToLiteral(const TypeName &typeName, const QString &expression, SL sl = {});

    BindingProperty(Utils::SmallStringView propertyName,
                    const Internal::InternalNodePointer &internalNode,
                    Model *model,
                    AbstractView *view)
        : AbstractProperty(propertyName, internalNode, model, view)
    {}

private:
    ModelNode resolveBinding(QStringView binding, ModelNode currentNode) const;
};

using BindingProperties = QList<BindingProperty>;

bool compareBindingProperties(const QmlDesigner::BindingProperty &bindingProperty01,
                              const QmlDesigner::BindingProperty &bindingProperty02);

QMLDESIGNERCORE_EXPORT QTextStream &operator<<(QTextStream &stream, const BindingProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const BindingProperty &AbstractProperty);

} // namespace QmlDesigner
