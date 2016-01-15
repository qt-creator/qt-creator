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

InstanceContainer::InstanceContainer()
    : m_instanceId(-1), m_majorNumber(-1), m_minorNumber(-1)
{
}

InstanceContainer::InstanceContainer(qint32 instanceId, const TypeName &type, int majorNumber, int minorNumber, const QString &componentPath, const QString &nodeSource, NodeSourceType nodeSourceType, NodeMetaType metaType)
    : m_instanceId(instanceId), m_type(properDelemitingOfType(type)), m_majorNumber(majorNumber), m_minorNumber(minorNumber), m_componentPath(componentPath),
      m_nodeSource(nodeSource), m_nodeSourceType(nodeSourceType), m_metaType(metaType)
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

QDataStream &operator<<(QDataStream &out, const InstanceContainer &container)
{
    out << container.instanceId();
    out << container.type();
    out << container.majorNumber();
    out << container.minorNumber();
    out << container.componentPath();
    out << container.nodeSource();
    out << container.nodeSourceType();
    out << container.metaType();

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
