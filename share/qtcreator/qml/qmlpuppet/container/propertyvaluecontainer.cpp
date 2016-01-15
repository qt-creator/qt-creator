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

#include "propertyvaluecontainer.h"

#include <QtDebug>

namespace QmlDesigner {

PropertyValueContainer::PropertyValueContainer()
    : m_instanceId(-1)
{
}

PropertyValueContainer::PropertyValueContainer(qint32 instanceId, const PropertyName &name, const QVariant &value, const TypeName &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_value(value),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyValueContainer::instanceId() const
{
    return m_instanceId;
}

PropertyName PropertyValueContainer::name() const
{
    return m_name;
}

QVariant PropertyValueContainer::value() const
{
    return m_value;
}

bool PropertyValueContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

TypeName PropertyValueContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyValueContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.value();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_value;
    in >> container.m_dynamicTypeName;

    return in;
}

bool operator ==(const PropertyValueContainer &first, const PropertyValueContainer &second)
{
    return first.m_instanceId == second.m_instanceId
            && first.m_name == second.m_name
            && first.m_value == second.m_value
            && first.m_dynamicTypeName == second.m_dynamicTypeName;
}

bool operator <(const PropertyValueContainer &first, const PropertyValueContainer &second)
{
    return (first.m_instanceId < second.m_instanceId)
        || (first.m_instanceId == second.m_instanceId && first.m_name < second.m_name);
}

QDebug operator <<(QDebug debug, const PropertyValueContainer &container)
{
    debug.nospace() << "PropertyValueContainer("
                    << "instanceId: " << container.instanceId() << ", "
                    << "name: " << container.name() << ", "
                    << "value: " << container.value();

    if (!container.dynamicTypeName().isEmpty())
        debug.nospace() << ", " << "dynamicTypeName: " << container.dynamicTypeName();

    debug.nospace() << ")";

    return debug;
}

} // namespace QmlDesigner
