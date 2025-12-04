// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractproperty.h"

namespace QmlDesigner {

namespace Internal {
    class InternalNodeAbstractProperty;
    using InternalNodeAbstractPropertyPointer = std::shared_ptr<InternalNodeAbstractProperty>;
}

class QMLDESIGNERCORE_EXPORT NodeAbstractProperty : public AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

    using SL = ModelTracing::SourceLocation;

    friend QMLDESIGNERCORE_EXPORT bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);

public:
    NodeAbstractProperty();
    NodeAbstractProperty(const NodeAbstractProperty &property, AbstractView *view);
    void reparentHere(const ModelNode &modelNode, SL sl = {});
    bool isEmpty(SL sl = {}) const;
    int count(SL sl = {}) const;
    int indexOf(const ModelNode &node, SL sl = {}) const;
    NodeAbstractProperty parentProperty(SL sl = {}) const;

    QList<ModelNode> allSubNodes(SL sl = {});
    QList<ModelNode> directSubNodes(SL sl = {}) const;

    friend size_t qHash(const NodeAbstractProperty &property) { return qHash(AbstractProperty(property)); }

    NodeAbstractProperty(PropertyNameView propertyName,
                         const Internal::InternalNodePointer &internalNode,
                         Model *model,
                         AbstractView *view)
        : AbstractProperty(propertyName, internalNode, model, view)
    {}

protected:
    NodeAbstractProperty(const Internal::InternalNodeAbstractPropertyPointer &property, Model *model, AbstractView *view);
    void reparentHere(const ModelNode &modelNode, bool isNodeList, const TypeName &typeName = TypeName());
};


QMLDESIGNERCORE_EXPORT bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
QMLDESIGNERCORE_EXPORT bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const NodeAbstractProperty &property);

} // namespace QmlDesigner
