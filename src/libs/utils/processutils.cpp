/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "processutils.h"

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#else
#include <errno.h>
#include <stdio.h>
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

void ProcessStartHandler::setBelowNormalPriority()
{
#ifdef Q_OS_WIN
    m_process->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *args) {
            args->flags |= BELOW_NORMAL_PRIORITY_CLASS;
    });
#endif // Q_OS_WIN
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

void ProcessHelper::setUseCtrlCStub(bool enabled)
{
    // Do not use the stub in debug mode. Activating the stub will shut down
    // Qt Creator otherwise, because they share the same Windows console.
    // See QTCREATORBUG-11995 for details.
#ifdef QT_DEBUG
    Q_UNUSED(enabled)
#else
    m_useCtrlCStub = enabled;
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
    Q_UNUSED(pid)
#endif
}

void ProcessHelper::interruptProcess(QProcess *process)
{
    ProcessHelper *helper = qobject_cast<ProcessHelper *>(process);
    if (helper && helper->m_useCtrlCStub)
        ProcessHelper::interruptPid(process->processId());
}

void ProcessHelper::setupChildProcess_impl()
{
#if defined Q_OS_UNIX
    // nice value range is -20 to +19 where -20 is highest, 0 default and +19 is lowest
    if (m_lowPriority) {
        errno = 0;
        if (::nice(5) == -1 && errno != 0)
            perror("Failed to set nice value");
    }

    // Disable terminal by becoming a session leader.
    if (m_unixTerminalDisabled)
        setsid();
#endif
}

} // namespace Utils
