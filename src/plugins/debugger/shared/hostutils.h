/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
enum
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
bool isFatalWinException(long code);

// Check for EXCEPTION_BREAKPOINT, EXCEPTION_SINGLE_STEP
bool isDebuggerWinException(unsigned long code);

} // namespace Internal
} // namespace Debugger
