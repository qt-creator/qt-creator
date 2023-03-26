// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "instancecontainer.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

static TypeName properDelemitingOfType(const TypeName &typeName)
{
    TypeName convertedTypeName = typeName;
    int lastIndex = typeName.lastIndexOf('.');
    if (lastIndex > 0)
        convertedTypeName[lastIndex] = '/';

    return convertedTypeName;
}

InstanceContainer::InstanceContainer() = default;

InstanceContainer::InstanceContainer(qint32 instanceId,
                                     const TypeName &type,
                                     int majorNumber,
                                     int minorNumber,
                                     const QString &componentPath,
                                     const QString &nodeSource,
                                     NodeSourceType nodeSourceType,
                                     NodeMetaType metaType,
                                     NodeFlags metaFlags)
    : m_instanceId(instanceId)
    ,m_type(properDelemitingOfType(type))
    ,m_majorNumber(majorNumber)
    ,m_minorNumber(minorNumber)
    ,m_componentPath(componentPath)
    ,m_nodeSource(nodeSource)
    ,m_nodeSourceType(nodeSourceType)
    ,m_metaType(metaType)
    ,m_metaFlags(metaFlags)
{
}

qint32 InstanceContainer::instanceId() const
{
    return m_instanceId;
}

TypeName InstanceContainer::type() const
{
    return m_type;
}

int InstanceContainer::majorNumber() const
{
    return m_majorNumber;
}

int InstanceContainer::minorNumber() const
{
    return m_minorNumber;
}

QString InstanceContainer::componentPath() const
{
    return m_componentPath;
}

QString InstanceContainer::nodeSource() const
{
    return m_nodeSource;
}

InstanceContainer::NodeSourceType InstanceContainer::nodeSourceType() const
{
    return static_cast<NodeSourceType>(m_nodeSourceType);
}

InstanceContainer::NodeMetaType InstanceContainer::metaType() const
{
    return static_cast<NodeMetaType>(m_metaType);
}

InstanceContainer::NodeFlags InstanceContainer::metaFlags() const
{
    return NodeFlags(m_metaFlags);
}

bool InstanceContainer::checkFlag(NodeFlag flag) const
{
    return NodeFlags(m_metaFlags).testFlag(flag);
}

QDataStream &operator<<(QDataStream &out, const InstanceContainer &container)
{
    out << container.instanceId();
    out << container.type();
    out << container.majorNumber();
    out << container.minorNumber();
    out << container.componentPath();
    out << container.nodeSource();
    out << qint32(container.nodeSourceType());
    out << qint32(container.metaType());
    out << qint32(container.metaFlags());

    return out;
}


QDataStream &operator>>(QDataStream &in, InstanceContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_type;
    in >> container.m_majorNumber;
    in >> container.m_minorNumber;
    in >> container.m_componentPath;
    in >> container.m_nodeSource;
    in >> container.m_nodeSourceType;
    in >> container.m_metaType;
    in >> container.m_metaFlags;

    return in;
}

QDebug operator <<(QDebug debug, const InstanceContainer &command)
{
    debug.nospace() << "InstanceContainer("
                    << "instanceId: " << command.instanceId() << ", "
                    << "type: " << command.type() << ", "
                    << "majorNumber: " << command.majorNumber() << ", "
                    << "minorNumber: " << command.minorNumber() << ", ";

    if (!command.componentPath().isEmpty())
        debug.nospace() << "componentPath: " << command.componentPath() << ", ";

    if (!command.nodeSource().isEmpty())
        debug.nospace() << "nodeSource: " << command.nodeSource() << ", ";

    if (command.nodeSourceType() == InstanceContainer::NoSource)
        debug.nospace() << "nodeSourceType: NoSource, ";
    else if (command.nodeSourceType() == InstanceContainer::CustomParserSource)
        debug.nospace() << "nodeSourceType: CustomParserSource, ";
    else
        debug.nospace() << "nodeSourceType: ComponentSource, ";

    if (command.metaType() == InstanceContainer::ObjectMetaType)
        debug.nospace() << "metatype: ObjectMetaType";
    else
        debug.nospace() << "metatype: ItemMetaType";

    return debug.nospace() << ")";

}

} // namespace QmlDesigner
