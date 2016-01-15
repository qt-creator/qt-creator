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

#include "idcontainer.h"

#include <QDebug>

namespace QmlDesigner {

IdContainer::IdContainer()
    : m_instanceId(-1)
{
}

IdContainer::IdContainer(qint32 instanceId, const QString &id)
    : m_instanceId(instanceId),
    m_id(id)
{
}

qint32 IdContainer::instanceId() const
{
    return m_instanceId;
}

QString IdContainer::id() const
{
    return m_id;
}

QDataStream &operator<<(QDataStream &out, const IdContainer &container)
{
    out << container.instanceId();
    out << container.id();

    return out;
}

QDataStream &operator>>(QDataStream &in, IdContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_id;

    return in;
}

QDebug operator <<(QDebug debug, const IdContainer &container)
{
    return debug.nospace() << "IdContainer("
                    << "instanceId: " << container.instanceId() << ", "
                    << "id: " << container.id() << ")";
}

} // namespace QmlDesigner
