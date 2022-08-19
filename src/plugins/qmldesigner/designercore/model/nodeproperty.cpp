// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodeproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidargumentexception.h"
#include "invalidreparentingexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

namespace QmlDesigner {

NodeProperty::NodeProperty() = default;

NodeProperty::NodeProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view)
    :  NodeAbstractProperty(propertyName, internalNode, model, view)
{

}

void NodeProperty::setModelNode(const ModelNode &modelNode)
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modelNode.isValid())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isNodeProperty()
            && internalProperty->toNodeProperty()->node() == modelNode.internalNode())
            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeProperty())
        privateModel()->removeProperty(internalNode()->property(name()));

    privateModel()->reparentNode(internalNode(), name(), modelNode.internalNode(), false); //### we have to add a flag that this is not a list
}

ModelNode NodeProperty::modelNode() const
{
     if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

     if (internalNode()->hasProperty(name())) { //check if oldValue != value
         Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
         if (internalProperty->isNodeProperty())
             return ModelNode(internalProperty->toNodeProperty()->node(), model(), view());
     }

    return ModelNode();
}

void NodeProperty::reparentHere(const ModelNode &modelNode)
{
    NodeAbstractProperty::reparentHere(modelNode, false);
}

void NodeProperty::setDynamicTypeNameAndsetModelNode(const TypeName &typeName, const ModelNode &modelNode)
{
    if (!modelNode.isValid())
       throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode.hasParentProperty()) /* Not supported */
        throw InvalidReparentingException(__LINE__, __FUNCTION__, __FILE__);

    NodeAbstractProperty::reparentHere(modelNode, false, typeName);
}

} // namespace QmlDesigner
