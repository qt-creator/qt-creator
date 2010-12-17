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

#ifndef CDBCOMSTUB_H
#define CDBCOMSTUB_H

// Stubs to make it partially compile for test purposes on non-Windows.

// FIXME: Make everything more Windows-like instead of choosing arbitrary
// values to make it compile.

typedef unsigned long ULONG;
typedef unsigned long long ULONG64;
typedef void *PVOID;
typedef unsigned short *PCWSTR;
typedef void *HANDLE;
typedef int HRESULT;
typedef int DEBUG_VALUE;
typedef int PDEBUG_BREAKPOINT2;
const int MAX_PATH = 1024;

inline bool FAILED(HRESULT) { return false; }

enum
{
    DEBUG_OUTPUT_PROMPT_REGISTERS = 1,
    DEBUG_OUTPUT_EXTENSION_WARNING = 2,
    DEBUG_OUTPUT_WARNING = 4,
    DEBUG_OUTPUT_ERROR = 8,
    DEBUG_OUTPUT_DEBUGGEE = 16,
    DEBUG_OUTPUT_DEBUGGEE_PROMPT = 32,
    DEBUG_OUTPUT_PROMPT = 64,
};

#define IN
#define OUT
#define THIS
#define THIS_
#define REFIID void *
#define THIS_
#define STDMETHOD(x) HRESULT x
#define STDMETHOD_(x, y) x y

struct IUnknown
{
    virtual ~IUnknown();
    virtual STDMETHOD_(ULONG, AddRef)(THIS) { return 1; }
    virtual STDMETHOD_(ULONG, Release)(THIS) { return 1; }
};

struct IDebugOutputCallbacksWide : IUnknown
{
};

struct CIDebugClient : IUnknown
{
};

struct CIDebugControl : IUnknown
{
};

struct CIDebugSystemObjects : IUnknown
{
};

struct CIDebugSymbols : IUnknown
{
};

struct CIDebugRegisters : IUnknown
{
    HRESULT GetNumberRegisters(ULONG *) const { return 0; }
    HRESULT GetDescription(ULONG, char *, int, int, int) const { return 0; }
    HRESULT GetValues(ULONG, int, int, DEBUG_VALUE *) const { return 0; }
};

struct CIDebugDataSpaces : IUnknown
{
};

struct CIDebugSymbolGroup : IUnknown
{
};

struct CIDebugBreakpoint : IUnknown
{
};

enum DebugSymbolFlags
{
    DEBUG_SYMBOL_IS_LOCAL = 1,
    DEBUG_SYMBOL_IS_ARGUMENT = 2,
    DEBUG_SYMBOL_READ_ONLY = 4
};

struct DEBUG_SYMBOL_PARAMETERS
{
    DebugSymbolFlags Flags;
    unsigned long ParentSymbol;
};

struct DEBUG_STACK_FRAME
{
};

#endif // Q_OS_WIN
