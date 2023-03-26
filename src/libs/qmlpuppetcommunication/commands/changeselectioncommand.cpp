// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeselectioncommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

ChangeSelectionCommand::ChangeSelectionCommand() = default;

ChangeSelectionCommand::ChangeSelectionCommand(const QVector<qint32> &idVector)
    : m_instanceIdVector(idVector)
{
}

QVector<qint32> ChangeSelectionCommand::instanceIds() const
{
    return m_instanceIdVector;
}

QDataStream &operator<<(QDataStream &out, const ChangeSelectionCommand &command)
{
    out << command.instanceIds();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeSelectionCommand &command)
{
    in >> command.m_instanceIdVector;

    return in;
}

QDebug operator <<(QDebug debug, const ChangeSelectionCommand &command)
{
    return debug.nospace() << "ChangeSelectionCommand(instanceIdVector: " << command.m_instanceIdVector << ")";
}

bool operator ==(const ChangeSelectionCommand &first, const ChangeSelectionCommand &second)
{
    return first.m_instanceIdVector == second.m_instanceIdVector;
}

} // namespace QmlDesigner
