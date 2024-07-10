// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopprocesssignaloperation.h"

#include "../projectexplorertr.h"

#include <utils/winutils.h>
#include <utils/fileutils.h>
#include <utils/processinfo.h>

#include <QDir>
#include <QGuiApplication>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif // PROCESS_SUSPEND_RESUME
#else // Q_OS_WIN
#include <errno.h>
#include <signal.h>
#endif // else Q_OS_WIN

using namespace Utils;

namespace ProjectExplorer {

void DesktopProcessSignalOperation::killProcess(qint64 pid)
{
    killProcessSilently(pid);
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::killProcess(const QString &filePath)
{
    m_errorMessage.clear();
    const QList<ProcessInfo> processInfoList = ProcessInfo::processInfoList();
    for (const ProcessInfo &processInfo : processInfoList) {
        if (processInfo.commandLine == filePath)
            killProcessSilently(processInfo.processId);
    }
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::interruptProcess(qint64 pid)
{
    m_errorMessage.clear();
    interruptProcessSilently(pid);
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::interruptProcess(const QString &filePath)
{
    m_errorMessage.clear();
    const QList<ProcessInfo> processInfoList = ProcessInfo::processInfoList();
    for (const ProcessInfo &processInfo : processInfoList) {
        if (processInfo.commandLine == filePath)
            interruptProcessSilently(processInfo.processId);
    }
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::appendMsgCannotKill(qint64 pid, const QString &why)
{
    if (!m_errorMessage.isEmpty())
        m_errorMessage += QChar::fromLatin1('\n');
    m_errorMessage += Tr::tr("Cannot kill process with pid %1: %2").arg(pid).arg(why);
    m_errorMessage += QLatin1Char(' ');
}

void DesktopProcessSignalOperation::appendMsgCannotInterrupt(qint64 pid, const QString &why)
{
    if (!m_errorMessage.isEmpty())
        m_errorMessage += QChar::fromLatin1('\n');
    m_errorMessage += Tr::tr("Cannot interrupt process with pid %1: %2").arg(pid).arg(why);
    m_errorMessage += QLatin1Char(' ');
}

void DesktopProcessSignalOperation::killProcessSilently(qint64 pid)
{
#ifdef Q_OS_WIN
    const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
            |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
            |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
    if (const HANDLE handle = OpenProcess(rights, FALSE, DWORD(pid))) {
        if (!TerminateProcess(handle, UINT(-1)))
            appendMsgCannotKill(pid, winErrorMessage(GetLastError()));
        CloseHandle(handle);
    } else {
        appendMsgCannotKill(pid, Tr::tr("Cannot open process."));
    }
#else
    if (pid <= 0)
        appendMsgCannotKill(pid, Tr::tr("Invalid process id."));
    else if (kill(pid, SIGKILL))
        appendMsgCannotKill(pid, QString::fromLocal8Bit(strerror(errno)));
#endif // Q_OS_WIN
}

void DesktopProcessSignalOperation::interruptProcessSilently(qint64 pid)
{
#ifdef Q_OS_WIN
    HANDLE inferior = NULL;
    const DWORD rights = PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION | PROCESS_VM_OPERATION
                         | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_DUP_HANDLE
                         | PROCESS_TERMINATE | PROCESS_CREATE_THREAD | PROCESS_SUSPEND_RESUME;
    inferior = OpenProcess(rights, FALSE, pid);
    if (inferior == NULL) {
        appendMsgCannotInterrupt(
            pid, Tr::tr("Cannot open process: %1") + winErrorMessage(GetLastError()));
    } else if (!DebugBreakProcess(inferior)) {
        appendMsgCannotInterrupt(
            pid,
            Tr::tr("DebugBreakProcess failed:") + QLatin1Char(' ')
                + winErrorMessage(GetLastError()));
    }
    if (inferior != NULL)
        CloseHandle(inferior);
#else
    if (pid <= 0)
        appendMsgCannotInterrupt(pid, Tr::tr("Invalid process id."));
    else if (kill(pid, SIGINT))
        appendMsgCannotInterrupt(pid, QString::fromLocal8Bit(strerror(errno)));
#endif // Q_OS_WIN
}

} // namespace ProjectExplorer
