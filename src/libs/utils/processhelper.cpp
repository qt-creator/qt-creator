// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processhelper.h"

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#else
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace Utils {

QIODevice::OpenMode ProcessStartHandler::openMode() const
{
    if (m_processMode == ProcessMode::Writer)
        return QIODevice::ReadWrite; // some writers also read data
    if (m_writeData.isEmpty())
        return QIODevice::ReadOnly; // only reading
    return QIODevice::ReadWrite; // initial write and then reading (close the write channel)
}

void ProcessStartHandler::handleProcessStart()
{
    if (m_processMode == ProcessMode::Writer)
        return;
    if (m_writeData.isEmpty())
        m_process->closeWriteChannel();
}

void ProcessStartHandler::handleProcessStarted()
{
    if (!m_writeData.isEmpty()) {
        m_process->write(m_writeData);
        m_writeData = {};
        if (m_processMode == ProcessMode::Reader)
            m_process->closeWriteChannel();
    }
}

void ProcessStartHandler::setNativeArguments(const QString &arguments)
{
#ifdef Q_OS_WIN
    if (!arguments.isEmpty())
        m_process->setNativeArguments(arguments);
#else
    Q_UNUSED(arguments)
#endif // Q_OS_WIN
}

void ProcessStartHandler::setWindowsSpecificStartupFlags(
    bool belowNormalPriority, bool createConsoleWindow, bool forceDefaultErrorMode)
{
#ifdef Q_OS_WIN
    m_process->setCreateProcessArgumentsModifier(
        [belowNormalPriority, createConsoleWindow, forceDefaultErrorMode](
            QProcess::CreateProcessArguments *args) {
            if (createConsoleWindow) {
                args->flags |= CREATE_NEW_CONSOLE;
                args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
            }

            if (belowNormalPriority)
                args->flags |= BELOW_NORMAL_PRIORITY_CLASS;

            if (forceDefaultErrorMode)
                args->flags |= CREATE_DEFAULT_ERROR_MODE;
        });
#else // Q_OS_WIN
    Q_UNUSED(belowNormalPriority)
    Q_UNUSED(createConsoleWindow)
    Q_UNUSED(forceDefaultErrorMode)
#endif
}

#ifdef Q_OS_WIN
static BOOL sendMessage(UINT message, HWND hwnd, LPARAM lParam)
{
    DWORD dwProcessID;
    GetWindowThreadProcessId(hwnd, &dwProcessID);
    if ((DWORD)lParam == dwProcessID) {
        SendNotifyMessage(hwnd, message, 0, 0);
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK sendShutDownMessageToAllWindowsOfProcess_enumWnd(HWND hwnd, LPARAM lParam)
{
    static UINT uiShutDownMessage = RegisterWindowMessage(L"qtcctrlcstub_shutdown");
    return sendMessage(uiShutDownMessage, hwnd, lParam);
}

BOOL CALLBACK sendInterruptMessageToAllWindowsOfProcess_enumWnd(HWND hwnd, LPARAM lParam)
{
    static UINT uiInterruptMessage = RegisterWindowMessage(L"qtcctrlcstub_interrupt");
    return sendMessage(uiInterruptMessage, hwnd, lParam);
}
#endif

ProcessHelper::ProcessHelper(QObject *parent)
    : QProcess(parent), m_processStartHandler(this)
{}

void ProcessHelper::setLowPriority()
{
    m_lowPriority = true;
    enableChildProcessModifier();
}

void ProcessHelper::setUnixTerminalDisabled()
{
#if defined(Q_OS_UNIX)
#  if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    m_unixTerminalDisabled = true;
    enableChildProcessModifier();
#  else
    UnixProcessParameters params = unixProcessParameters();
    params.flags |= UnixProcessFlag::CreateNewSession;
    setUnixProcessParameters(params);
#  endif
#endif
}

void ProcessHelper::setUseCtrlCStub(bool enabled)
{
    m_useCtrlCStub = enabled;
}

void Utils::ProcessHelper::setAllowCoreDumps(bool enabled)
{
#if defined(Q_OS_UNIX) && QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    UnixProcessParameters params = unixProcessParameters();
    params.flags.setFlag(UnixProcessFlag::DisableCoreDumps, enabled);
    setUnixProcessParameters(params);
#else
    Q_UNUSED(enabled)
#endif
}

void ProcessHelper::enableChildProcessModifier()
{
#if defined(Q_OS_UNIX)
    setChildProcessModifier([this] {
        // nice value range is -20 to +19 where -20 is highest, 0 default and +19 is lowest
        if (m_lowPriority) {
            errno = 0;
            if (::nice(5) == -1 && errno != 0)
                perror("Failed to set nice value");
        }

#  if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
        // Disable terminal by becoming a session leader.
        if (m_unixTerminalDisabled)
            setsid();
#  endif
    });
#endif
}

void ProcessHelper::terminateProcess()
{
#ifdef Q_OS_WIN
    if (m_useCtrlCStub)
        EnumWindows(sendShutDownMessageToAllWindowsOfProcess_enumWnd, processId());
    else
        terminate();
#else
    terminate();
#endif
}

void ProcessHelper::terminateProcess(QProcess *process)
{
    ProcessHelper *helper = qobject_cast<ProcessHelper *>(process);
    if (helper)
        helper->terminateProcess();
    else
        process->terminate();
}

void ProcessHelper::interruptPid(qint64 pid)
{
#ifdef Q_OS_WIN
    EnumWindows(sendInterruptMessageToAllWindowsOfProcess_enumWnd, pid);
#else
    ::kill(pid, SIGINT);
#endif
}

} // namespace Utils
