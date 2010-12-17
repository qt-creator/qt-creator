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

#ifndef DEBUGGER_CDBDEBUGEVENTCALLBACK_H
#define DEBUGGER_CDBDEBUGEVENTCALLBACK_H

#include "debugeventcallbackbase.h"

#include <QtCore/QStringList>

namespace Debugger {
namespace Internal {

class CdbEngine;

class CdbDebugEventCallback : public CdbCore::DebugEventCallbackBase
{
public:
    explicit CdbDebugEventCallback(CdbEngine* dbg);

    // IDebugEventCallbacks.

    STDMETHOD(GetInterestMask)(
        THIS_
        __out PULONG mask
        );

    STDMETHOD(Breakpoint)(
        THIS_
        __in PDEBUG_BREAKPOINT2 Bp
        );

    STDMETHOD(Exception)(
        THIS_
        __in PEXCEPTION_RECORD64 Exception,
        __in ULONG FirstChance
        );

    STDMETHOD(CreateThread)(
        THIS_
        __in ULONG64 Handle,
        __in ULONG64 DataOffset,
        __in ULONG64 StartOffset
        );
    STDMETHOD(ExitThread)(
        THIS_
        __in ULONG ExitCode
        );

    STDMETHOD(CreateProcess)(
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
        );

    STDMETHOD(ExitProcess)(
        THIS_
        __in ULONG ExitCode
        );

    STDMETHOD(LoadModule)(
        THIS_
        __in ULONG64 ImageFileHandle,
        __in ULONG64 BaseOffset,
        __in ULONG ModuleSize,
        __in_opt PCWSTR ModuleName,
        __in_opt PCWSTR ImageName,
        __in ULONG CheckSum,
        __in ULONG TimeDateStamp
        );

    STDMETHOD(UnloadModule)(
        THIS_
        __in_opt PCWSTR ImageBaseName,
        __in ULONG64 BaseOffset
        );

    STDMETHOD(SystemError)(
        THIS_
        __in ULONG Error,
        __in ULONG Level
        );

private:
    CdbEngine *m_pEngine;
};

// Event handler logs exceptions to the debugger window
// and ignores the rest. To be used for running dumper calls.
class CdbExceptionLoggerEventCallback : public CdbCore::DebugEventCallbackBase
{
public:
    CdbExceptionLoggerEventCallback(int logChannel,
                                    bool skipNonFatalExceptions,
                                    CdbEngine *engine);

    STDMETHOD(GetInterestMask)(
        THIS_
        __out PULONG mask
        );

    STDMETHOD(Exception)(
        THIS_
        __in PEXCEPTION_RECORD64 Exception,
        __in ULONG FirstChance
        );

    int exceptionCount() const { return m_exceptionMessages.size(); }
    QStringList exceptionMessages() const { return m_exceptionMessages; }
    QList<ULONG> exceptionCodes()  const { return m_exceptionCodes; }

private:
    const int m_logChannel;
    const bool m_skipNonFatalExceptions;
    CdbEngine *m_engine;
    QList<ULONG> m_exceptionCodes;
    QStringList m_exceptionMessages;
};

// Event handler that ignores everything
class IgnoreDebugEventCallback : public CdbCore::DebugEventCallbackBase
{
public:
    explicit IgnoreDebugEventCallback();

    STDMETHOD(GetInterestMask)(
        THIS_
        __out PULONG mask
        );
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBDEBUGEVENTCALLBACK_H
