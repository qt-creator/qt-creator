// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalsignalhandlerproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalSignalHandlerProperty::InternalSignalHandlerProperty(const PropertyName &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}


InternalSignalHandlerProperty::Pointer InternalSignalHandlerProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    auto newPointer(new InternalSignalHandlerProperty(name, propertyOwner));
    InternalSignalHandlerProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
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

bool InternalSignalHandlerProperty::isSignalHandlerProperty() const
{
    return true;
}

InternalSignalDeclarationProperty::Pointer InternalSignalDeclarationProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    auto newPointer(new InternalSignalDeclarationProperty(name, propertyOwner));
    InternalSignalDeclarationProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    newPointer->setDynamicTypeName("signal");

    return smartPointer;
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

bool InternalSignalDeclarationProperty::isSignalDeclarationProperty() const
{
    return true;
}

InternalSignalDeclarationProperty::InternalSignalDeclarationProperty(const PropertyName &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}

} // namespace Internal
} // namespace QmlDesigner
