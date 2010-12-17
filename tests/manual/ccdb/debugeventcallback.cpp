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

#include "debugeventcallback.h"

#include <cstdio>

DebugEventCallback::DebugEventCallback()
{
}

STDMETHODIMP DebugEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = DEBUG_EVENT_CREATE_PROCESS  | DEBUG_EVENT_EXIT_PROCESS
             | DEBUG_EVENT_LOAD_MODULE   | DEBUG_EVENT_UNLOAD_MODULE
             | DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD
             | DEBUG_EVENT_BREAKPOINT
             | DEBUG_EVENT_EXCEPTION;
    return S_OK;
}

STDMETHODIMP DebugEventCallback::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT2)
{
    printf("Breakpoint hit\n");
    return S_OK;
}
STDMETHODIMP DebugEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 exc,
    __in ULONG firstChance
    )
{
    printf("Exception %ul occurred first-chance: %ul\n", exc->ExceptionCode,  firstChance);
    return S_OK;
}

STDMETHODIMP DebugEventCallback::CreateThread(
    THIS_
    __in ULONG64 /* Handle */,
    __in ULONG64 /* DataOffset */,
    __in ULONG64 /* StartOffset */
    )
{
    printf("Thread created\n");
    return S_OK;
}

STDMETHODIMP DebugEventCallback::ExitThread(
    THIS_
    __in ULONG /* ExitCode */
    )
{
    printf("Thread quit\n");
    return S_OK;
}

STDMETHODIMP DebugEventCallback::CreateProcess(
    THIS_
    __in ULONG64 /* ImageFileHandle */,
    __in ULONG64 Handle,
    __in ULONG64 /* Offset */,
    __in ULONG /* ModuleSize */,
    __in_opt PCWSTR /* ModuleName */,
    __in_opt PCWSTR /* ImageName */,
    __in ULONG /* CheckSum */,
    __in ULONG /* TimeDateStamp */,
    __in ULONG64 /* InitialThreadHandle */,
    __in ULONG64 /* ThreadDataOffset */,
    __in ULONG64 /* StartOffset */
    )
{
    printf("Process created %Ld\n", Handle);
    emit processAttached(reinterpret_cast<void*>(Handle));
    return S_OK;
}

STDMETHODIMP DebugEventCallback::ExitProcess(
    THIS_
    __in ULONG /* ExitCode */
    )
{
    printf("Process quit\n");
    return S_OK;
}

STDMETHODIMP DebugEventCallback::LoadModule(
    THIS_
    __in ULONG64 /* ImageFileHandle */,
    __in ULONG64 /* Offset */,
    __in ULONG /* ModuleSize */,
    __in_opt PCWSTR /* ModuleName */,
    __in_opt PCWSTR /* ImageName */,
    __in ULONG /* CheckSum */,
    __in ULONG /* TimeDateStamp */
    )
{
    printf("Module loaded\n");
    return S_OK;
}

STDMETHODIMP DebugEventCallback::UnloadModule(
    THIS_
    __in_opt PCWSTR /* ImageName */,
    __in ULONG64 /* Offset */
    )
{
    printf("Module unloaded\n");
    return S_OK;
}

STDMETHODIMP DebugEventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    printf("System error %ul at %ul\n", Error, Level);
    return S_OK;
}


STDMETHODIMP DebugEventCallback::ChangeDebuggeeState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    printf("Debuggee state changed %ul %ul\n", Flags, Argument);
    return S_OK;
}
