/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "internalvariantproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalVariantProperty::InternalVariantProperty(const QString &name, const InternalNodePointer &node)
        : InternalProperty(name, node)
{
}

InternalVariantProperty::Pointer InternalVariantProperty::create(const QString &name, const InternalNodePointer &propertyOwner)
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

void InternalVariantProperty::setDynamicValue(const QString &type, const QVariant &value)
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
