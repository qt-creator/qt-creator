// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completecomponentcommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

CompleteComponentCommand::CompleteComponentCommand() = default;

CompleteComponentCommand::CompleteComponentCommand(const QVector<qint32> &container)
    : m_instanceVector(container)
{
}

const QVector<qint32> CompleteComponentCommand::instances() const
{
    return m_instanceVector;
}

QDataStream &operator<<(QDataStream &out, const CompleteComponentCommand &command)
{
    out << command.instances();

    return out;
}

QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command)
{
    in >> command.m_instanceVector;

    return in;
}

QDebug operator <<(QDebug debug, const CompleteComponentCommand &command)
{
    return debug.nospace() << "CompleteComponentCommand(instances: " << command.m_instanceVector << ")";
}

} // namespace QmlDesigner
