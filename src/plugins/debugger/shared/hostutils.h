// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

// Resume a suspended thread by id.
bool winResumeThread(unsigned long dwThreadId, QString *errorMessage);

bool isWinProcessBeingDebugged(unsigned long pid);

// Special exception codes.
enum : unsigned long
{
    winExceptionCppException = 0xe06d7363,
    winExceptionSetThreadName = 0x406d1388,
    winExceptionRpcServerUnavailable = 0x6ba,
    winExceptionRpcServerInvalid = 0x6a6,
    winExceptionDllNotFound = 0xc0000135,
    winExceptionDllEntryPointNoFound = 0xc0000139,
    winExceptionDllInitFailed = 0xc0000142,
    winExceptionMissingSystemFile = 0xc0000143,
    winExceptionAppInitFailed = 0xc0000143,
    winExceptionWX86Breakpoint = 0x4000001f,
    winExceptionCtrlPressed = 0x40010005
};

// Format windows Exception
void formatWindowsException(unsigned long code, quint64 address,
                            unsigned long flags, quint64 info1, quint64 info2,
                            QTextStream &str);
// Check for access violation, etc.
bool isFatalWinException(unsigned long code);

// Check for EXCEPTION_BREAKPOINT, EXCEPTION_SINGLE_STEP
bool isDebuggerWinException(unsigned long code);

} // namespace Internal
} // namespace Debugger
