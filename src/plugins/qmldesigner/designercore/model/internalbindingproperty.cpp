// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalbindingproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalBindingProperty::InternalBindingProperty(const PropertyName &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}


InternalBindingProperty::Pointer InternalBindingProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    auto newPointer(new InternalBindingProperty(name, propertyOwner));
    InternalBindingProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
}

bool InternalBindingProperty::isValid() const
{
    return InternalProperty::isValid() && isBindingProperty();
}

QString InternalBindingProperty::expression() const
{
    return m_expression;
}
void InternalBindingProperty::setExpression(const QString &expression)
{
    m_expression = expression;
}

bool InternalBindingProperty::isBindingProperty() const
{
    return true;
}

void InternalBindingProperty::setDynamicExpression(const TypeName &type, const QString &expression)
{
    setExpression(expression);
    setDynamicTypeName(type);
}

} // namespace Internal
} // namespace QmlDesigner
