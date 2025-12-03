// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractproperty.h"
#include "internalnode_p.h"
#include "model_p.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "signalhandlerproperty.h"
#include "variantproperty.h"

#include <QByteArrayView>
#include <QTextStream>

namespace QmlDesigner {

 /*!
\class QmlDesigner::AbstractProperty
\ingroup CoreModel
\brief The AbstractProperty class is a value holder for a property.
*/

AbstractProperty::AbstractProperty(PropertyNameView propertyName,
                                   const Internal::InternalNodePointer &internalNode,
                                   Model *model,
                                   AbstractView *view)
    : m_propertyName(propertyName)
    , m_internalNode(internalNode)
    , m_model(model)
    , m_view(view)
{
    Q_ASSERT_X(!m_propertyName.contains(' '), Q_FUNC_INFO, "a property name cannot contain a space");
}

AbstractProperty::AbstractProperty(const Internal::InternalPropertyPointer &property,
                                   Model *model,
                                   AbstractView *view)
    : m_propertyName(property->name())
    , m_internalNode(property->propertyOwner())
    , m_model(model)
    , m_view(view)
{
}

AbstractProperty::AbstractProperty(const AbstractProperty &property, AbstractView *view)
    : m_propertyName(property.name())
    , m_internalNode(property.internalNode())
    , m_model(property.model())
    , m_view(view)
{}

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
    Returns whether the property is the default property for the model node.
*/
bool AbstractProperty::isDefaultProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is default property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    return ModelNode(m_internalNode, m_model.data(), view()).metaInfo(sl).defaultPropertyName(sl)
           == m_propertyName;
}

VariantProperty AbstractProperty::toVariantProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to variant property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    VariantProperty propertyVariant(name(), internalNodeSharedPointer(), model(), view());

    if (propertyVariant.isVariantProperty(sl))
        return propertyVariant;

    return VariantProperty();
}

NodeProperty AbstractProperty::toNodeProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to node property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    NodeProperty propertyNode(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNode.isNodeProperty(sl))
        return propertyNode;

    return NodeProperty();
}

SignalHandlerProperty AbstractProperty::toSignalHandlerProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to signal handler property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    SignalHandlerProperty propertyNode(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNode.isSignalHandlerProperty(sl))
        return propertyNode;

    return SignalHandlerProperty();
}

SignalDeclarationProperty AbstractProperty::toSignalDeclarationProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to signal declaration property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    SignalDeclarationProperty propertyNode(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNode.isSignalDeclarationProperty(sl))
        return propertyNode;

    return SignalDeclarationProperty();
}

NodeListProperty AbstractProperty::toNodeListProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to node list property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    NodeListProperty propertyNodeList(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNodeList.isNodeListProperty(sl))
        return propertyNodeList;

    return NodeListProperty();
}

NodeAbstractProperty AbstractProperty::toNodeAbstractProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to node abstract property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    NodeAbstractProperty propertyNodeAbstract(name(), internalNodeSharedPointer(), model(), view());

    if (propertyNodeAbstract.isNodeAbstractProperty(sl))
        return propertyNodeAbstract;

    return NodeAbstractProperty();
}

BindingProperty AbstractProperty::toBindingProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property to binding property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    BindingProperty propertyBinding(name(), internalNodeSharedPointer(), model(), view());

    if (propertyBinding.isBindingProperty(sl))
        return propertyBinding;

    return BindingProperty();
}

bool AbstractProperty::isVariantProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is variant property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isVariantProperty();

    return false;
}

bool AbstractProperty::isNodeAbstractProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is node abstract property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isNodeAbstractProperty();

    return false;
}

bool AbstractProperty::isNodeListProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is node list property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isNodeListProperty();

    return false;
}

bool AbstractProperty::isNodeProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is node property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isNodeProperty();

    return false;
}

bool AbstractProperty::isSignalHandlerProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is signal handler property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isSignalHandlerProperty();

    return false;
}

bool AbstractProperty::isSignalDeclarationProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is signal declaration property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isSignalDeclarationProperty();

    return false;
}

PropertyType AbstractProperty::type(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property type",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return PropertyType::None;

    if (auto property = internalNode()->property(name()))
        return property->propertyType();

    return PropertyType::None;
}

bool AbstractProperty::isBindingProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is binding property",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return property->isBindingProperty();

    return false;
}

bool AbstractProperty::isDynamic(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property is dynamic",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (auto property = internalNode()->property(name()))
        return not property->dynamicTypeName().isEmpty();

    return false;
}

TypeName AbstractProperty::dynamicTypeName(SL sl) const
{
    NanotraceHR::Tracer tracer{"abstract property dynamic type name",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    if (auto property = internalNode()->property(name()))
        return property->dynamicTypeName();

    return TypeName();
}

QDebug operator<<(QDebug debug, const AbstractProperty &property)
{
    return debug.nospace() << "AbstractProperty("
                           << (property.isValid() ? property.name() : PropertyNameView("invalid"))
                           << ')';
}

QTextStream &operator<<(QTextStream &stream, const AbstractProperty &property)
{
    stream << "AbstractProperty(" << property.name().toByteArray() << ')';

    return stream;
}

} // namespace QmlDesigner
