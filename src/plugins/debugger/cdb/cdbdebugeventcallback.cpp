/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cdbdebugeventcallback.h"
#include "cdbdebugengine.h"
#include "cdbexceptionutils.h"
#include "cdbdebugengine_p.h"
#include "debuggermanager.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

namespace Debugger {
namespace Internal {

// ---------- CdbDebugEventCallback

CdbDebugEventCallback::CdbDebugEventCallback(CdbDebugEngine* dbg) :
    m_pEngine(dbg)
{
}

STDMETHODIMP CdbDebugEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = DEBUG_EVENT_CREATE_PROCESS  | DEBUG_EVENT_EXIT_PROCESS
            | DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD
            | DEBUG_EVENT_BREAKPOINT
            | DEBUG_EVENT_EXCEPTION | baseInterestMask();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT2 Bp)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    m_pEngine->m_d->handleBreakpointEvent(Bp);
    return S_OK;
}


STDMETHODIMP CdbDebugEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG /* FirstChance */
    )
{
    QString msg;
    {
        QTextStream str(&msg);
        formatException(Exception, &m_pEngine->m_d->interfaces(), str);
    }
    const bool fatal = isFatalException(Exception->ExceptionCode);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\nex=" << Exception->ExceptionCode << " fatal=" << fatal << msg;
    m_pEngine->manager()->showApplicationOutput(msg);
    m_pEngine->manager()->showDebuggerOutput(LogMisc, msg);
    m_pEngine->m_d->notifyException(Exception->ExceptionCode, fatal);
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::CreateThread(
    THIS_
    __in ULONG64 Handle,
    __in ULONG64 DataOffset,
    __in ULONG64 StartOffset
    )
{
    Q_UNUSED(Handle)
    Q_UNUSED(DataOffset)
    Q_UNUSED(StartOffset)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    m_pEngine->m_d->updateThreadList();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ExitThread(
    THIS_
    __in ULONG ExitCode
    )
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << ExitCode;
    // @TODO: It seems the terminated thread is still in the list...
    m_pEngine->m_d->updateThreadList();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::CreateProcess(
    THIS_
    __in ULONG64 ImageFileHandle,
    __in ULONG64 Handle,
    __in ULONG64 BaseOffset,
    __in ULONG ModuleSize,
    __in_opt PCWSTR ModuleName,
    __in_opt PCWSTR ImageName,
    __in ULONG CheckSum,
    __in ULONG TimeDateStamp,
    __in ULONG64 InitialThreadHandle,
    __in ULONG64 ThreadDataOffset,
    __in ULONG64 StartOffset
    )
{
    Q_UNUSED(ImageFileHandle)
    Q_UNUSED(BaseOffset)
    Q_UNUSED(ModuleSize)
    Q_UNUSED(ModuleName)
    Q_UNUSED(ImageName)
    Q_UNUSED(CheckSum)
    Q_UNUSED(TimeDateStamp)
    Q_UNUSED(ThreadDataOffset)
    Q_UNUSED(StartOffset)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << ModuleName;
    m_pEngine->m_d->processCreatedAttached(Handle, InitialThreadHandle);
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ExitProcess(
    THIS_
    __in ULONG ExitCode
    )
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << ExitCode;
    m_pEngine->processTerminated(ExitCode);
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::LoadModule(
    THIS_
    __in ULONG64 ImageFileHandle,
    __in ULONG64 BaseOffset,
    __in ULONG ModuleSize,
    __in_opt PCWSTR ModuleName,
    __in_opt PCWSTR ImageName,
    __in ULONG CheckSum,
    __in ULONG TimeDateStamp
    )
{
    Q_UNUSED(ImageFileHandle)
    Q_UNUSED(BaseOffset)
    Q_UNUSED(ModuleSize)
    Q_UNUSED(ImageName)
    Q_UNUSED(CheckSum)
    Q_UNUSED(TimeDateStamp)
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << ModuleName;
    handleModuleLoad();
    m_pEngine->m_d->handleModuleLoad(QString::fromUtf16(reinterpret_cast<const ushort *>(ModuleName)));
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::UnloadModule(
    THIS_
    __in_opt PCWSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    Q_UNUSED(ImageBaseName)
    Q_UNUSED(BaseOffset)
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << ImageBaseName;
    handleModuleUnload();
    m_pEngine->m_d->updateModules();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << Error << Level;
    return S_OK;
}

// -----------ExceptionLoggerEventCallback
CdbExceptionLoggerEventCallback::CdbExceptionLoggerEventCallback(int logChannel,
                                                                 bool skipNonFatalExceptions,
                                                                 DebuggerManager *manager) :
    m_logChannel(logChannel),
    m_skipNonFatalExceptions(skipNonFatalExceptions),
    m_manager(manager)
{
}

STDMETHODIMP CdbExceptionLoggerEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = DEBUG_EVENT_EXCEPTION | baseInterestMask();
    return S_OK;
}

STDMETHODIMP CdbExceptionLoggerEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG /* FirstChance */
    )
{
    const bool recordException = !m_skipNonFatalExceptions || isFatalException(Exception->ExceptionCode);
    QString message;
    formatException(Exception, QTextStream(&message));
    if (recordException) {
        m_exceptionCodes.push_back(Exception->ExceptionCode);
        m_exceptionMessages.push_back(message);
    }
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n' << message;
    m_manager->showDebuggerOutput(m_logChannel, message);
    if (recordException)
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    return S_OK;
}

// -----------IgnoreDebugEventCallback
IgnoreDebugEventCallback::IgnoreDebugEventCallback()
{
}

STDMETHODIMP IgnoreDebugEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = 0;
    return S_OK;
}

} // namespace Internal
} // namespace Debugger
