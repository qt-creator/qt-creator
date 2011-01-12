/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "internalbindingproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalBindingProperty::InternalBindingProperty(const QString &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}


InternalBindingProperty::Pointer InternalBindingProperty::create(const QString &name, const InternalNodePointer &propertyOwner)
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

void InternalBindingProperty::setDynamicExpression(const QString &type, const QString &expression)
{
    setExpression(expression);
    setDynamicTypeName(type);
}

} // namespace Internal
} // namespace QmlDesigner
