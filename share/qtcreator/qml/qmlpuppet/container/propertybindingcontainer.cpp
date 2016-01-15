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

#include "propertybindingcontainer.h"

#include <QDebug>

namespace QmlDesigner {

PropertyBindingContainer::PropertyBindingContainer()
    : m_instanceId(-1)
{
}

PropertyBindingContainer::PropertyBindingContainer(qint32 instanceId, const PropertyName &name, const QString &expression, const TypeName &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_expression(expression),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyBindingContainer::instanceId() const
{
    return m_instanceId;
}

PropertyName PropertyBindingContainer::name() const
{
    return m_name;
}

QString PropertyBindingContainer::expression() const
{
    return m_expression;
}

bool PropertyBindingContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

TypeName PropertyBindingContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyBindingContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.expression();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_expression;
    in >> container.m_dynamicTypeName;

    return in;
}

QDebug operator <<(QDebug debug, const PropertyBindingContainer &container)
{
    debug.nospace() << "PropertyBindingContainer("
                    << "instanceId: " << container.instanceId() << ", "
                    << "name: " << container.name() << ", "
                    << "expression: " << container.expression();

    if (!container.dynamicTypeName().isEmpty())
        debug.nospace() << ", " << "dynamicTypeName: " << container.dynamicTypeName();

    return debug.nospace() << ")";
}

} // namespace QmlDesigner
