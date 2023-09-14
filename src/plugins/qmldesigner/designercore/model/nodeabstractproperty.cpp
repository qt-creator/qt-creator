// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeabstractproperty.h"
#include "nodeproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidreparentingexception.h"
#include "internalnodeabstractproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

#include <nodemetainfo.h>

namespace QmlDesigner {

NodeAbstractProperty::NodeAbstractProperty() = default;

NodeAbstractProperty::NodeAbstractProperty(const NodeAbstractProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNodeSharedPointer(), property.model(), view)
{
}

NodeAbstractProperty::NodeAbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}

NodeAbstractProperty::NodeAbstractProperty(const Internal::InternalNodeAbstractProperty::Pointer &property, Model *model, AbstractView *view)
    : AbstractProperty(property, model, view)
{}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode)
{
    if (!isValid() || !modelNode.isValid())
        return;

    reparentHere(modelNode,
                 parentModelNode().metaInfo().property(name()).isListProperty()
                     || isDefaultProperty()); //we could use the metasystem instead?
}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode,  bool isNodeList, const TypeName &dynamicTypeName)
{
    if (!isValid() || !modelNode.isValid())
        return;

    if (modelNode.hasParentProperty() && modelNode.parentProperty() == *this
        && dynamicTypeName == modelNode.parentProperty().dynamicTypeName())
        return;

    Internal::WriteLocker locker(model());
    if (isNodeProperty()) {
        NodeProperty nodeProperty(toNodeProperty());
        if (nodeProperty.modelNode().isValid())
            return;
    }

    if (modelNode.isAncestorOf(parentModelNode()))
        return;

    /* This is currently not supported and not required. */
    /* Removing the property does work of course. */
    if (modelNode.hasParentProperty() && modelNode.parentProperty().isDynamic())
        return;

    if (auto internalProperty = internalNode()->property(name());
        internalProperty && !internalProperty->isNodeAbstractProperty()) {
        privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    if (modelNode.hasParentProperty()) {
        Internal::InternalNodeAbstractProperty::Pointer oldParentProperty = modelNode.internalNode()->parentProperty();

        privateModel()->reparentNode(internalNodeSharedPointer(),
                                     name(),
                                     modelNode.internalNode(),
                                     isNodeList,
                                     dynamicTypeName);

        Q_ASSERT(oldParentProperty);

    } else {
        privateModel()->reparentNode(internalNodeSharedPointer(),
                                     name(),
                                     modelNode.internalNode(),
                                     isNodeList,
                                     dynamicTypeName);
    }
}

bool NodeAbstractProperty::isEmpty() const
{
    if (isValid()) {
        Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(
            name());
        if (!property)
            return true;
        else
            return property->isEmpty();
    }

    return true;
}

int NodeAbstractProperty::indexOf(const ModelNode &node) const
{
    if (isValid()) {
        Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(
            name());
        if (!property)
            return 0;

        return property->indexOf(node.internalNode());
    }

    return -1;
}

NodeAbstractProperty NodeAbstractProperty::parentProperty() const
{
    if (!isValid())
        return {};

    if (!internalNode()->parentProperty())
        return {};

    return NodeAbstractProperty(internalNode()->parentProperty()->name(), internalNode()->parentProperty()->propertyOwner(), model(), view());
}

int NodeAbstractProperty::count() const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (!property)
        return 0;
    else
        return property->count();
}

QList<ModelNode> NodeAbstractProperty::allSubNodes()
{
    if (!internalNode() || !internalNode()->isValid)
        return {};

    auto property = internalNode()->property(name());

    if (!property)
        return {};

    switch (property->type()) {
    case PropertyType::Node:
        return QmlDesigner::toModelNodeList({property->to<PropertyType::Node>()->allSubNodes()},
                                            model(),
                                            view());
    case PropertyType::NodeList:
        return QmlDesigner::toModelNodeList({property->to<PropertyType::NodeList>()->allSubNodes()},
                                            model(),
                                            view());
    case PropertyType::Binding:
    case PropertyType::None:
    case PropertyType::SignalDeclaration:
    case PropertyType::SignalHandler:
    case PropertyType::Variant:
        break;
    }

    return {};
}

QList<ModelNode> NodeAbstractProperty::directSubNodes() const
{
    if (!internalNode() || !internalNode()->isValid)
        return {};

    auto property = internalNode()->property(name());

    if (!property)
        return {};

    switch (property->type()) {
    case PropertyType::Node:
        return QmlDesigner::toModelNodeList({property->to<PropertyType::Node>()->node()},
                                            model(),
                                            view());
    case PropertyType::NodeList:
        return QmlDesigner::toModelNodeList({property->to<PropertyType::NodeList>()->nodes()},
                                            model(),
                                            view());
    case PropertyType::Binding:
    case PropertyType::None:
    case PropertyType::SignalDeclaration:
    case PropertyType::SignalHandler:
    case PropertyType::Variant:
        break;
    }

    return {};
}

/*!
    Returns whether property handles \a property1 and \a property2 reference
    the same property in the same node.
*/
bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return AbstractProperty(property1) == AbstractProperty(property2);
}

/*!
    Returns whether the property handles \a property1 and \a property2 do not
    reference the same property in the same node.
  */
bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return !(property1 == property2);
}

QDebug operator<<(QDebug debug, const NodeAbstractProperty &property)
{
    return debug.nospace() << "NodeAbstractProperty(" << (property.isValid() ? property.name() : PropertyName("invalid")) << ')';
}

QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property)
{
    stream << "NodeAbstractProperty(" << property.name() << ')';

    return stream;
}
} // namespace QmlDesigner
