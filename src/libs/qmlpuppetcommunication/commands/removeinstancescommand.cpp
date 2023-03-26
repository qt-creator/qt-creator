// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "removeinstancescommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

RemoveInstancesCommand::RemoveInstancesCommand() = default;

RemoveInstancesCommand::RemoveInstancesCommand(const QVector<qint32> &idVector)
    : m_instanceIdVector(idVector)
{
}

const QVector<qint32> RemoveInstancesCommand::instanceIds() const
{
    return m_instanceIdVector;
}

QDataStream &operator<<(QDataStream &out, const RemoveInstancesCommand &command)
{
    out << command.instanceIds();

    return out;
}

QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command)
{
    in >> command.m_instanceIdVector;

    return in;
}

QDebug operator <<(QDebug debug, const RemoveInstancesCommand &command)
{
    return debug.nospace() << "RemoveInstancesCommand(instanceIdVector: " << command.m_instanceIdVector << ")";
}

} // namespace QmlDesigner
