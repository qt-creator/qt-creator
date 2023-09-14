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

public:
    void setExpression(const QString &expression);
    QString expression() const;

    BindingProperty();
    BindingProperty(const BindingProperty &property, AbstractView *view);

    void setDynamicTypeNameAndExpression(const TypeName &type, const QString &expression);

    ModelNode resolveToModelNode() const;
    AbstractProperty resolveToProperty() const;
    bool isList() const;
    QList<ModelNode> resolveToModelNodeList() const;
    void addModelNodeToArray(const ModelNode &modelNode);
    void removeModelNodeFromArray(const ModelNode &modelNode);

    static QList<BindingProperty> findAllReferencesTo(const ModelNode &modelNode);
    static void deleteAllReferencesTo(const ModelNode &modelNode);


    bool isAlias() const;
    bool isAliasExport() const;

    static QVariant convertToLiteral(const TypeName &typeName, const QString &expression);

    BindingProperty(const PropertyName &propertyName,
                    const Internal::InternalNodePointer &internalNode,
                    Model *model,
                    AbstractView *view);

private:
    ModelNode resolveBinding(const QString &binding, ModelNode currentNode) const;
};

using BindingProperties = QList<BindingProperty>;

bool compareBindingProperties(const QmlDesigner::BindingProperty &bindingProperty01, const QmlDesigner::BindingProperty &bindingProperty02);

QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const BindingProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const BindingProperty &AbstractProperty);

} // namespace QmlDesigner
