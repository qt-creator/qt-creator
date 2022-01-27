/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
