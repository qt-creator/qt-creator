// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalproperty.h"
#include "internalbindingproperty.h"
#include "internalvariantproperty.h"
#include "internalnodelistproperty.h"
#include "internalnodeproperty.h"
#include "internalsignalhandlerproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner {

namespace Internal {

// Creates invalid InternalProperty
InternalProperty::InternalProperty() = default;

InternalProperty::~InternalProperty() = default;

InternalProperty::InternalProperty(const PropertyName &name, const InternalNode::Pointer &propertyOwner)
     : m_name(name),
     m_propertyOwner(propertyOwner)
{
    Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Name of property cannot be empty");
}

bool InternalProperty::isValid() const
{
    return !m_propertyOwner.expired() && !m_name.isEmpty();
}

PropertyName InternalProperty::name() const
{
    return m_name;
}

bool InternalProperty::isBindingProperty() const
{
    return false;
}

bool InternalProperty::isVariantProperty() const
{
    return false;
}

std::shared_ptr<InternalBindingProperty> InternalProperty::toBindingProperty()
{
    Q_ASSERT(std::dynamic_pointer_cast<InternalBindingProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalBindingProperty>(shared_from_this());
}


bool InternalProperty::isNodeListProperty() const
{
     return false;
}

bool InternalProperty::isNodeProperty() const
{
    return false;
}

bool InternalProperty::isNodeAbstractProperty() const
{
    return false;
}

bool InternalProperty::isSignalHandlerProperty() const
{
    return false;
}

bool InternalProperty::isSignalDeclarationProperty() const
{
    return false;
}

std::shared_ptr<InternalVariantProperty> InternalProperty::toVariantProperty()

{
    Q_ASSERT(std::dynamic_pointer_cast<InternalVariantProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalVariantProperty>(shared_from_this());
}

InternalNode::Pointer InternalProperty::propertyOwner() const
{
    return m_propertyOwner.lock();
}

std::shared_ptr<InternalNodeListProperty> InternalProperty::toNodeListProperty()
{
    Q_ASSERT(std::dynamic_pointer_cast<InternalNodeListProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalNodeListProperty>(shared_from_this());
}

std::shared_ptr<InternalNodeProperty> InternalProperty::toNodeProperty()
{
    Q_ASSERT(std::dynamic_pointer_cast<InternalNodeProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalNodeProperty>(shared_from_this());
}

std::shared_ptr<InternalNodeAbstractProperty> InternalProperty::toNodeAbstractProperty()
{
    Q_ASSERT(std::dynamic_pointer_cast<InternalNodeAbstractProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalNodeAbstractProperty>(shared_from_this());
}

std::shared_ptr<InternalSignalHandlerProperty> InternalProperty::toSignalHandlerProperty()
{
    Q_ASSERT(std::dynamic_pointer_cast<InternalSignalHandlerProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalSignalHandlerProperty>(shared_from_this());
}

std::shared_ptr<InternalSignalDeclarationProperty> InternalProperty::toSignalDeclarationProperty()
{
    Q_ASSERT(std::dynamic_pointer_cast<InternalSignalDeclarationProperty>(shared_from_this()));
    return std::static_pointer_cast<InternalSignalDeclarationProperty>(shared_from_this());
}

void InternalProperty::remove()
{
    propertyOwner()->removeProperty(name());
    m_propertyOwner.reset();
}

TypeName InternalProperty::dynamicTypeName() const
{
    return m_dynamicType;
}

void InternalProperty::setDynamicTypeName(const TypeName &name)
{
    m_dynamicType = name;
}


void InternalProperty::resetDynamicTypeName()
{
   m_dynamicType.clear();
}


} //namespace Internal
} //namespace QmlDesigner

