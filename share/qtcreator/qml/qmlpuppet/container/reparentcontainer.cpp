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

#include "reparentcontainer.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

ReparentContainer::ReparentContainer()
    : m_instanceId(-1),
    m_oldParentInstanceId(-1),
    m_newParentInstanceId(-1)
{
}

ReparentContainer::ReparentContainer(qint32 instanceId,
                  qint32 oldParentInstanceId,
                  const PropertyName &oldParentProperty,
                  qint32 newParentInstanceId,
                  const PropertyName &newParentProperty)
    : m_instanceId(instanceId),
    m_oldParentInstanceId(oldParentInstanceId),
    m_oldParentProperty(oldParentProperty),
    m_newParentInstanceId(newParentInstanceId),
    m_newParentProperty(newParentProperty)
{
}

qint32 ReparentContainer::instanceId() const
{
    return m_instanceId;
}

qint32 ReparentContainer::oldParentInstanceId() const
{
    return m_oldParentInstanceId;
}

PropertyName ReparentContainer::oldParentProperty() const
{
    return m_oldParentProperty;
}

qint32 ReparentContainer::newParentInstanceId() const
{
    return m_newParentInstanceId;
}

PropertyName ReparentContainer::newParentProperty() const
{
    return m_newParentProperty;
}

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container)
{
    out << container.instanceId();
    out << container.oldParentInstanceId();
    out << container.oldParentProperty();
    out << container.newParentInstanceId();
    out << container.newParentProperty();

    return out;
}

QDataStream &operator>>(QDataStream &in, ReparentContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_oldParentInstanceId;
    in >> container.m_oldParentProperty;
    in >> container.m_newParentInstanceId;
    in >> container.m_newParentProperty;

    return in;
}

QDebug operator <<(QDebug debug, const ReparentContainer &container)
{
    debug.nospace() << "ReparentContainer("
                    << "instanceId: " << container.instanceId();

    if (container.oldParentInstanceId() >= 0)
        debug.nospace() << ", " << "oldParentInstanceId: " << container.oldParentInstanceId();

    if (container.oldParentProperty().isEmpty())
        debug.nospace() << ", " << "oldParentProperty: " << container.oldParentProperty();

    if (container.newParentInstanceId() >= 0)
        debug.nospace() << ", " << "newParentInstanceId: " << container.newParentInstanceId();

    if (container.newParentProperty().isEmpty())
        debug.nospace() << ", " << "newParentProperty: " << container.newParentProperty();

    return debug.nospace() << ")";
}

} // namespace QmlDesigner
