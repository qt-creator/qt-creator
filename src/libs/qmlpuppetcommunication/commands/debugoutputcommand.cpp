// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugoutputcommand.h"

#include <QtDebug>

namespace QmlDesigner {

DebugOutputCommand::DebugOutputCommand() = default;

DebugOutputCommand::DebugOutputCommand(const QString &text, DebugOutputCommand::Type type, const QVector<qint32> &instanceIds)
    : m_instanceIds(instanceIds)
    , m_text(text)
    , m_type(type)
{
}

qint32 DebugOutputCommand::type() const
{
    return m_type;
}

QString DebugOutputCommand::text() const
{
    return m_text;
}

QVector<qint32> DebugOutputCommand::instanceIds() const
{
    return m_instanceIds;
}

QDataStream &operator<<(QDataStream &out, const DebugOutputCommand &command)
{
    out << command.type();
    out << command.text();
    out << command.instanceIds();

    return out;
}

QDataStream &operator>>(QDataStream &in, DebugOutputCommand &command)
{
    in >> command.m_type;
    in >> command.m_text;
    in >> command.m_instanceIds;

    return in;
}

bool operator ==(const DebugOutputCommand &first, const DebugOutputCommand &second)
{
    return first.m_type == second.m_type
            && first.m_text == second.m_text;
}

QDebug operator <<(QDebug debug, const DebugOutputCommand &command)
{
        return debug.nospace() << "DebugOutputCommand("
                               << "type: " << command.type() << ", "
                               << "text: " << command.text() << ")";
}

} // namespace QmlDesigner
