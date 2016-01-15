/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
InternalProperty::InternalProperty()
{
}

InternalProperty::~InternalProperty()
{
}

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
    return m_propertyOwner && !m_name.isEmpty();
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

QSharedPointer<InternalVariantProperty> InternalProperty::toVariantProperty() const

{
    Q_ASSERT(internalPointer().dynamicCast<InternalVariantProperty>());
    return internalPointer().staticCast<InternalVariantProperty>();
}

InternalNode::Pointer InternalProperty::propertyOwner() const
{
    return m_propertyOwner.toStrongRef();
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

void InternalProperty::remove()
{
    propertyOwner()->removeProperty(name());
    m_propertyOwner.clear();
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

