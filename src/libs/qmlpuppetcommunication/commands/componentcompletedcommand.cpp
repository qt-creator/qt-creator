// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentcompletedcommand.h"

#include <QDataStream>
#include <QDebug>

#include <algorithm>

namespace QmlDesigner {

ComponentCompletedCommand::ComponentCompletedCommand() = default;

ComponentCompletedCommand::ComponentCompletedCommand(const QVector<qint32> &container)
    : m_instanceVector(container)
{
}

QVector<qint32> ComponentCompletedCommand::instances() const
{
    return m_instanceVector;
}

void ComponentCompletedCommand::sort()
{
    std::sort(m_instanceVector.begin(), m_instanceVector.end());
}

QDataStream &operator<<(QDataStream &out, const ComponentCompletedCommand &command)
{
    out << command.instances();

    return out;
}

QDataStream &operator>>(QDataStream &in, ComponentCompletedCommand &command)
{
    in >> command.m_instanceVector;

    return in;
}

bool operator ==(const ComponentCompletedCommand &first, const ComponentCompletedCommand &second)
{
    return first.m_instanceVector == second.m_instanceVector;
}

QDebug operator <<(QDebug debug, const ComponentCompletedCommand &command)
{
    return debug.nospace() << "ComponentCompletedCommand(" << command.instances() << ")";

}

} // namespace QmlDesigner

