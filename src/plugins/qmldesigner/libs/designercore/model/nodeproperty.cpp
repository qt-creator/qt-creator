// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

namespace QmlDesigner {

NodeProperty::NodeProperty() = default;

void NodeProperty::setModelNode(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"node property set model node",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (!modelNode.isValid())
        return;

    if (auto property = internalNode()->property(name()); property) {
        auto nodeProperty = property->to<PropertyType::Node>();
        if (nodeProperty && nodeProperty->node() == modelNode.internalNode())
            return;

        if (!nodeProperty)
            privateModel()->removePropertyAndRelatedResources(property);
    }

    privateModel()->reparentNode(internalNodeSharedPointer(),
                                 name(),
                                 modelNode.internalNode(),
                                 false); //### we have to add a flag that this is not a list
}

ModelNode NodeProperty::modelNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"node property model node",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    auto internalProperty = internalNode()->nodeProperty(name());
    if (internalProperty) //check if oldValue != value
        return ModelNode(internalProperty->node(), model(), view());

    return ModelNode();
}

void NodeProperty::reparentHere(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"node property reparent here",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    NodeAbstractProperty::reparentHere(modelNode, false);
}

void NodeProperty::setDynamicTypeNameAndsetModelNode(const TypeName &typeName,
                                                     const ModelNode &modelNode,
                                                     SL sl)
{
    NanotraceHR::Tracer tracer{"node property set dynamic type and set model node",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!modelNode.isValid())
        return;

    if (modelNode.hasParentProperty()) /* Not supported */
        return;

    NodeAbstractProperty::reparentHere(modelNode, false, typeName);
}

} // namespace QmlDesigner
