// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "createinstancescommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

CreateInstancesCommand::CreateInstancesCommand() = default;

CreateInstancesCommand::CreateInstancesCommand(const QVector<InstanceContainer> &container)
    : m_instanceVector(container)
{
}

QVector<InstanceContainer> CreateInstancesCommand::instances() const
{
    return m_instanceVector;
}

QDataStream &operator<<(QDataStream &out, const CreateInstancesCommand &command)
{
    out << command.instances();

    return out;
}

QDataStream &operator>>(QDataStream &in, CreateInstancesCommand &command)
{
    in >> command.m_instanceVector;

    return in;
}

QDebug operator <<(QDebug debug, const CreateInstancesCommand &command)
{
    return debug.nospace() << "CreateInstancesCommand(" << command.instances() << ")";
}

} // namespace QmlDesigner
