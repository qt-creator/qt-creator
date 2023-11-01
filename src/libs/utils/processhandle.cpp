// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processhandle.h"

namespace Utils {

/*!
    \class Utils::ProcessHandle
    \inmodule QtCreator
    \brief The ProcessHandle class is a helper class to describe a process.

    Encapsulates parameters of a running process, local (PID) or remote (to be
    done, address, port, and so on).
*/

// That's the same as in QProcess, i.e. Qt doesn't care for process #0.
const qint64 InvalidPid = 0;

ProcessHandle::ProcessHandle()
    : m_pid(InvalidPid)
{
}

ProcessHandle::ProcessHandle(qint64 pid)
    : m_pid(pid)
{
}

bool ProcessHandle::isValid() const
{
    return m_pid != InvalidPid;
}

void ProcessHandle::setPid(qint64 pid)
{
    m_pid = pid;
}

qint64 ProcessHandle::pid() const
{
    return m_pid;
}

bool ProcessHandle::equals(const ProcessHandle &rhs) const
{
    return m_pid == rhs.m_pid;
}

#ifndef Q_OS_MACOS
bool ProcessHandle::activate()
{
    return false;
}
#endif

} // Utils
