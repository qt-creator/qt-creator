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

#include "instancecontainer.h"

#include <QDataStream>

namespace QmlDesigner {

InstanceContainer::InstanceContainer()
    : m_instanceId(-1), m_majorNumber(-1), m_minorNumber(-1)
{
}

InstanceContainer::InstanceContainer(qint32 instanceId, const QString &type, int majorNumber, int minorNumber, const QString &componentPath, const QString &nodeSource, NodeSourceType nodeSourceType,NodeMetaType metaType)
    : m_instanceId(instanceId), m_type(type), m_majorNumber(majorNumber), m_minorNumber(minorNumber), m_componentPath(componentPath),
      m_nodeSource(nodeSource), m_nodeSourceType(nodeSourceType), m_metaType(metaType)
{
    m_type.replace(QLatin1Char('.'), QLatin1Char('/'));
}

qint32 InstanceContainer::instanceId() const
{
    return m_instanceId;
}

QString InstanceContainer::type() const
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
} // namespace QmlDesigner
