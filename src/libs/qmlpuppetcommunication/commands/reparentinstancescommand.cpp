// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "reparentinstancescommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

ReparentInstancesCommand::ReparentInstancesCommand() = default;

ReparentInstancesCommand::ReparentInstancesCommand(const QVector<ReparentContainer> &container)
    : m_reparentInstanceVector(container)
{
}

const QVector<ReparentContainer> ReparentInstancesCommand::reparentInstances() const
{
    return m_reparentInstanceVector;
}


QDataStream &operator<<(QDataStream &out, const ReparentInstancesCommand &command)
{
    out << command.reparentInstances();

    return out;
}

QDataStream &operator>>(QDataStream &in, ReparentInstancesCommand &command)
{
    in >> command.m_reparentInstanceVector;

    return in;
}

QDebug operator <<(QDebug debug, const ReparentInstancesCommand &command)
{
    return debug.nospace() << "ReparentInstancesCommand(reparentInstances: " << command.m_reparentInstanceVector << ")";
}

} // namespace QmlDesigner
