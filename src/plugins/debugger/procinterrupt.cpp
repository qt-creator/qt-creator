// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "procinterrupt.h"
#include "debuggerconstants.h"

#include <QDir>
#include <QGuiApplication>
#include <QProcess> // makes kill visible on Windows.

using namespace Debugger::Internal;

static inline QString msgCannotInterrupt(qint64 pid, const QString &why)
{
    return QString::fromLatin1("Cannot interrupt process %1: %2").arg(pid).arg(why);
}

#if defined(Q_OS_WIN)
#include <utils/winutils.h>
#include <windows.h>

#if !defined(PROCESS_SUSPEND_RESUME) // Check flag for MinGW
#    define PROCESS_SUSPEND_RESUME (0x0800)
#endif // PROCESS_SUSPEND_RESUME

// Open the process and break into it
bool Debugger::Internal::interruptProcess(qint64 pID, QString *errorMessage)
{
    bool ok = false;
    HANDLE inferior = NULL;
    const DWORD rights = PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION | PROCESS_VM_OPERATION
                         | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_DUP_HANDLE
                         | PROCESS_TERMINATE | PROCESS_CREATE_THREAD | PROCESS_SUSPEND_RESUME;
    inferior = OpenProcess(rights, FALSE, DWORD(pID));
    if (inferior == NULL) {
        *errorMessage = QString::fromLatin1("Cannot open process %1: %2")
                            .arg(pID)
                            .arg(Utils::winErrorMessage(GetLastError()));
    } else if (ok = DebugBreakProcess(inferior); !ok) {
        *errorMessage = "DebugBreakProcess failed: " + Utils::winErrorMessage(GetLastError());
    }

    if (inferior != NULL)
        CloseHandle(inferior);
    if (!ok)
        *errorMessage = msgCannotInterrupt(pID, *errorMessage);
    return ok;
}

#else // Q_OS_WIN

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

bool Debugger::Internal::interruptProcess(qint64 pID, QString *errorMessage)
{
    if (pID <= 0) {
        *errorMessage = msgCannotInterrupt(pID, QString::fromLatin1("Invalid process id."));
        return false;
    }
    if (kill(pID, SIGINT)) {
        *errorMessage = msgCannotInterrupt(pID, QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    return true;
}

#endif // !Q_OS_WIN
