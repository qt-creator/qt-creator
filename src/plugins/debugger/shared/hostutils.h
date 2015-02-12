/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_HOSTUTILS_H
#define DEBUGGER_HOSTUTILS_H

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

#endif // DEBUGGER_HOSTUTILS_H
