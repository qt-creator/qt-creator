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

#include "internalvariantproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalVariantProperty::InternalVariantProperty(const PropertyName &name, const InternalNodePointer &node)
        : InternalProperty(name, node)
{
}

InternalVariantProperty::Pointer InternalVariantProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    InternalVariantProperty *newPointer(new InternalVariantProperty(name, propertyOwner));
    InternalVariantProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
}

QVariant InternalVariantProperty::value() const
{
    return m_value;
}

void InternalVariantProperty::setValue(const QVariant &value)
{
    m_value = value;
}

bool InternalVariantProperty::isVariantProperty() const
{
    return true;
}

void InternalVariantProperty::setDynamicValue(const TypeName &type, const QVariant &value)
{
     setValue(value);
     setDynamicTypeName(type);
}

bool InternalVariantProperty::isValid() const
{
    return InternalProperty::isValid() && isVariantProperty();
}

} // namespace Internal
} // namespace QmlDesigner
