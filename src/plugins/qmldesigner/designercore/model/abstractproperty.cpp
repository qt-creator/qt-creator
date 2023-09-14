// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractproperty.h"
#include "internalnode_p.h"
#include <model.h>
#include "model_p.h"
#include <modelnode.h>
#include "variantproperty.h"
#include "bindingproperty.h"
#include "signalhandlerproperty.h"
#include "nodeproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include <QTextStream>
#include <qmlobjectnode.h>

namespace QmlDesigner {

 /*!
\class QmlDesigner::AbstractProperty
\ingroup CoreModel
\brief The AbstractProperty class is a value holder for a property.
*/

AbstractProperty::AbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer  &internalNode, Model* model,  AbstractView *view)
    : m_propertyName(propertyName),
    m_internalNode(internalNode),
    m_model(model),
    m_view(view)
{
    Q_ASSERT_X(!m_propertyName.contains(' '), Q_FUNC_INFO, "a property name cannot contain a space");
}

AbstractProperty::AbstractProperty(const Internal::InternalPropertyPointer &property, Model* model,  AbstractView *view)
        : m_propertyName(property->name()),
        m_internalNode(property->propertyOwner()),
        m_model(model),
        m_view(view)
{
}

AbstractProperty::AbstractProperty(const AbstractProperty &property, AbstractView *view)
    : m_propertyName(property.name()),
      m_internalNode(property.internalNode()),
      m_model(property.model()),
      m_view(view)
{

}

AbstractProperty::~AbstractProperty() = default;

Internal::ModelPrivate *AbstractProperty::privateModel() const
{
    return m_model ? m_model->d.get() : nullptr;
}

Model *AbstractProperty::model() const
{
    return m_model.data();
}

AbstractView *AbstractProperty::view() const
{
    return m_view.data();
}

 /*!
 Holds a value for a property. Returns the value of the property.

 The QVariant is null if the property does not exist.
*/
const PropertyName &AbstractProperty::name() const
{
    return m_propertyName;
}

 /*!
 Checks if the property is valid.

 A property is valid if the belonging model node
 is valid. This function is not overloaded for
 subclasses.

 Returns whether the property is valid for the \c true return value.
*/
bool AbstractProperty::isValid() const
{
    return m_internalNode && !m_model.isNull() && m_internalNode->isValid
           && !m_propertyName.isEmpty() && !m_propertyName.contains(' ') && m_propertyName != "id";
}

bool AbstractProperty::exists() const
{
    if (!isValid())
        return false;
    return parentModelNode().hasProperty(name());
}

 /*!
    Returns the model node to which the property belongs.
*/
ModelNode AbstractProperty::parentModelNode() const
{
    return ModelNode(m_internalNode, m_model.data(), m_view);
}

/*!
    Returns the QmlObjectNode to which the property belongs.
*/
QmlObjectNode AbstractProperty::parentQmlObjectNode() const
{
    return QmlObjectNode(parentModelNode());
}
/*!
    Returns whether the property is the default property for the model node.
*/
bool AbstractProperty::isDefaultProperty() const
{
    return ModelNode(m_internalNode, m_model.data(), view()).metaInfo().defaultPropertyName() == m_propertyName;
}

VariantProperty AbstractProperty::toVariantProperty() const
{
    if (!isValid())
        return {};

    VariantProperty propertyVariant(name(), internalNodeSharedPointer(), model(), view());

    if (propertyVariant.isVariantProperty())
        return propertyVariant;

    return VariantProperty();
}

NodeProperty AbstractProperty::toNodeProperty() const
{
    if (!isValid())
        return {};

    NodeProperty propertyNode(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNode.isNodeProperty())
        return propertyNode;

    return NodeProperty();
}

SignalHandlerProperty AbstractProperty::toSignalHandlerProperty() const
{
    if (!isValid())
        return {};

    SignalHandlerProperty propertyNode(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNode.isSignalHandlerProperty())
        return propertyNode;

    return SignalHandlerProperty();
}

SignalDeclarationProperty AbstractProperty::toSignalDeclarationProperty() const
{
    if (!isValid())
        return {};

    SignalDeclarationProperty propertyNode(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNode.isSignalDeclarationProperty())
        return propertyNode;

    return SignalDeclarationProperty();
}

NodeListProperty AbstractProperty::toNodeListProperty() const
{
    if (!isValid())
        return {};

    NodeListProperty propertyNodeList(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNodeList.isNodeListProperty())
        return propertyNodeList;

    return NodeListProperty();
}

NodeAbstractProperty AbstractProperty::toNodeAbstractProperty() const
{
    if (!isValid())
        return {};

    NodeAbstractProperty propertyNodeAbstract(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNodeAbstract.isNodeAbstractProperty())
        return propertyNodeAbstract;

    return NodeAbstractProperty();
}

BindingProperty AbstractProperty::toBindingProperty() const
{
    if (!isValid())
        return {};

    BindingProperty propertyBinding(name(), internalNodeSharedPointer(), model(), view());

    if (propertyBinding.isBindingProperty())
        return propertyBinding;

    return BindingProperty();
}

bool AbstractProperty::isVariantProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isVariantProperty();

    return false;
}

bool AbstractProperty::isNodeAbstractProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isNodeAbstractProperty();

    return false;
}

bool AbstractProperty::isNodeListProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isNodeListProperty();

    return false;
}

bool AbstractProperty::isNodeProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isNodeProperty();

    return false;
}

bool AbstractProperty::isSignalHandlerProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isSignalHandlerProperty();

    return false;
}

bool AbstractProperty::isSignalDeclarationProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isSignalDeclarationProperty();

    return false;
}

PropertyType AbstractProperty::type() const
{
    if (!isValid())
        return PropertyType::None;

    if (auto property = internalNode()->property(name()))
        return property->propertyType();

    return PropertyType::None;
}

bool AbstractProperty::isBindingProperty() const
{
    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isBindingProperty();

    return false;
}

bool AbstractProperty::isDynamic() const
{
    return !dynamicTypeName().isEmpty();
}

TypeName AbstractProperty::dynamicTypeName() const
{
    if (!isValid())
        return {};

    if (auto property = internalNode()->property(name()))
        return property->dynamicTypeName();

    return TypeName();
}

QDebug operator<<(QDebug debug, const AbstractProperty &property)
{
    return debug.nospace() << "AbstractProperty("
                           << (property.isValid() ? property.name() : PropertyName("invalid")) << ')';
}

QTextStream &operator<<(QTextStream &stream, const AbstractProperty &property)
{
    stream << "AbstractProperty(" << property.name() << ')';

    return stream;
}

} // namespace QmlDesigner
