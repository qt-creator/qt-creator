/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "internalbindingproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalBindingProperty::InternalBindingProperty(const PropertyName &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}


InternalBindingProperty::Pointer InternalBindingProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    InternalBindingProperty *newPointer(new InternalBindingProperty(name, propertyOwner));
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
