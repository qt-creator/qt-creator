// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner::Internal {

static auto traceTokenInitArg(const QByteArray &name)
{
    return ModelTracing::category().beginObject("InternalProperty"_t, keyValue("name", name));
}

// Creates invalid InternalProperty
InternalProperty::InternalProperty()
    : traceToken(traceTokenInitArg(m_name))
{}

InternalProperty::~InternalProperty() = default;

InternalProperty::InternalProperty(const PropertyName &name,
                                   const InternalNode::Pointer &propertyOwner,
                                   PropertyType propertyType)
    : m_name(name)
    , m_propertyOwner(propertyOwner)
    , m_propertyType(propertyType)
    , traceToken(traceTokenInitArg(m_name))
{}

bool InternalProperty::isValid() const
{
    return !m_propertyOwner.expired() && !m_name.isEmpty();
}

PropertyName InternalProperty::name() const
{
    return m_name;
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

} //namespace QmlDesigner::Internal

