// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyvaluecontainer.h"

#include <QtDebug>

namespace QmlDesigner {

PropertyValueContainer::PropertyValueContainer()
    : m_instanceId(-1)
{
}

PropertyValueContainer::PropertyValueContainer(qint32 instanceId,
                                               const PropertyName &name,
                                               const QVariant &value,
                                               const TypeName &dynamicTypeName,
                                               AuxiliaryDataType auxiliaryDataType)
    : m_instanceId(instanceId)
    , m_name(name)
    , m_value(value)
    , m_dynamicTypeName(dynamicTypeName)
    , m_auxiliaryDataType{auxiliaryDataType}
{
}

PropertyValueContainer::PropertyValueContainer(qint32 option)
    : m_instanceId(option),
    m_name("-option-")
{
// '-option-' is not a valid property name and indicates that we store the transaction option.
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

// The reflection flag indicates that a property change notification
// is reflected. This means that the notification is the reaction to a
// property change original done by the puppet itself.
// In the Qt5InformationNodeInstanceServer such notification are
// therefore ignored.

void PropertyValueContainer::setReflectionFlag(bool b)
{
    m_isReflected = b;
}

bool PropertyValueContainer::isReflected() const
{
    return m_isReflected;
}

AuxiliaryDataType PropertyValueContainer::auxiliaryDataType() const
{
    return m_auxiliaryDataType;
}

QDataStream &operator<<(QDataStream &out, const PropertyValueContainer &container)
{
    out << container.m_instanceId;
    out << container.m_name;
    out << container.m_value;
    out << container.m_dynamicTypeName;
    out << container.m_isReflected;
    out << container.m_auxiliaryDataType;

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_value;
    in >> container.m_dynamicTypeName;
    in >> container.m_isReflected;
    in >> container.m_auxiliaryDataType;

    return in;
}

bool operator ==(const PropertyValueContainer &first, const PropertyValueContainer &second)
{
    return first.m_instanceId == second.m_instanceId && first.m_name == second.m_name
           && first.m_value == second.m_value && first.m_dynamicTypeName == second.m_dynamicTypeName
           && first.m_isReflected == second.m_isReflected
           && first.m_auxiliaryDataType == second.m_auxiliaryDataType;
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
