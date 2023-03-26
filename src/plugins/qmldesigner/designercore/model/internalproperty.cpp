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

InternalProperty::Pointer InternalProperty::internalPointer() const
{
    Q_ASSERT(!m_internalPointer.isNull());
    return m_internalPointer.toStrongRef();
}

void InternalProperty::setInternalWeakPointer(const Pointer &pointer)
{
    Q_ASSERT(!pointer.isNull());
    m_internalPointer = pointer;
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

QSharedPointer<InternalBindingProperty> InternalProperty::toBindingProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalBindingProperty>());
    return internalPointer().staticCast<InternalBindingProperty>();
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

QSharedPointer<InternalVariantProperty> InternalProperty::toVariantProperty() const

{
    Q_ASSERT(internalPointer().dynamicCast<InternalVariantProperty>());
    return internalPointer().staticCast<InternalVariantProperty>();
}

InternalNode::Pointer InternalProperty::propertyOwner() const
{
    return m_propertyOwner.lock();
}

QSharedPointer<InternalNodeListProperty> InternalProperty::toNodeListProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalNodeListProperty>());
    return internalPointer().staticCast<InternalNodeListProperty>();
}

QSharedPointer<InternalNodeProperty> InternalProperty::toNodeProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalNodeProperty>());
    return internalPointer().staticCast<InternalNodeProperty>();
}

QSharedPointer<InternalNodeAbstractProperty> InternalProperty::toNodeAbstractProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalNodeAbstractProperty>());
    return internalPointer().staticCast<InternalNodeAbstractProperty>();
}

QSharedPointer<InternalSignalHandlerProperty> InternalProperty::toSignalHandlerProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalSignalHandlerProperty>());
    return internalPointer().staticCast<InternalSignalHandlerProperty>();
}

QSharedPointer<InternalSignalDeclarationProperty> InternalProperty::toSignalDeclarationProperty() const
{
    Q_ASSERT(internalPointer().dynamicCast<InternalSignalDeclarationProperty>());
    return internalPointer().staticCast<InternalSignalDeclarationProperty>();
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

