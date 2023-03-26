// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyabstractcontainer.h"

#include <QDebug>

namespace QmlDesigner {

PropertyAbstractContainer::PropertyAbstractContainer()
    : m_instanceId(-1)
{
}

PropertyAbstractContainer::PropertyAbstractContainer(qint32 instanceId, const PropertyName &name, const TypeName &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyAbstractContainer::instanceId() const
{
    return m_instanceId;
}

PropertyName PropertyAbstractContainer::name() const
{
    return m_name;
}

bool PropertyAbstractContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

TypeName PropertyAbstractContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_dynamicTypeName;

    return in;
}

QDebug operator <<(QDebug debug, const PropertyAbstractContainer &container)
{
    debug.nospace() << "PropertyAbstractContainer("
                    << "instanceId: " << container.instanceId() << ", "
                    << "name: " << container.name();

    if (!container.dynamicTypeName().isEmpty())
        debug.nospace() << ", " << "dynamicTypeName: " << container.dynamicTypeName();

    debug.nospace() << ")";

    return debug;
}


} // namespace QmlDesigner
