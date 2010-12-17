/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cdbdebugeventcallback.h"
#include "cdbengine.h"
#include "cdbexceptionutils.h"
#include "cdbengine_p.h"
#include "dbgwinutils.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

namespace Debugger {
namespace Internal {

// ---------- CdbDebugEventCallback

CdbDebugEventCallback::CdbDebugEventCallback(CdbEngine *dbg) :
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
    const bool fatal = isFatalWinException(Exception->ExceptionCode);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\nex=" << Exception->ExceptionCode << " fatal=" << fatal << msg;
    m_pEngine->showMessage(msg, AppError);
    m_pEngine->showMessage(msg, LogMisc);
    m_pEngine->m_d->notifyException(Exception->ExceptionCode, fatal, msg);
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
    m_pEngine->m_d->handleModuleLoad(BaseOffset, QString::fromUtf16(reinterpret_cast<const ushort *>(ModuleName)));
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::UnloadModule(
    THIS_
    __in_opt PCWSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    Q_UNUSED(BaseOffset)
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << ImageBaseName;
    if (!ImageBaseName)
        return S_OK;
    m_pEngine->m_d->handleModuleUnload(QString::fromUtf16(reinterpret_cast<const ushort *>(ImageBaseName)));
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
         bool skipNonFatalExceptions, CdbEngine *engine) :
    m_logChannel(logChannel),
    m_skipNonFatalExceptions(skipNonFatalExceptions),
    m_engine(engine)
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
    const bool recordException = !m_skipNonFatalExceptions || isFatalWinException(Exception->ExceptionCode);
    QString msg;
    formatException(Exception, QTextStream(&msg));
    if (recordException) {
        m_exceptionCodes.push_back(Exception->ExceptionCode);
        m_exceptionMessages.push_back(msg);
    }
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n' << msg;
    m_engine->showMessage(msg, m_logChannel);
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
