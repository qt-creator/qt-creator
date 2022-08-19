// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "synchronizecommand.h"

#include <QDebug>

namespace QmlDesigner {

SynchronizeCommand::SynchronizeCommand()
    : m_synchronizeId(-1)
{
}

SynchronizeCommand::SynchronizeCommand(int synchronizeId)
    : m_synchronizeId (synchronizeId)
{
}

int SynchronizeCommand::synchronizeId() const
{
    return m_synchronizeId;
}


QDataStream &operator<<(QDataStream &out, const SynchronizeCommand &command)
{
    out << command.synchronizeId();

    return out;
}

QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command)
{
    in >> command.m_synchronizeId;

    return in;
}

bool operator ==(const SynchronizeCommand &first, const SynchronizeCommand &second)
{
    return first.m_synchronizeId == second.m_synchronizeId;
}

QDebug operator <<(QDebug debug, const SynchronizeCommand &command)
{
    return debug.nospace() << "SynchronizeCommand(synchronizeId: " << command.synchronizeId() << ")";
}

} // namespace QmlDesigner
