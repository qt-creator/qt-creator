// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner {

template<typename String> void convertToString(String &string, PropertyType type)
{
    string.append("\"");

    switch (type) {
    case PropertyType::None:
        string.append("None"_sv);
        break;
    case PropertyType::Variant:
        string.append("Variant"_sv);
        break;
    case PropertyType::Node:
        string.append("Node"_sv);
        break;
    case PropertyType::NodeList:
        string.append("NodeList"_sv);
        break;
    case PropertyType::Binding:
        string.append("Binding"_sv);
        break;
    case PropertyType::SignalHandler:
        string.append("SignalHandler"_sv);
        break;
    case PropertyType::SignalDeclaration:
        string.append("SignalDeclaration"_sv);
        break;
    }

    string.append("\"");
}

namespace Internal {

InternalProperty::~InternalProperty() = default;

InternalProperty::InternalProperty(PropertyNameView name,
                                   const InternalNode::Pointer &propertyOwner,
                                   PropertyType propertyType)
    : m_name(name)
    , m_propertyOwner(propertyOwner)
    , m_propertyType(propertyType)
    , traceToken(propertyOwner->traceToken.begin(name,
                                                 keyValue("owner", propertyOwner->internalId),
                                                 keyValue("type", propertyType)))
{}

bool InternalProperty::isValid() const
{
    return !m_propertyOwner.expired() && !m_name.isEmpty();
}

PropertyNameView InternalProperty::name() const
{
    return m_name;
}

TypeName InternalProperty::dynamicTypeName() const
{
    return m_dynamicType;
}

void InternalProperty::setDynamicTypeName(const TypeName &name)
{
    traceToken.tick("dynamic type name"_t, keyValue("name", name));

    m_dynamicType = name;
}

void InternalProperty::resetDynamicTypeName()
{
    traceToken.tick("reset dynamic type name"_t);

    m_dynamicType.clear();
}

} // namespace Internal
} // namespace QmlDesigner
