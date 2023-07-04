// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalsignalhandlerproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalSignalHandlerProperty::InternalSignalHandlerProperty(const PropertyName &name,
                                                             const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner, PropertyType::SignalHandler)
{
}

bool InternalSignalHandlerProperty::isValid() const
{
    return InternalProperty::isValid() && isSignalHandlerProperty();
}

QString InternalSignalHandlerProperty::source() const
{
    return m_source;
}
void InternalSignalHandlerProperty::setSource(const QString &source)
{
    m_source = source;
}

bool InternalSignalDeclarationProperty::isValid() const
{
    return InternalProperty::isValid() && isSignalDeclarationProperty();
}

QString InternalSignalDeclarationProperty::signature() const
{
    return m_signature;
}

void InternalSignalDeclarationProperty::setSignature(const QString &signature)
{
    m_signature = signature;
}

InternalSignalDeclarationProperty::InternalSignalDeclarationProperty(
    const PropertyName &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner, PropertyType::SignalDeclaration)
{
    setDynamicTypeName("signal");
}

} // namespace Internal
} // namespace QmlDesigner
