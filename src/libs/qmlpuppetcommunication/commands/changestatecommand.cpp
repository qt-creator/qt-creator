// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changestatecommand.h"

#include <QDebug>

namespace QmlDesigner {

ChangeStateCommand::ChangeStateCommand()
    : m_stateInstanceId(-1)
{
}

ChangeStateCommand::ChangeStateCommand(qint32 stateInstanceId)
    : m_stateInstanceId(stateInstanceId)
{
}

qint32 ChangeStateCommand::stateInstanceId() const
{
    return m_stateInstanceId;
}

QDataStream &operator<<(QDataStream &out, const ChangeStateCommand &command)
{
    out << command.stateInstanceId();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeStateCommand &command)
{
    in >> command.m_stateInstanceId;

    return in;
}

QDebug operator <<(QDebug debug, const ChangeStateCommand &command)
{
    return debug.nospace() << "ChangeStateCommand(stateInstanceId: " << command.m_stateInstanceId << ")";
}

} // namespace QmlDesigner
