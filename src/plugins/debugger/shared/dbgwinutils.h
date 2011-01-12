/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_DBG_WINUTILS_H
#define DEBUGGER_DBG_WINUTILS_H

#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace Debugger {
namespace Internal {

class BreakpointParameters;

struct ProcData; // debuggerdialogs, used by the process listing dialogs

QList<ProcData> winProcessList();

// Resume a suspended thread by id.
bool winResumeThread(unsigned long dwThreadId, QString *errorMessage);

// Open a process by PID and break into it.
bool winDebugBreakProcess(unsigned long  pid, QString *errorMessage);

unsigned long winGetCurrentProcessId();

/* Helper for (case-)normalizing file names:
 * Determine normalized case of a Windows file name (camelcase.cpp -> CamelCase.cpp)
 * as the debugger reports lower case file names.
 * Restriction: File needs to exist and be non-empty and will be to be opened/mapped.
 * The result should be cached as the function can be extensive. */

QString winNormalizeFileName(const QString &f);

bool isWinProcessBeingDebugged(unsigned long pid);

// Special exception codes.
enum { winExceptionCppException = 0xe06d7363,
       winExceptionStartupCompleteTrap = 0x406d1388,
       winExceptionRpcServerUnavailable = 0x6ba,
       winExceptionRpcServerInvalid = 0x6a6,
       winExceptionDllNotFound = 0xc0000135,
       winExceptionDllEntryPointNoFound = 0xc0000139,
       winExceptionDllInitFailed = 0xc0000142,
       winExceptionMissingSystemFile = 0xc0000143,
       winExceptionAppInitFailed = 0xc0000143,
       winExceptionWX86Breakpoint = 0x4000001f
};

// Format windows Exception
void formatWindowsException(unsigned long code, quint64 address,
                            unsigned long flags, quint64 info1, quint64 info2,
                            QTextStream &str);
// Check for access violation, etc.
bool isFatalWinException(long code);

// Check for EXCEPTION_BREAKPOINT, EXCEPTION_SINGLE_STEP
bool isDebuggerWinException(long code);

// fix up breakpoints (catch/throw, etc).
BreakpointParameters fixWinMSVCBreakpoint(const BreakpointParameters &p);

// Special function names in MSVC runtime
extern const char *winMSVCThrowFunction;
extern const char *winMSVCCatchFunction;

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DBG_WINUTILS_H
