// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "nodeabstractproperty.h"

namespace QmlDesigner {

namespace Internal { class ModelPrivate; }

class QMLDESIGNERCORE_EXPORT NodeProperty final : public NodeAbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

public:
    void setModelNode(const ModelNode &modelNode);
    ModelNode modelNode() const;

    void reparentHere(const ModelNode &modelNode);
    void setDynamicTypeNameAndsetModelNode(const TypeName &typeName, const ModelNode &modelNode);

    NodeProperty();
    NodeProperty(const PropertyName &propertyName,
                 const Internal::InternalNodePointer &internalNode,
                 Model *model,
                 AbstractView *view);
};

} // namespace QmlDesigner
