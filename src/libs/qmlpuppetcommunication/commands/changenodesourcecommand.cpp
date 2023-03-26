// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changenodesourcecommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

ChangeNodeSourceCommand::ChangeNodeSourceCommand() = default;

ChangeNodeSourceCommand::ChangeNodeSourceCommand(qint32 newInstanceId, const QString &newNodeSource)
    : m_instanceId(newInstanceId), m_nodeSource(newNodeSource)
{
}

qint32 ChangeNodeSourceCommand::instanceId() const
{
    return m_instanceId;
}

QString ChangeNodeSourceCommand::nodeSource() const
{
    return m_nodeSource;
}

QDataStream &operator<<(QDataStream &out, const ChangeNodeSourceCommand &command)
{
    out << command.instanceId();
    out << command.nodeSource();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeNodeSourceCommand &command)
{
    in >> command.m_instanceId;
    in >> command.m_nodeSource;

    return in;
}

QDebug operator <<(QDebug debug, const ChangeNodeSourceCommand &command)
{
    return debug.nospace() << "ReparentInstancesCommand("
                           << "instanceId: " << command.m_instanceId
                           << "nodeSource: " << command.m_nodeSource << ")";
}

} // namespace QmlDesigner
