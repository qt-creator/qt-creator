// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nanotracecommand.h"

namespace QmlDesigner {

StartNanotraceCommand::StartNanotraceCommand(const QString &path)
    : m_filePath(path)
{ }

QString StartNanotraceCommand::path() const
{
    return m_filePath;
}

QDataStream &operator<<(QDataStream &out, const StartNanotraceCommand &command)
{
    out << command.m_filePath;
    return out;
}

QDataStream &operator>>(QDataStream &in, StartNanotraceCommand &command)
{
    in >> command.m_filePath;
    return in;
}

QDebug operator<<(QDebug debug, const StartNanotraceCommand &command)
{
    return debug.nospace() << "StartNanotraceCommand(" << command.m_filePath << ")";
}


QDataStream &operator<<(QDataStream &out, const EndNanotraceCommand &/*command*/)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, EndNanotraceCommand &/*command*/)
{
    return in;
}

QDebug operator<<(QDebug debug, const EndNanotraceCommand &/*command*/)
{
    return debug.nospace() << "EndNanotraceCommand()";
}


SyncNanotraceCommand::SyncNanotraceCommand(const QString &name)
    : m_name(name)
{ }

QString SyncNanotraceCommand::name() const
{
    return m_name;
}

QDataStream &operator>>(QDataStream &in, SyncNanotraceCommand &command)
{
    in >> command.m_name;
    return in;
}

QDataStream &operator<<(QDataStream &out, const SyncNanotraceCommand &command)
{
    out << command.m_name;
    return out;
}

QDebug operator<<(QDebug debug, const SyncNanotraceCommand &command)
{
    return debug.nospace() << "SyncNanotraceCommand(" << command.m_name << ")";
}

} // namespace QmlDesigner
