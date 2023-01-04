// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
